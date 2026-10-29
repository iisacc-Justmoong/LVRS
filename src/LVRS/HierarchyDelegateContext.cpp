#include "HierarchyDelegateContext.h"
#include <thread>
#include <stdexcept>
namespace lvrs {
using namespace ii;
namespace {
struct Failure final{core::Error error;};
template<class T>T take(core::Result<T> value){if(!value)throw Failure{value.error()};return std::move(*value);}
void good(core::Result<void> value){if(!value)throw Failure{value.error()};}
template<class F>auto protect(F function)->decltype(function()){try{return function();}catch(const Failure& error){return std::unexpected(error.error);}catch(...){return core::fail(core::ErrorCode::failed,"Hierarchy delegate failed",std::current_exception());}}
HierarchyItemOptions normalize(HierarchyItemOptions value,const HierarchyItemOptions& source){
    value.controller=source.controller;value.width=source.width;value.row_height=source.row_height;
    value.row.key=source.row.key;value.row.item_id=source.row.item_id;value.row.depth=source.row.depth;
    value.row.has_child_items=source.row.has_child_items;value.row.selected=source.row.selected;value.row.expanded=source.row.expanded;value.drag_preview=source.drag_preview;
    value.row.enabled&=source.row.enabled;value.row.activatable&=source.row.activatable;value.row.draggable&=source.row.draggable;return value;
}
HierarchyItemOptions merge(HierarchyItemOptions value,const HierarchyItemOptions& previous,const HierarchyItemOptions& source){
    const auto field=[&](auto member){if(value.*member==previous.*member)value.*member=source.*member;};
    field(&HierarchyItemOptions::indent_step);field(&HierarchyItemOptions::icon_size);field(&HierarchyItemOptions::chevron_size);field(&HierarchyItemOptions::left_padding);field(&HierarchyItemOptions::right_padding);field(&HierarchyItemOptions::spacing);field(&HierarchyItemOptions::corner_radius);field(&HierarchyItemOptions::count_width);field(&HierarchyItemOptions::drag_opacity);
    field(&HierarchyItemOptions::text);field(&HierarchyItemOptions::disabled_text);field(&HierarchyItemOptions::description);field(&HierarchyItemOptions::idle);field(&HierarchyItemOptions::hover);field(&HierarchyItemOptions::pressed);field(&HierarchyItemOptions::active);field(&HierarchyItemOptions::inactive);field(&HierarchyItemOptions::drag);
    const auto row=[&](auto member){if(value.row.*member==previous.row.*member)value.row.*member=source.row.*member;};
    row(&HierarchyRow::label);row(&HierarchyRow::icon_name);row(&HierarchyRow::icon_glyph);row(&HierarchyRow::icon);row(&HierarchyRow::count);row(&HierarchyRow::enabled);row(&HierarchyRow::activatable);row(&HierarchyRow::draggable);row(&HierarchyRow::show_chevron);
    return normalize(std::move(value),source);
}
}
struct HierarchyDelegateContext::Impl final {
    std::thread::id owner=std::this_thread::get_id();
    HierarchyItemOptions source,applied_source;HierarchyItemEvents inherited;
    HierarchyDelegateState value;HierarchyItem row;
    core::Observable<std::uint64_t> epoch{0};bool creating=false,updating=false;
    core::Result<void> ready(bool write=false)const{
        if(owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Hierarchy delegate requires its owner thread");
        if(value.closed)return core::fail(core::ErrorCode::closed,"Hierarchy delegate is closed");
        if(write&&(creating||updating))return core::fail(core::ErrorCode::busy,"Hierarchy delegate is constructing or publishing its source");return {};
    }
    bool live()const{return !value.closed&&bool(row.root().snapshot());}
};
core::Result<HierarchyDelegateContext> HierarchyDelegateContext::create(HierarchyItemOptions options,HierarchyItemEvents events){
    if(auto r=HierarchyItem::validate_options(options);!r)return std::unexpected(r.error());
    if(options.row.key.empty())return core::fail(core::ErrorCode::invalid_argument,"Hierarchy delegate requires a stable row key");
    return protect([&]()->core::Result<HierarchyDelegateContext>{HierarchyDelegateContext result;result.state_=std::make_shared<Impl>();result.state_->source=result.state_->applied_source=std::move(options);result.state_->inherited=std::move(events);return result;});
}
core::Result<HierarchyItemOptions> HierarchyDelegateContext::options()const{auto p=state_;if(!p)return core::fail(core::ErrorCode::closed,"Hierarchy delegate has no source");if(auto r=p->ready();!r)return std::unexpected(r.error());(void)p->epoch.get();return p->source;}
core::Result<HierarchyDelegateState> HierarchyDelegateContext::state()const{auto p=state_;if(!p)return core::fail(core::ErrorCode::closed,"Hierarchy delegate has no source");if(p->owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Hierarchy delegate requires its owner thread");if(!p->value.closed)(void)p->epoch.get();return p->value;}
core::Result<HierarchyItem> HierarchyDelegateContext::item()const{auto p=state_;if(!p)return core::fail(core::ErrorCode::closed,"Hierarchy delegate has no source");if(auto r=p->ready();!r)return std::unexpected(r.error());if(!p->value.has_item)return core::fail(core::ErrorCode::not_found,"Hierarchy delegate has not produced an item");if(auto r=p->row.root().snapshot();!r)return std::unexpected(r.error());return p->row;}
core::Result<HierarchyItem> HierarchyDelegateContext::create_item(gui::WidgetBuilder& widgets,gui::Widget parent,std::string_view id,HierarchyItemOptions options,HierarchyItemEvents events,HierarchyCountFactory count)const{
    auto p=state_;if(!p)return core::fail(core::ErrorCode::closed,"Hierarchy delegate has no source");if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->value.has_item)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy delegate must produce exactly one item");
    return protect([&]()->core::Result<HierarchyItem>{
        p->creating=true;struct Reset{bool& value;~Reset(){value=false;}}reset{p->creating};auto weak=std::weak_ptr(p);
        good(widgets.retain(p));
        good(widgets.on_retire([weak]{if(auto p=weak.lock())p->value.closed=true;}));
        const auto signal=[weak](auto inherited,auto own,auto... args){auto owner=weak.lock();if(!owner||owner->value.closed)return;std::exception_ptr failed;try{if(inherited)inherited(args...);}catch(...){failed=std::current_exception();}if(owner->live())try{if(own)own(args...);}catch(...){if(!failed)failed=std::current_exception();}if(failed)std::rethrow_exception(failed);};
        HierarchyItemEvents combined{.activated=[signal,inherited=p->inherited.activated,own=events.activated]{signal(inherited,own);},.expansion_changed=[signal,inherited=p->inherited.expansion_changed,own=events.expansion_changed](bool value){signal(inherited,own,value);},.pointer=[weak,inherited=p->inherited.pointer,own=events.pointer](const auto& input)->core::Result<bool>{auto p=weak.lock();if(!p||p->value.closed)return false;bool consumed=false;if(inherited)consumed=take(inherited(input));if(p->live()&&own)consumed=take(own(input))||consumed;return consumed;}};
        auto result=take(HierarchyItem::create(widgets,parent,id,normalize(std::move(options),p->source),std::move(combined),std::move(count)));
        p->row=result;p->value.has_item=true;p->value.applied_revision=p->value.revision;p->applied_source=p->source;return result;
    });
}
core::Result<void> HierarchyDelegateContext::apply(HierarchyItemOptions options)const{
    auto p=state_;if(!p)return core::fail(core::ErrorCode::closed,"Hierarchy delegate has no source");if(auto r=p->ready();!r)return r;if(p->creating)return core::fail(core::ErrorCode::busy,"Hierarchy delegate is constructing its item");if(!p->value.has_item)return core::fail(core::ErrorCode::not_found,"Hierarchy delegate has no item");
    return core::ObservableReadScope::pause([&]()->core::Result<void>{return protect([&]()->core::Result<void>{good(p->row.set_options(normalize(std::move(options),p->source)));p->value.applied_revision=p->value.revision;p->applied_source=p->source;return {};});});
}
core::Result<void> HierarchyDelegateContext::adopt_legacy(HierarchyItem item)const{
    auto p=state_;if(!p)return core::fail(core::ErrorCode::closed,"Hierarchy delegate has no source");if(auto r=p->ready(true);!r)return r;if(p->value.has_item)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy delegate already has an item");auto value=item.options();if(!value)return std::unexpected(value.error());if(value->row.key!=p->source.row.key)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy delegate changed its row key");p->row=std::move(item);p->value.has_item=p->value.legacy=true;return apply(std::move(*value));
}
core::Result<bool> HierarchyDelegateContext::update_source(HierarchyItemOptions source)const{
    auto p=state_;if(!p)return core::fail(core::ErrorCode::closed,"Hierarchy delegate has no source");if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(auto r=HierarchyItem::validate_options(source);!r)return std::unexpected(r.error());if(source.row.key!=p->source.row.key)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy delegate source keys cannot change");
    if(source==p->source&&p->value.applied_revision==p->value.revision)return false;
    return core::ObservableReadScope::pause([&]()->core::Result<bool>{return protect([&]()->core::Result<bool>{
        if(p->value.revision==UINT64_MAX)return core::fail(core::ErrorCode::full,"Hierarchy delegate revision exhausted");
        p->updating=true;struct Reset{bool& value;~Reset(){value=false;}}reset{p->updating};p->source=std::move(source);++p->value.revision;
        if(p->value.legacy&&p->value.has_item){auto current=take(p->row.options());good(apply(merge(std::move(current),p->applied_source,p->source)));}
        p->epoch.set(p->value.revision);
        if(p->value.has_item&&p->value.applied_revision!=p->value.revision)return core::fail(core::ErrorCode::failed,"Hierarchy delegate did not apply the source revision");return true;
    });});
}
core::Result<void> HierarchyDelegateContext::close()const{auto p=state_;if(!p)return {};if(p->owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Hierarchy delegate requires its owner thread");p->value.closed=true;return {};}
}
