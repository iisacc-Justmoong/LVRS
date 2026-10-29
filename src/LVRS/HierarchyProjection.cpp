#include "HierarchyProjection.h"
#include "HierarchyVisibility.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <unordered_map>
namespace lvrs {
using namespace ii;
namespace {
struct Failure final{core::Error error;};
template<class T>T take(core::Result<T> value){if(!value)throw Failure{value.error()};return std::move(*value);}
void good(core::Result<void> value){if(!value)throw Failure{value.error()};}
template<class T,class F>core::Result<T> protect(F&& f){try{return f();}catch(const Failure& e){return std::unexpected(e.error);}catch(const std::bad_alloc&){return core::fail(core::ErrorCode::full,"Hierarchy projection allocation failed");}catch(...){return core::fail(core::ErrorCode::failed,"Hierarchy projection operation failed",std::current_exception());}}
bool valid_extent(double value){return std::isfinite(value)&&value>0&&value<=8192;}
}
struct HierarchyProjection::Impl final {
    struct Override final {std::optional<bool> expanded;std::optional<double> extent;};
    std::shared_ptr<HierarchyModel> model;
    HierarchyProjectionOptions options;HierarchyProjectionState value;
    std::thread::id owner=std::this_thread::get_id();
    std::shared_ptr<HierarchyChangePort> source;
    std::vector<std::weak_ptr<const HierarchyProjectionPort>> ports;
    hierarchy_detail::VisibilityIndex index;
    std::vector<bool> expansion,collapsed;
    std::vector<double> extents;
    std::unordered_map<std::string,Override> overrides;
    std::optional<core::Error> fault;
    bool notifying=false,pending=false;
    std::size_t pending_first=hierarchy_npos,pending_end=0;
    bool pending_structure=false,pending_visibility=false;
    ~Impl(){if(source)(void)source->close();}
    core::Result<void> ready(bool write=false)const {
        if(owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"HierarchyProjection requires its owner thread");
        if(value.closed)return core::fail(core::ErrorCode::closed,"HierarchyProjection is closed");
        if(fault)return std::unexpected(*fault);
        auto state=model->state();if(!state)return std::unexpected(state.error());
        if(state->revision!=value.model_revision)return core::fail(core::ErrorCode::busy,"Hierarchy projection has not received this source revision");
        if(write&&notifying)return core::fail(core::ErrorCode::busy,"Hierarchy projection notifications are read-only");
        return {};
    }
    core::Result<void> row_ready(std::size_t i,bool write=false)const{if(auto r=ready(write);!r)return r;if(i>=value.count)return core::fail(core::ErrorCode::out_of_range,"Hierarchy projection index is outside its source");return {};}
    void totals(){value.visible_count=index.count();value.eligible_count=index.eligible_count();value.content_extent=index.extent();}
    bool hidden_by(std::size_t i,bool expanded)const{const auto& row=take(model->row(i)).get();auto relation=take(model->relation(i));return row.show_chevron&&(row.has_child_items||relation.child_count)&&!expanded;}
    void rebuild(const std::unordered_map<std::string,Override>* replacement=nullptr){
        auto state=take(model->state());auto rows=take(model->rows());
        // Includes range nodes, row vectors, temporary leaves and conservative
        // key/override admission. This is an admission estimate, not RSS.
        constexpr std::size_t fixed=4*48+128;
        if(rows.size()>options.maximum_rows||rows.size()>options.maximum_index_bytes/fixed)throw Failure{core::Error{core::ErrorCode::full,"Hierarchy projection index admission budget exceeded"}};
        std::size_t bytes=rows.size()*fixed;
        for(const auto& row:rows){if(row.key.size()>options.maximum_index_bytes-bytes)throw Failure{core::Error{core::ErrorCode::full,"Hierarchy projection key admission budget exceeded"}};bytes+=row.key.size();}
        std::vector<bool> next_expansion(rows.size()),next_collapsed(rows.size());
        std::vector<double> next_extents(rows.size());
        std::vector<hierarchy_detail::VisibilityIndex::Leaf> leaves(rows.size());
        std::vector<std::size_t> ends;std::unordered_map<std::string,Override> retained;
        const auto& selected=replacement?*replacement:overrides;
        for(std::size_t i=0;i<rows.size();++i){
            while(!ends.empty()&&ends.back()<=i)ends.pop_back();
            auto found=selected.find(rows[i].key);const auto entry=found==selected.end()?Override{}:found->second;
            if(found!=selected.end())retained.emplace(rows[i].key,entry);
            next_expansion[i]=entry.expanded.value_or(rows[i].expanded);next_extents[i]=entry.extent.value_or(options.row_extent);
            const auto relation=take(model->relation(i));next_collapsed[i]=rows[i].show_chevron&&(rows[i].has_child_items||relation.child_count)&&!next_expansion[i];
            leaves[i]={int(ends.size()),next_extents[i],rows[i].enabled&&rows[i].activatable};
            if(next_collapsed[i]&&relation.subtree_end>i+1)ends.push_back(relation.subtree_end);
        }
        hierarchy_detail::VisibilityIndex next;next.reset(leaves);
        index=std::move(next);expansion=std::move(next_expansion);collapsed=std::move(next_collapsed);extents=std::move(next_extents);overrides=std::move(retained);
        value.count=rows.size();value.model_revision=state.revision;++value.topology_resets;fault.reset();value.faulted=false;totals();
    }
    core::Result<void> publish(std::size_t first,std::size_t end,bool structure,bool visibility){
        pending=true;pending_first=std::min(pending_first,first);pending_end=std::max(pending_end,end);pending_structure|=structure;pending_visibility|=visibility;
        if(notifying)return {};
        notifying=true;struct Reset{bool& b;~Reset(){b=false;}}reset{notifying};
        bool failed=false;std::exception_ptr cause;
        for(unsigned round=0;pending&&round<8;++round){
            HierarchyProjectionChange change{value,pending_first,pending_end,pending_structure,pending_visibility};
            pending=false;pending_first=hierarchy_npos;pending_end=0;pending_structure=pending_visibility=false;
            std::erase_if(ports,[](const auto& weak){return weak.expired();});const auto targets=ports;
            for(const auto& weak:targets){
                if(value.revision!=change.state.revision)break;
                if(auto port=weak.lock()){auto sent=port->send(change);if(!sent&&sent.error().code!=runtime::QueryErrorCode::receiver_expired){if(!failed)cause=sent.error().cause;failed=true;}}
            }
        }
        if(pending)return core::fail(core::ErrorCode::busy,"Hierarchy projection observation did not settle");
        if(value.closed)ports.clear();
        if(failed)return core::fail(core::ErrorCode::failed,"Committed hierarchy projection notification failed",cause);
        return {};
    }
    core::Result<void> receive(const HierarchyChange& change){
        bool structure=false;
        auto updated=protect<void>([&]()->core::Result<void>{
            if(value.closed)return {};
            value.model_revision=change.revision;
            if(change.kind==HierarchyChangeKind::closed){value.closed=true;index.reset({});value.count=0;totals();++value.revision;structure=true;return {};}
            const bool structural=fault||change.kind==HierarchyChangeKind::reset||change.kind==HierarchyChangeKind::move;
            structure=structural;
            if(structural)rebuild();
            else if(change.kind==HierarchyChangeKind::data){
                for(auto i=change.first;i<std::min(change.end,value.count);++i){
                    const auto& row=take(model->row(i)).get();
                    if(change.activation)index.eligible(i,row.enabled&&row.activatable);
                    if(change.visibility){
                        auto found=overrides.find(row.key);const bool expanded=found!=overrides.end()&&found->second.expanded?*found->second.expanded:row.expanded;
                        const bool next=hidden_by(i,expanded);
                        if(next!=collapsed[i]){auto relation=take(model->relation(i));index.add(i+1,relation.subtree_end,next?1:-1);++value.range_updates;collapsed[i]=next;}
                        expansion[i]=expanded;
                    }
                }
                totals();
            }
            ++value.revision;return {};
        });
        if(!updated){fault=updated.error();value.faulted=true;return updated;}
        return publish(change.first,change.end,structure,change.visibility||value.closed);
    }
    bool expand(std::size_t i,bool expanded){
        if(expansion[i]==expanded)return false;
        const auto& row=take(model->row(i)).get();auto& entry=overrides[row.key];entry.expanded=expanded;
        const bool next=hidden_by(i,expanded);
        if(next!=collapsed[i]){auto relation=take(model->relation(i));index.add(i+1,relation.subtree_end,next?1:-1);++value.range_updates;}
        expansion[i]=expanded;collapsed[i]=next;return true;
    }
    core::Result<bool> expand_many(std::span<const std::size_t> indices,bool expanded){
        return protect<bool>([&]()->core::Result<bool>{
            // Admit overrides before changing the range index, so allocation
            // failure cannot leave a partially applied visibility update.
            for(auto i:indices)overrides.try_emplace(take(model->row(i)).get().key);
            bool changed=false;std::size_t first=hierarchy_npos,end=0;
            for(auto i:indices)if(expand(i,expanded)){changed=true;first=std::min(first,i);end=std::max(end,take(model->relation(i)).subtree_end);}
            if(!changed)return false;totals();++value.revision;good(publish(first,end,false,true));return true;
        });
    }
};
HierarchyProjection::HierarchyProjection(std::shared_ptr<Impl> p):impl_(std::move(p)){}
HierarchyProjection::~HierarchyProjection(){(void)close();}
core::Result<bool> HierarchyProjection::set_default_row_extent(double extent){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(!valid_extent(extent))return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy default extent");if(extent==p->options.row_extent)return false;const auto previous=p->options.row_extent;p->options.row_extent=extent;auto built=protect<void>([&]()->core::Result<void>{p->rebuild();return {};});if(!built){p->options.row_extent=previous;return std::unexpected(built.error());}++p->value.revision;auto sent=p->publish(0,p->value.count,false,false);if(!sent)return std::unexpected(sent.error());return true;}
core::Result<std::shared_ptr<HierarchyProjection>> HierarchyProjection::create(std::shared_ptr<HierarchyModel> model,HierarchyProjectionOptions options){
    if(!model||!valid_extent(options.row_extent)||!options.maximum_rows||options.maximum_rows>4000000||options.maximum_index_bytes<4096||options.maximum_index_bytes>std::size_t(1024)*1024*1024)return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy projection options");
    return protect<std::shared_ptr<HierarchyProjection>>([&]()->core::Result<std::shared_ptr<HierarchyProjection>>{
        auto state=take(model->state());if(state.closed)return core::fail(core::ErrorCode::closed,"Hierarchy projection source is closed");
        auto p=std::make_shared<Impl>();p->model=std::move(model);p->options=options;p->rebuild();auto weak=std::weak_ptr(p);
        p->source=std::make_shared<HierarchyChangePort>([weak](const auto& change){if(auto p=weak.lock()){auto received=p->receive(change);if(!received)throw std::runtime_error(received.error().message);}});
        good(p->model->observe(p->source));return std::shared_ptr<HierarchyProjection>(new HierarchyProjection(std::move(p)));
    });
}
core::Result<HierarchyProjectionState> HierarchyProjection::state()const{auto p=impl_;if(p->owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Hierarchy projection state requires its owner");return p->value;}
core::Result<bool> HierarchyProjection::expanded(std::size_t i)const{if(auto r=impl_->row_ready(i);!r)return std::unexpected(r.error());return impl_->expansion[i];}
core::Result<bool> HierarchyProjection::visible(std::size_t i)const{if(auto r=impl_->row_ready(i);!r)return std::unexpected(r.error());return impl_->index.visible(i);}
core::Result<bool> HierarchyProjection::eligible(std::size_t i)const{if(auto r=impl_->row_ready(i);!r)return std::unexpected(r.error());return impl_->index.visible(i)&&impl_->index.prefix_count(i+1,true)>impl_->index.prefix_count(i,true);}
core::Result<std::size_t> HierarchyProjection::visible_rank(std::size_t i)const{if(auto r=impl_->row_ready(i);!r)return std::unexpected(r.error());if(!impl_->index.visible(i))return core::fail(core::ErrorCode::not_found,"Hierarchy row is hidden");return impl_->index.prefix_count(i);}
core::Result<std::size_t> HierarchyProjection::eligible_rank(std::size_t i)const{auto r=eligible(i);if(!r)return std::unexpected(r.error());if(!*r)return core::fail(core::ErrorCode::not_found,"Hierarchy row is not eligible");return impl_->index.prefix_count(i,true);}
core::Result<std::size_t> HierarchyProjection::at_visible(std::size_t rank)const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());auto i=impl_->index.at(rank);if(i==hierarchy_npos)return core::fail(core::ErrorCode::out_of_range,"Hierarchy visible rank is outside the view");return i;}
core::Result<std::size_t> HierarchyProjection::at_eligible(std::size_t rank)const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());auto i=impl_->index.at(rank,true);if(i==hierarchy_npos)return core::fail(core::ErrorCode::out_of_range,"Hierarchy eligible rank is outside the view");return i;}
core::Result<std::size_t> HierarchyProjection::at_offset(double y)const{if(auto r=impl_->ready();!r)return std::unexpected(r.error());if(!std::isfinite(y))return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy offset");auto i=impl_->index.at_offset(y);if(i==hierarchy_npos)return core::fail(core::ErrorCode::out_of_range,"Hierarchy offset is outside the content");return i;}
core::Result<double> HierarchyProjection::offset(std::size_t i)const{if(auto r=impl_->row_ready(i);!r)return std::unexpected(r.error());return impl_->index.prefix_extent(i);}
core::Result<double> HierarchyProjection::row_extent(std::size_t i)const{if(auto r=impl_->row_ready(i);!r)return std::unexpected(r.error());return impl_->extents[i];}
core::Result<HierarchyMetadata> HierarchyProjection::metadata(std::size_t i)const{
    if(auto r=impl_->row_ready(i);!r)return std::unexpected(r.error());
    auto out=impl_->model->metadata(i);if(!out)return out;out->visible=impl_->index.visible(i);
    if(out->visible){out->visible_index=impl_->index.prefix_count(i);out->visible_sibling_index=out->sibling_index;out->visible_sibling_count=out->sibling_count;out->visible_child_count=impl_->collapsed[i]?0:out->child_count;}
    const auto relation=impl_->model->relation(i);if(!relation)return std::unexpected(relation.error());
    out->visible_descendant_count=impl_->index.prefix_count(relation->subtree_end)-impl_->index.prefix_count(i+1);return out;
}
core::Result<bool> HierarchyProjection::set_expanded(std::size_t i,bool value){auto p=impl_;if(auto r=p->row_ready(i,true);!r)return std::unexpected(r.error());return protect<bool>([&]()->core::Result<bool>{if(!p->expand(i,value))return false;p->totals();++p->value.revision;good(p->publish(i,take(p->model->relation(i)).subtree_end,false,true));return true;});}
core::Result<bool> HierarchyProjection::expand_ancestors(std::size_t i){auto p=impl_;if(auto r=p->row_ready(i,true);!r)return std::unexpected(r.error());return protect<bool>([&]()->core::Result<bool>{std::vector<std::size_t> ancestors;for(auto parent=take(p->model->relation(i)).parent;parent!=hierarchy_npos;parent=take(p->model->relation(parent)).parent)if(!p->expansion[parent])ancestors.push_back(parent);return p->expand_many(ancestors,true);});}
core::Result<bool> HierarchyProjection::expand_all(){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());return protect<bool>([&]()->core::Result<bool>{auto next=p->overrides;bool changed=false;for(std::size_t i=0;i<p->value.count;++i)if(!p->expansion[i]&&p->hidden_by(i,false)){next[take(p->model->row(i)).get().key].expanded=true;changed=true;}if(!changed)return false;p->rebuild(&next);++p->value.revision;good(p->publish(0,p->value.count,false,true));return true;});}
core::Result<bool> HierarchyProjection::collapse_all(bool keep_root){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());return protect<bool>([&]()->core::Result<bool>{
    // Explicit bulk operation rebuilds once, rather than N range updates.
    auto next=p->overrides;bool changed=false;for(std::size_t i=0;i<p->value.count;++i){const auto& row=take(p->model->row(i)).get();const bool expanded=keep_root&&row.depth==0;if(p->expansion[i]!=expanded){next[row.key].expanded=expanded;changed=true;}}
    if(!changed)return false;p->rebuild(&next);++p->value.revision;good(p->publish(0,p->value.count,false,true));return true;
});}
core::Result<bool> HierarchyProjection::set_row_extent(std::size_t i,double value){auto p=impl_;if(auto r=p->row_ready(i,true);!r)return std::unexpected(r.error());if(!valid_extent(value))return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy row extent");return protect<bool>([&]()->core::Result<bool>{if(p->extents[i]==value)return false;p->overrides[take(p->model->row(i)).get().key].extent=value;p->extents[i]=value;p->index.extent(i,value);p->totals();++p->value.revision;good(p->publish(i,i+1,false,true));return true;});}
core::Result<void> HierarchyProjection::observe(std::shared_ptr<const HierarchyProjectionPort> port){auto p=impl_;if(auto r=p->ready();!r)return r;if(!port)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy projection observer is null");return protect<void>([&]()->core::Result<void>{std::erase_if(p->ports,[](const auto& w){return w.expired();});for(const auto& w:p->ports)if(w.lock()==port)return {};if(p->ports.size()>=4096)return core::fail(core::ErrorCode::full,"Hierarchy projection observer budget exhausted");p->ports.push_back(port);return {};});}
core::Result<void> HierarchyProjection::close(){auto p=impl_;if(p->owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Hierarchy projection close requires its owner");if(p->value.closed)return {};return protect<void>([&]()->core::Result<void>{p->value.closed=true;if(p->source)(void)p->source->close();p->index.reset({});p->value.count=0;p->totals();++p->value.revision;return p->publish(0,0,true,true);});}
} // namespace lvrs
