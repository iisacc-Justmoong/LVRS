#include "HierarchyToolbar.h"
#include "ToolbarDetail.h"
#include "SurfaceDetail.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace lvrs {
using namespace ii;
namespace {
bool valid(const HierarchyToolbarOptions& o){for(auto n:{o.minimum_width,o.horizontal_padding,o.vertical_padding,o.spacing,o.slot_size,o.width,o.height})if(!surface_detail::metric(n))return false;return o.slot_size>=1&&o.horizontal_padding*2<=8192&&o.vertical_padding*2+o.slot_size<=8192&&surface_detail::color(o.background)&&std::isfinite(o.background_opacity)&&o.background_opacity>=0&&o.background_opacity<=1;}
ToolbarId entry_id(const HierarchyToolbarEntry& item,std::size_t index){return item.button_id.value_or(ToolbarId{std::int64_t(index)});}
void raise(const core::Error& e){if(e.cause)std::rethrow_exception(e.cause);throw std::runtime_error(e.message);}
}
struct HierarchyToolbar::Impl final:std::enable_shared_from_this<Impl> {
    HierarchyToolbarOptions options;HierarchyToolbarEvents events;
    gui::Widget root,manual,host,model_row;
    std::vector<ToolbarButton> manual_buttons,model_buttons;
    std::vector<HierarchyToolbarEntry> entries;
    ToolbarSelection current;
    std::optional<ToolbarId> requested_id;
    std::string id;std::uint64_t generation=0,revision=0;double effective_spacing=0;bool updating=false;unsigned dispatch_depth=0;
    const std::vector<ToolbarButton>& buttons()const{return entries.empty()?manual_buttons:model_buttons;}
    bool live()const{return root.snapshot().has_value();}
    core::Result<void> normalize(){
        if(!live())return core::fail(core::ErrorCode::closed,"HierarchyToolbar scope retired");
        const auto all=buttons();int target=-1;
        const auto enabled=[&](int i){auto o=all[i].options();return o&&o->enabled&&o->tone!=gui::ButtonTone::disabled;};
        const auto identity=requested_id.value_or(current.button_id);
        for(std::size_t i=0;i<all.size();++i){auto o=all[i].options();if(o&&o->button_id==identity){target=int(i);break;}}
        if(target<0){auto selected=current.button.placement();if(selected)for(std::size_t i=0;i<all.size();++i){auto place=all[i].root().placement();if(place&&place->scope.lock()==selected->scope.lock()&&all[i].root().snapshot()->id==current.button.snapshot()->id){target=int(i);break;}}}
        if(target<0||!enabled(target)){
            target=-1;if(!entries.empty())for(std::size_t i=0;i<entries.size();++i)if(entries[i].selected&&enabled(int(i))){target=int(i);break;}
            if(target<0)for(std::size_t i=0;i<all.size();++i)if(enabled(int(i))){target=int(i);break;}
            if(target<0&&!all.empty())target=0;
        }
        current={};if(target>=0){auto o=all[target].options();if(!o)return std::unexpected(o.error());current={all[target].root(),o->button_id,target};}
        requested_id.reset();for(std::size_t i=0;i<all.size();++i)if(auto r=all[i].set_active(int(i)==target);!r)return r;
        return {};
    }
    core::Result<void> geometry(gui::Rect bounds){
        auto color=options.background;color.alpha*=options.background_opacity;if(auto r=surface_detail::paint(root,bounds,color,0);!r)return r;
        const auto all=buttons();std::size_t visible=0;double consumed=0;
        for(const auto& b:all){auto o=b.options();if(!o)return std::unexpected(o.error());if(!o->visible)continue;++visible;auto s=b.root().snapshot();if(!s)return std::unexpected(s.error());consumed+=entries.empty()?s->implicit_size.width:options.slot_size;}
        const double inner=std::max(0.,double(bounds.width)-2*options.horizontal_padding);
        effective_spacing=options.distribute_spacing?(visible>1?std::max(0.,(inner-consumed)/(visible-1)):0):options.spacing;
        const gui::StackOptions row_options{.spacing=effective_spacing,.alignment=gui::StackAlignment::center};
        if(auto r=manual.set_stack_options(row_options);!r)return r;
        if(model_row.snapshot()){if(auto r=model_row.set_stack_options(row_options);!r)return r;if(auto r=model_row.set_extent({inner,options.slot_size});!r)return r;}
        return {};
    }
    core::Result<void> configure(){
        const auto& o=options;
        if(auto r=root.set_stack_options({.spacing=0,.alignment=gui::StackAlignment::stretch,.insets=gui::Insets{o.horizontal_padding,o.vertical_padding,o.horizontal_padding,o.vertical_padding},.minimum_width=o.minimum_width,.minimum_height=o.slot_size+2*o.vertical_padding,.intrinsic_width=true});!r)return r;
        if(auto r=root.set_extent({o.width,o.height});!r)return r;
        if(auto r=root.set_enabled(o.enabled);!r)return r;
        if(auto r=manual.set_visible(entries.empty());!r)return r;if(auto r=host.set_visible(!entries.empty());!r)return r;
        const auto width=entries.size()*o.slot_size+(o.distribute_spacing?0:std::max(0,int(entries.size())-1)*o.spacing);
        if(auto r=host.set_extent({width,o.slot_size});!r)return r;
        auto snapshot=root.snapshot();if(!snapshot)return std::unexpected(snapshot.error());return geometry(snapshot->bounds);
    }
    core::Result<bool> trigger(int index){
        if(!live())return core::fail(core::ErrorCode::closed,"HierarchyToolbar scope retired");
        if(updating)return core::fail(core::ErrorCode::busy,"HierarchyToolbar model is committing");
        const auto all=buttons();if(index<0||std::size_t(index)>=all.size())return false;
        auto options=all[index].options();if(!options)return std::unexpected(options.error());auto source=all[index].root().snapshot();if(!source)return std::unexpected(source.error());
        if(!this->options.enabled||!options->enabled||options->tone==gui::ButtonTone::disabled||!source->enabled)return false;
        if(dispatch_depth==16)return core::fail(core::ErrorCode::busy,"HierarchyToolbar nested activation limit exceeded");
        struct Guard{unsigned& n;explicit Guard(unsigned& v):n(v){++n;}~Guard(){--n;}}guard{dispatch_depth};
        const bool changed=current.index!=index;current={all[index].root(),options->button_id,index};requested_id.reset();
        for(std::size_t i=0;i<all.size();++i)if(auto r=all[i].set_active(int(i)==index);!r)return std::unexpected(r.error());
        auto event_handlers=events;const auto epoch=revision;const auto entry=entries.empty()?HierarchyToolbarEntry{}:entries[index];
        ToolbarTrigger context{.button=current.button,.button_id=current.button_id,.index=index,.item=entry.item};
        if(!entry.events.empty()){context.event_name=entry.events.front().name;context.payload=entry.events.front().payload;}
        const auto weak=weak_from_this();context.emit=[weak,epoch,context](std::string_view name,core::Value payload)->core::Result<void>{
            auto p=weak.lock();if(!p||!p->live()||p->revision!=epoch)return core::fail(core::ErrorCode::closed,"Toolbar event context retired");
            if(!surface_detail::text(name))return core::fail(core::ErrorCode::invalid_argument,"Invalid toolbar event name");auto cost=toolbar_detail::value_cost(payload);if(!cost)return std::unexpected(cost.error());if(name.empty())return {};
            auto callback=p->events.button_event_triggered;try{if(callback)callback({std::string(name),std::move(payload)},context);}catch(...){return core::fail(core::ErrorCode::failed,"Toolbar event callback failed",std::current_exception());}return {};
        };
        context.activate=[weak,epoch](int next)->core::Result<bool>{auto p=weak.lock();if(!p||!p->live()||p->revision!=epoch)return core::fail(core::ErrorCode::closed,"Toolbar activation context retired");return p->trigger(next);};
        const auto current_state=current;
        try{
            if(changed&&event_handlers.active_changed)event_handlers.active_changed(current_state);
            if(!live()||revision!=epoch)return true;
            if(event_handlers.button_triggered)event_handlers.button_triggered(context);
            if(!live()||revision!=epoch)return true;
            for(const auto& spec:entry.events){if(event_handlers.button_event_triggered)event_handlers.button_event_triggered(spec,context);if(!live()||revision!=epoch)return true;}
            if(entry.on_triggered)entry.on_triggered(context);
        }catch(...){return core::fail(core::ErrorCode::failed,"HierarchyToolbar callback failed after selection",std::current_exception());}
        return true;
    }
    core::Result<void> replace(std::vector<HierarchyToolbarEntry> next){
        if(updating)return core::fail(core::ErrorCode::busy,"HierarchyToolbar model is committing");
        if(next.size()>256)return core::fail(core::ErrorCode::full,"HierarchyToolbar supports at most 256 entries");
        const auto total=next.size()*options.slot_size+(options.distribute_spacing?0:std::max(0,int(next.size())-1)*options.spacing)+2*options.horizontal_padding;
        if(total>8192)return core::fail(core::ErrorCode::invalid_argument,"HierarchyToolbar model extent exceeds native bounds");
        std::size_t bytes=0;std::vector<gui::ButtonOptions> prepared;prepared.reserve(next.size());
        for(std::size_t i=0;i<next.size();++i){auto& item=next[i];item.button.button_id=entry_id(item,i);item.button.width=options.slot_size;item.button.height=options.slot_size;auto native=toolbar_detail::prepare(item.button);if(!native)return std::unexpected(native.error());prepared.push_back(std::move(*native));
            if(item.events.size()>64)return core::fail(core::ErrorCode::full,"Toolbar entry event limit is 64");auto cost=toolbar_detail::value_cost(item.item);if(!cost)return std::unexpected(cost.error());bytes+=*cost;
            for(auto& event:item.events){if(!surface_detail::text(event.name))return core::fail(core::ErrorCode::invalid_argument,"Invalid toolbar event name");auto cost=toolbar_detail::value_cost(event.payload);if(!cost)return std::unexpected(cost.error());bytes+=*cost+event.name.size();}
            if(bytes>4*1024*1024)return core::fail(core::ErrorCode::full,"Toolbar model exceeds its total payload budget");
        }
        struct Guard{bool& flag;explicit Guard(bool& f):flag(f){flag=true;}~Guard(){flag=false;}}guard{updating};
        bool same=next.size()==entries.size();for(std::size_t i=0;same&&i<next.size();++i)same=entry_id(next[i],i)==entry_id(entries[i],i);
        if(same){
            for(std::size_t i=0;i<next.size();++i)if(auto r=model_buttons[i].set_options(next[i].button);!r)return r;
            entries.swap(next);++revision;
        }else if(next.empty()){
            if(auto r=host.clear_content();!r)return r;entries.clear();model_buttons.clear();model_row={};++generation;++revision;
        }else{
            struct Candidate{std::vector<ToolbarButton> buttons;gui::Widget row;};auto built=std::make_shared<Candidate>();const auto epoch=generation+1;const auto weak=weak_from_this();
            auto result=host.set_content([built,next,weak,epoch](auto& b)->core::Result<void>{
                auto row=b.hstack({},"buttons",{.spacing=0,.alignment=gui::StackAlignment::center});if(!row)return std::unexpected(row.error());built->row=*row;
                for(std::size_t i=0;i<next.size();++i){auto button=ToolbarButton::create(b,*row,"button_"+std::to_string(i),next[i].button,[weak,epoch,index=int(i)]{if(auto p=weak.lock();p&&p->generation==epoch){auto r=p->trigger(index);if(!r)raise(r.error());}});if(!button)return std::unexpected(button.error());built->buttons.push_back(*button);}
                return {};
            },[&,built,epoch]()->core::Result<void>{entries.swap(next);model_buttons.swap(built->buttons);model_row=built->row;generation=epoch;++revision;return {};});
            if(!result)return result;
        }
        if(auto r=normalize();!r)return r;return configure();
    }
};
const std::vector<HierarchyToolbarEntry>& toolbar_items(const std::vector<HierarchyToolbarEntry>& items){if(items.size()>256)throw std::length_error("HierarchyToolbar entry limit is 256");return items;}
const std::vector<HierarchyToolbarEntry>& toolbar_items(const HierarchyToolbarItems& items){static const std::vector<HierarchyToolbarEntry> empty;return items?toolbar_items(*items):empty;}
core::Result<HierarchyToolbar> HierarchyToolbar::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,HierarchyToolbarOptions options,std::vector<HierarchyToolbarEntry> entries,HierarchyToolbarEvents events,ToolbarContentFactory content){
    if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid HierarchyToolbar options");auto p=std::make_shared<Impl>();p->options=options;p->events=std::move(events);p->id=id;
    auto root=b.vstack(parent,id);if(!root)return std::unexpected(root.error());p->root=*root;
    auto manual=b.hstack(*root,std::string(id)+"/manual");if(!manual)return std::unexpected(manual.error());p->manual=*manual;
    auto host=b.content_host(*root,std::string(id)+"/model");if(!host)return std::unexpected(host.error());p->host=*host;
    if(auto r=manual->set_clip_content(true);!r)return std::unexpected(r.error());
    if(auto r=b.retain(p);!r)return std::unexpected(r.error());const auto weak=std::weak_ptr<Impl>{p};
    if(auto r=root->set_layout_handler([weak](gui::Rect bounds){if(auto p=weak.lock())surface_detail::require(p->geometry(bounds));});!r)return std::unexpected(r.error());
    if(auto r=b.set_default_content(*root,*manual);!r)return std::unexpected(r.error());
    if(auto r=p->replace(std::move(entries));!r)return std::unexpected(r.error());
    if(content){try{auto result=content(b,*manual);if(!result)return std::unexpected(result.error());}catch(...){return core::fail(core::ErrorCode::failed,"HierarchyToolbar content factory failed",std::current_exception());}}
    if(auto r=p->normalize();!r)return std::unexpected(r.error());HierarchyToolbar result;result.root_=*root;result.manual_=*manual;result.state_=p;return result;
}
core::Result<std::shared_ptr<HierarchyToolbar::Impl>> HierarchyToolbar::lock()const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"HierarchyToolbar scope retired");return p;}
core::Result<HierarchyToolbarOptions> HierarchyToolbar::options()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options;}
core::Result<void> HierarchyToolbar::set_options(HierarchyToolbarOptions o)const{auto p=lock();if(!p)return std::unexpected(p.error());if(!valid(o))return core::fail(core::ErrorCode::invalid_argument,"Invalid HierarchyToolbar options");const auto count=(*p)->entries.size();if(count*o.slot_size+(o.distribute_spacing?0:std::max(0,int(count)-1)*o.spacing)+2*o.horizontal_padding>8192)return core::fail(core::ErrorCode::invalid_argument,"HierarchyToolbar model extent exceeds native bounds");auto old=(*p)->options;(*p)->options=o;if(old.slot_size!=o.slot_size)return (*p)->replace((*p)->entries);return (*p)->configure();}
core::Result<void> HierarchyToolbar::set_items(std::vector<HierarchyToolbarEntry> entries)const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->replace(std::move(entries));}
core::Result<std::vector<HierarchyToolbarEntry>> HierarchyToolbar::items()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->entries;}
core::Result<void> HierarchyToolbar::register_button(ToolbarButton button,bool normalize_now)const{
    auto p=lock();if(!p)return std::unexpected(p.error());auto snapshot=button.root().snapshot();if(!snapshot)return std::unexpected(snapshot.error());auto place=button.root().placement(),parent=manual_.placement();
    if(!place||!parent||place->scope.lock()!=parent->scope.lock()||snapshot->parent_id!=(*p)->id+"/manual")return core::fail(core::ErrorCode::invalid_argument,"ToolbarButton must be a direct child of the manual slot");
    for(const auto& entry:(*p)->manual_buttons)if(entry.root().snapshot()->id==snapshot->id)return {};
    if((*p)->manual_buttons.size()==256)return core::fail(core::ErrorCode::full,"Manual toolbar button limit is 256");
    auto weak=std::weak_ptr<Impl>{*p};const auto id=snapshot->id;
    if(auto r=button.attach([weak,id]{if(auto p=weak.lock();p&&p->entries.empty()){for(std::size_t i=0;i<p->manual_buttons.size();++i){auto s=p->manual_buttons[i].root().snapshot();if(s&&s->id==id){auto r=p->trigger(int(i));if(!r)raise(r.error());break;}}}},[weak]{if(auto p=weak.lock();p&&p->entries.empty())surface_detail::require(p->normalize());});!r)return r;
    (*p)->manual_buttons.push_back(button);if(normalize_now)return (*p)->normalize();return {};
}
core::Result<void> HierarchyToolbar::normalize()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->normalize();}
core::Result<void> HierarchyToolbar::set_active_id(ToolbarId id)const{auto p=lock();if(!p)return std::unexpected(p.error());if(!toolbar_detail::id_valid(id))return core::fail(core::ErrorCode::invalid_argument,"Invalid toolbar ID");(*p)->requested_id=std::move(id);return (*p)->normalize();}
core::Result<ToolbarSelection> HierarchyToolbar::selection()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->current;}
core::Result<gui::Widget> HierarchyToolbar::button_at(int index)const{auto p=lock();if(!p)return std::unexpected(p.error());const auto& buttons=(*p)->buttons();if(index<0||std::size_t(index)>=buttons.size())return core::fail(core::ErrorCode::out_of_range,"Toolbar button index is out of range");return buttons[index].root();}
core::Result<int> HierarchyToolbar::button_count()const{auto p=lock();if(!p)return std::unexpected(p.error());return int((*p)->buttons().size());}
core::Result<int> HierarchyToolbar::visible_button_count()const{auto p=lock();if(!p)return std::unexpected(p.error());int count=0;for(const auto& button:(*p)->buttons()){auto o=button.options();if(!o)return std::unexpected(o.error());if(o->visible)++count;}return count;}
core::Result<bool> HierarchyToolbar::trigger_index(int index)const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->trigger(index);}
core::Result<double> HierarchyToolbar::distributed_spacing()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->effective_spacing;}
}
