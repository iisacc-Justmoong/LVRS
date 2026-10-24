#include "HierarchyModel.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <cmath>
#include <thread>
#include <unordered_map>
namespace lvrs {
using namespace ii;
namespace {
template<class T,class F>core::Result<T> protect(F&& f){try{return f();}catch(const std::bad_alloc&){return core::fail(core::ErrorCode::full,"Hierarchy allocation failed");}catch(...){return core::fail(core::ErrorCode::failed,"Hierarchy operation failed",std::current_exception());}}
struct Hash final{using is_transparent=void;std::size_t operator()(std::string_view v)const noexcept{return std::hash<std::string_view>{}(v);}};
std::size_t text_cost(const HierarchyRow& r){return r.key.size()*2+r.label.size()+r.icon_name.size()+r.icon_glyph.size();}
bool valid(HierarchyModelOptions o){return o.maximum_rows&&o.maximum_rows<=4000000&&o.maximum_depth&&o.maximum_depth<=1024&&o.maximum_text_bytes&&o.maximum_text_bytes<=64*1024*1024&&o.maximum_data_bytes>=4096&&o.maximum_data_bytes<=std::size_t(1024)*1024*1024&&o.maximum_metadata_bytes>=1024&&o.maximum_metadata_bytes<=64*1024*1024;}
}
struct HierarchyModel::Impl final{
    using Keys=std::unordered_map<std::string,std::size_t,Hash,std::equal_to<>>;
    struct Prepared final{std::vector<HierarchyRow> rows;std::vector<HierarchyRelation> relations;Keys keys;std::unordered_map<std::int64_t,std::size_t> ids;std::size_t bytes=0;};
    HierarchyModelOptions options;Prepared data;HierarchyModelStats stats;
    std::thread::id owner=std::this_thread::get_id();std::uint64_t revision=1;
    std::vector<std::weak_ptr<const HierarchyChangePort>> ports;
    bool closed=false,committing=false,notifying=false;
    core::Result<void> ready(bool write=false)const{
        if(owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"HierarchyModel requires its owner thread");
        if(closed)return core::fail(core::ErrorCode::closed,"HierarchyModel is closed");
        if(write&&(committing||notifying))return core::fail(core::ErrorCode::busy,"HierarchyModel notifications are read-only");
        return {};
    }
    core::Result<void> normalize(HierarchyRow& r,std::size_t index)const{
        const auto bounded=[&](std::string_view s,std::size_t max){return s.size()<=max&&core::valid_utf8(s)&&s.find('\0')==s.npos;};
        if(!bounded(r.key,1024)||!bounded(r.label,options.maximum_text_bytes)||!bounded(r.icon_name,4096)||!bounded(r.icon_glyph,4096)||r.depth>options.maximum_depth)return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy key, text or depth");
        const auto first=r.key.find_first_not_of(" \t\r\n"),last=r.key.find_last_not_of(" \t\r\n");
        r.key=first==r.key.npos?std::string{}:r.key.substr(first,last-first+1);
        if(r.key.empty())r.key=std::to_string(r.item_id>=0?r.item_id:std::int64_t(index));
        if(r.label.empty())r.label=r.key;
        if(r.label.size()>options.maximum_text_bytes)return core::fail(core::ErrorCode::invalid_argument,"Normalized hierarchy label exceeds its text budget");
        return {};
    }
    core::Result<Prepared> prepare(std::vector<HierarchyRow> rows)const{
        if(rows.size()>options.maximum_rows)return core::fail(core::ErrorCode::full,"Hierarchy row admission limit exceeded");
        Prepared p;const auto fixed=sizeof(HierarchyRow)+sizeof(HierarchyRelation)+96;
        if(rows.size()>options.maximum_data_bytes/fixed)return core::fail(core::ErrorCode::full,"Hierarchy index admission budget exceeded");
        p.bytes=rows.size()*fixed;p.rows=std::move(rows);p.relations.resize(p.rows.size());p.keys.reserve(p.rows.size());p.ids.reserve(p.rows.size());
        std::vector<std::size_t> stack,last(p.rows.size()+1,hierarchy_npos),counts(p.rows.size()+1);
        for(std::size_t i=0;i<p.rows.size();++i){
            auto& row=p.rows[i];if(auto r=normalize(row,i);!r)return std::unexpected(r.error());
            if((!i&&row.depth)||(i&&row.depth>p.rows[i-1].depth+1))return core::fail(core::ErrorCode::invalid_argument,"Hierarchy preorder depth must begin at zero without gaps");
            const auto bytes=text_cost(row);if(bytes>options.maximum_data_bytes-p.bytes)return core::fail(core::ErrorCode::full,"Hierarchy text admission budget exceeded");p.bytes+=bytes;
            if(!p.keys.emplace(row.key,i).second)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy keys must be unique");
            p.ids.try_emplace(row.item_id>=0?row.item_id:std::int64_t(i),i);
            while(stack.size()>row.depth){p.relations[stack.back()].subtree_end=i;stack.pop_back();}
            auto& info=p.relations[i];info.parent=stack.empty()?hierarchy_npos:stack.back();
            const auto bucket=info.parent==hierarchy_npos?p.rows.size():info.parent;info.sibling_index=counts[bucket]++;
            info.previous_sibling=last[bucket];if(last[bucket]!=hierarchy_npos)p.relations[last[bucket]].next_sibling=i;last[bucket]=i;
            if(info.parent!=hierarchy_npos){auto& parent=p.relations[info.parent];if(!parent.child_count)parent.first_child=i;++parent.child_count;}
            stack.push_back(i);
        }
        for(auto i:stack)p.relations[i].subtree_end=p.rows.size();
        for(std::size_t i=p.rows.size();i-->0;){auto& info=p.relations[i];info.subtree_maximum_depth=std::max(info.subtree_maximum_depth,p.rows[i].depth);if(info.parent!=hierarchy_npos)p.relations[info.parent].subtree_maximum_depth=std::max(p.relations[info.parent].subtree_maximum_depth,info.subtree_maximum_depth);info.sibling_count=counts[info.parent==hierarchy_npos?p.rows.size():info.parent];}
        return p;
    }
    core::Result<void> publish(HierarchyChange change){
        change.revision=revision;change.count=data.rows.size();++stats.notifications;
        std::erase_if(ports,[](const auto& port){return port.expired();});const auto targets=ports;
        notifying=true;struct Reset{bool& flag;~Reset(){flag=false;}}reset{notifying};
        bool failed=false;std::exception_ptr cause;
        for(const auto& weak:targets){if(closed&&change.kind!=HierarchyChangeKind::closed)break;if(auto port=weak.lock()){auto sent=port->send(change);if(!sent&&sent.error().code!=runtime::QueryErrorCode::receiver_expired){if(!failed)cause=sent.error().cause;failed=true;}}}
        if(failed)return core::fail(core::ErrorCode::failed,"Committed hierarchy notification failed",cause);return {};
    }
    core::Result<HierarchyMovePlan> plan(std::string_view key,std::size_t raw,std::uint32_t desired)const{
        auto found=data.keys.find(key);if(found==data.keys.end())return core::fail(core::ErrorCode::not_found,"Hierarchy move source was not found");
        const auto begin=found->second,end=data.relations[begin].subtree_end,count=data.rows.size(),length=end-begin;
        if(raw>count||(raw>begin&&raw<end))return core::fail(core::ErrorCode::invalid_argument,"Hierarchy insertion is inside the moved subtree");
        const auto gap=raw>=end?raw-length:raw;
        const auto original=[&](std::size_t remaining){return remaining<begin?remaining:remaining+length;};
        const auto previous=gap?original(gap-1):hierarchy_npos,next=gap<count-length?original(gap):hierarchy_npos;
        const auto minimum=next==hierarchy_npos?0:data.rows[next].depth,maximum=previous==hierarchy_npos?0:data.rows[previous].depth+1;
        const auto depth=std::max(minimum,std::min(maximum,desired));if(depth>options.maximum_depth)return core::fail(core::ErrorCode::out_of_range,"Hierarchy insertion depth exceeds its budget");
        auto parent=previous;
        if(!depth)parent=hierarchy_npos;
        else{while(parent!=hierarchy_npos&&data.rows[parent].depth>depth-1)parent=data.relations[parent].parent;if(parent==hierarchy_npos||data.rows[parent].depth!=depth-1)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy insertion has no parent at the requested depth");}
        if(std::int64_t(data.relations[begin].subtree_maximum_depth)+std::int64_t(depth)-data.rows[begin].depth>std::int64_t(options.maximum_depth))return core::fail(core::ErrorCode::out_of_range,"Moved hierarchy subtree exceeds its depth budget");
        HierarchyDropMode mode=HierarchyDropMode::root;auto anchor=hierarchy_npos;
        if(parent!=hierarchy_npos&&parent==previous){mode=HierarchyDropMode::child;anchor=parent;}
        else if(next!=hierarchy_npos&&data.rows[next].depth==depth){mode=HierarchyDropMode::before;anchor=next;}
        else if(previous!=hierarchy_npos&&data.rows[previous].depth==depth){mode=HierarchyDropMode::after;anchor=previous;}
        else if(parent!=hierarchy_npos){mode=HierarchyDropMode::child;anchor=parent;}
        else if(next!=hierarchy_npos&&data.rows[next].depth==0){mode=HierarchyDropMode::before;anchor=next;}
        else if(previous!=hierarchy_npos&&data.rows[previous].depth==0){mode=HierarchyDropMode::after;anchor=previous;}
        return HierarchyMovePlan{revision,begin,end,raw,gap,parent,anchor,depth,minimum,maximum,mode,data.rows[begin].key,parent==hierarchy_npos?"":data.rows[parent].key,anchor==hierarchy_npos?"":data.rows[anchor].key};
    }
};
HierarchyModel::HierarchyModel(std::shared_ptr<Impl> p):impl_(std::move(p)){}
HierarchyModel::~HierarchyModel(){(void)close();}
core::Result<std::shared_ptr<HierarchyModel>> HierarchyModel::create(HierarchyModelOptions options){
    if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid HierarchyModel options");
    return protect<std::shared_ptr<HierarchyModel>>([&]{auto p=std::make_shared<Impl>();p->options=options;return std::shared_ptr<HierarchyModel>(new HierarchyModel(std::move(p)));});
}
core::Result<bool> HierarchyModel::set_rows(std::vector<HierarchyRow> rows){auto p=impl_;
    if(auto r=p->ready(true);!r)return std::unexpected(r.error());
    return protect<bool>([&]()->core::Result<bool>{auto next=p->prepare(std::move(rows));if(!next)return std::unexpected(next.error());if(next->rows==p->data.rows)return false;
        p->committing=true;struct Reset{bool& flag;~Reset(){flag=false;}}reset{p->committing};
        p->data=std::move(*next);++p->revision;++p->stats.topology_builds;
        if(auto r=p->publish({.kind=HierarchyChangeKind::reset,.end=p->data.rows.size(),.visibility=true,.activation=true});!r)return std::unexpected(r.error());return true;});
}
core::Result<bool> HierarchyModel::update_row(std::string_view key,HierarchyRow row){auto p=impl_;
    if(auto r=p->ready(true);!r)return std::unexpected(r.error());
    return protect<bool>([&]()->core::Result<bool>{
        const auto found=p->data.keys.find(key);if(found==p->data.keys.end())return core::fail(core::ErrorCode::not_found,"Hierarchy row was not found");
        const auto i=found->second;if(row.key.empty())row.key=std::string(key);if(auto r=p->normalize(row,i);!r)return std::unexpected(r.error());const auto& old=p->data.rows[i];
        if(row.key!=old.key||row.item_id!=old.item_id||row.depth!=old.depth)return core::fail(core::ErrorCode::invalid_argument,"Use set_rows or a subtree move to change hierarchy identity or depth");
        if(row==old)return false;const auto before=text_cost(old),after=text_cost(row);
        if(after>before&&after-before>p->options.maximum_data_bytes-p->data.bytes)return core::fail(core::ErrorCode::full,"Hierarchy row update exceeds its text budget");
        const bool visibility=row.expanded!=old.expanded||row.show_chevron!=old.show_chevron||row.has_child_items!=old.has_child_items;
        const bool activation=row.enabled!=old.enabled||row.activatable!=old.activatable||row.selected!=old.selected;
        p->data.rows[i]=std::move(row);p->data.bytes=p->data.bytes-before+after;++p->revision;++p->stats.data_updates;
        if(auto r=p->publish({.kind=HierarchyChangeKind::data,.first=i,.end=i+1,.visibility=visibility,.activation=activation});!r)return std::unexpected(r.error());return true;});
}
core::Result<bool> HierarchyModel::set_editable(bool value){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->options.editable==value)return false;p->options.editable=value;++p->revision;if(auto r=p->publish({.kind=HierarchyChangeKind::capabilities});!r)return std::unexpected(r.error());return true;}
core::Result<HierarchyModelState> HierarchyModel::state()const{if(impl_->owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"HierarchyModel state requires its owner");return HierarchyModelState{impl_->revision,impl_->data.rows.size(),impl_->options.editable,impl_->closed};}
core::Result<HierarchyModelStats> HierarchyModel::stats()const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());return impl_->stats;}
core::Result<std::span<const HierarchyRow>> HierarchyModel::rows()const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());return std::span<const HierarchyRow>(impl_->data.rows);}
core::Result<std::reference_wrapper<const HierarchyRow>> HierarchyModel::row(std::size_t i)const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());if(i>=impl_->data.rows.size())return core::fail(core::ErrorCode::out_of_range,"Hierarchy row index is outside the model");return std::cref(impl_->data.rows[i]);}
core::Result<std::reference_wrapper<const HierarchyRow>> HierarchyModel::row(std::string_view key)const{auto i=index_of_key(key);if(!i)return std::unexpected(i.error());return row(*i);}
core::Result<std::size_t> HierarchyModel::index_of_key(std::string_view key)const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());auto i=impl_->data.keys.find(key);if(i==impl_->data.keys.end())return core::fail(core::ErrorCode::not_found,"Hierarchy key was not found");return i->second;}
core::Result<std::size_t> HierarchyModel::index_of_id(std::int64_t id)const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());auto i=impl_->data.ids.find(id);if(i==impl_->data.ids.end())return core::fail(core::ErrorCode::not_found,"Hierarchy ID was not found");return i->second;}
core::Result<HierarchyRelation> HierarchyModel::relation(std::size_t i)const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());if(i>=impl_->data.relations.size())return core::fail(core::ErrorCode::out_of_range,"Hierarchy relation index is outside the model");return impl_->data.relations[i];}
core::Result<HierarchyMetadata> HierarchyModel::metadata(std::size_t i)const{
    if(auto r=impl_->ready();!r)return std::unexpected(r.error());if(i>=impl_->data.rows.size())return core::fail(core::ErrorCode::out_of_range,"Hierarchy metadata index is outside the model");
    return protect<HierarchyMetadata>([&]()->core::Result<HierarchyMetadata>{
        ++impl_->stats.metadata_queries;const auto& relation=impl_->data.relations[i];const auto& row=impl_->data.rows[i];HierarchyMetadata out;
        out.flat_index=i;out.parent_index=relation.parent;out.sibling_index=relation.sibling_index;out.sibling_count=relation.sibling_count;out.child_count=relation.child_count;out.descendant_count=relation.subtree_end-i-1;out.has_children=relation.child_count||row.has_child_items;
        std::vector<std::size_t> ancestors;for(auto p=relation.parent;p!=hierarchy_npos;p=impl_->data.relations[p].parent)ancestors.push_back(p);std::reverse(ancestors.begin(),ancestors.end());
        std::size_t bytes=sizeof(HierarchyMetadata);const auto admit=[&](std::size_t n){if(n>impl_->options.maximum_metadata_bytes-bytes)return false;bytes+=n;return true;};
        for(auto p:ancestors){const auto& r=impl_->data.rows[p];if(!admit(r.key.size()*3+r.label.size()*5+sizeof(std::string)*8+8))return core::fail(core::ErrorCode::full,"Hierarchy path metadata exceeds its query budget");out.ancestor_keys.push_back(r.key);out.ancestor_labels.push_back(r.label);out.path_keys.push_back(r.key);out.path_labels.push_back(r.label);if(!out.path_label.empty())out.path_label+=" / ";out.path_label+=r.label;}
        out.parent_path_label=out.path_label;if(relation.parent!=hierarchy_npos){out.parent_key=impl_->data.rows[relation.parent].key;out.parent_label=impl_->data.rows[relation.parent].label;}
        if(!admit(row.key.size()+row.label.size()*2+4))return core::fail(core::ErrorCode::full,"Hierarchy row metadata exceeds its query budget");
        if(!out.path_label.empty())out.path_label+=" / ";out.path_label+=row.label;out.path_keys.push_back(row.key);out.path_labels.push_back(row.label);
        for(auto child=relation.first_child;child!=hierarchy_npos;child=impl_->data.relations[child].next_sibling){const auto& r=impl_->data.rows[child];if(!admit(r.key.size()+r.label.size()+sizeof(std::string)*4))return core::fail(core::ErrorCode::full,"Hierarchy child metadata exceeds its query budget");out.child_keys.push_back(r.key);out.child_labels.push_back(r.label);}
        return out;
    });
}
core::Result<HierarchyMovePlan> HierarchyModel::plan_move(std::string_view key,std::size_t raw,std::uint32_t depth)const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());return protect<HierarchyMovePlan>([&]{++impl_->stats.move_plans;return impl_->plan(key,raw,depth);});}
core::Result<HierarchyMovePlan> HierarchyModel::resolve_drag_target(std::string_view key,std::size_t raw,double x,double step,double padding)const{
    if(auto r=impl_->ready();!r)return std::unexpected(r.error());
    if(!std::isfinite(x)||std::abs(x)>1e15||!std::isfinite(step)||step<0||step>8192||!std::isfinite(padding)||std::abs(padding)>8192)return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy drag coordinates");
    if(step==0)step=8;const auto desired=std::clamp(std::floor((x-padding+step*.5)/step),0.,double(impl_->options.maximum_depth));return plan_move(key,raw,std::uint32_t(desired));
}
core::Result<bool> HierarchyModel::commit_move(const HierarchyMovePlan& plan){auto p=impl_;
    if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(!p->options.editable)return core::fail(core::ErrorCode::unsupported,"Hierarchy source is read-only");
    return protect<bool>([&]()->core::Result<bool>{
        if(plan.revision!=p->revision)return core::fail(core::ErrorCode::busy,"Hierarchy move plan is stale");
        auto checked=p->plan(plan.source_key,plan.raw_insertion,plan.depth);if(!checked||*checked!=plan)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy move plan does not match its source");
        const auto delta=std::int64_t(plan.depth)-p->data.rows[plan.source].depth;if(plan.source==plan.insertion&&!delta)return false;
        auto rows=p->data.rows;for(auto i=plan.source;i<plan.source_end;++i)rows[i].depth=std::uint32_t(std::int64_t(rows[i].depth)+delta);
        if(plan.raw_insertion<plan.source)std::rotate(rows.begin()+std::ptrdiff_t(plan.raw_insertion),rows.begin()+std::ptrdiff_t(plan.source),rows.begin()+std::ptrdiff_t(plan.source_end));
        else if(plan.raw_insertion>plan.source_end)std::rotate(rows.begin()+std::ptrdiff_t(plan.source),rows.begin()+std::ptrdiff_t(plan.source_end),rows.begin()+std::ptrdiff_t(plan.raw_insertion));
        auto next=p->prepare(std::move(rows));if(!next)return std::unexpected(next.error());p->data=std::move(*next);++p->revision;++p->stats.topology_builds;++p->stats.moves;
        if(auto r=p->publish({.kind=HierarchyChangeKind::move,.first=std::min(plan.source,plan.insertion),.end=std::max(plan.source_end,plan.insertion+plan.source_end-plan.source),.visibility=true,.activation=true,.move=plan});!r)return std::unexpected(r.error());return true;
    });
}
core::Result<void> HierarchyModel::observe(std::shared_ptr<const HierarchyChangePort> port){auto p=impl_;if(auto r=p->ready();!r)return r;if(!port)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy observer is null");return protect<void>([&]()->core::Result<void>{std::erase_if(p->ports,[](const auto& weak){return weak.expired();});for(const auto& weak:p->ports)if(weak.lock()==port)return {};if(p->ports.size()>=4096)return core::fail(core::ErrorCode::full,"Hierarchy observer budget exhausted");p->ports.push_back(port);return {};});}
core::Result<void> HierarchyModel::close(){auto p=impl_;if(p->owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"HierarchyModel close requires its owner");if(p->closed)return {};p->closed=true;++p->revision;auto r=protect<void>([&]{return p->publish({.kind=HierarchyChangeKind::closed});});p->ports.clear();return r;}
} // namespace lvrs
