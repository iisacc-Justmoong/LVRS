#include <LVRS/Card.h>
#include <LVRS/CardDetail.h>
#include <LVRS/SurfaceDetail.h>
#include <algorithm>
#include <stdexcept>
namespace lvrs {
using namespace ii;
namespace {
void raise(const core::Error& e){if(e.cause)std::rethrow_exception(e.cause);throw std::runtime_error(e.message);}
std::string description(const CardOptions& o){std::string result;for(const auto* s:{&o.description,&o.details,&o.metadata,&o.status_text})if(!s->empty()){if(!result.empty())result+=". ";result+=*s;}return result;}
}
const gui::WidgetFactory& card_component(const gui::WidgetFactory& factory){return factory;}
const gui::WidgetFactory& card_component(const CardComponent& factory){static const gui::WidgetFactory empty;return factory?*factory:empty;}
struct Card::Impl final {
    CardOptions options;CardEvents events;card_detail::Prepared prepared;gui::WidgetAppearance appearance;
    gui::Widget root,chrome,base,overlay,preview,inline_preview,icon,menu,action;
    bool preview_custom=false,icon_custom=false,configuring=false;std::uint64_t revision=1;
    struct Cache{std::optional<gui::WidgetPaintState> state;std::uint64_t revision=0;std::shared_ptr<const gui::WidgetDrawing> drawing;};Cache background_cache,foreground_cache;
    core::Result<std::shared_ptr<const gui::WidgetDrawing>> paint(const gui::WidgetPaintState& state,bool foreground){
        auto& cache=foreground?foreground_cache:background_cache;
        if(cache.revision==revision&&cache.state==state)return cache.drawing;
        const auto custom=preview_custom||state.content_children>0;auto visual=card_detail::resolve(options,prepared,state,custom);auto layout=card_detail::layout(options,prepared,visual,state.size);
        cache.drawing=foreground?card_detail::foreground(options,prepared,visual,layout,state.enabled):card_detail::background(options,prepared,visual,layout,custom,icon_custom,state.enabled);cache.state=state;cache.revision=revision;return cache.drawing;
    }
    core::Result<void> place(gui::Rect bounds){
        auto native=root.action_options();if(!native)return std::unexpected(native.error());const gui::WidgetPaintState ps{{bounds.width,bounds.height},options.enabled,false,false,false,native->checked};
        const auto visual=card_detail::resolve(options,prepared,ps,preview_custom);const auto layout=card_detail::layout(options,prepared,visual,ps.size);
        if(bounds.width<=0||bounds.height<=0)return {};
        if(auto r=chrome.set_extent({bounds.width,bounds.height});!r)return r;
        for(auto widget:{base,overlay,preview,inline_preview})if(auto r=widget.set_frame({0,0,bounds.width,bounds.height});!r)return r;
        if(auto r=icon.set_frame(layout.icon);!r)return r;if(auto r=menu.set_frame(layout.menu);!r)return r;return action.set_frame(layout.action);
    }
    core::Result<void> configure(){
        const gui::WidgetPaintState state{{0,0},options.enabled,false,false,false,options.selected};const auto visual=card_detail::resolve(options,prepared,state,preview_custom);
        gui::ActionOptions native{.accessibility_name=options.title,.enabled=options.enabled,.focus_ring=false,.width=options.width?options.width:visual.implicit_size.width,.height=options.height?options.height:visual.implicit_size.height,.checkable=options.selectable.value_or(!visual.preview_only),.checked=options.selected,.tab_focusable=!visual.preview_only,.pointer_focusable=!visual.preview_only,.accessibility_description=description(options)};
        if(auto r=root.set_action_options(std::move(native));!r)return r;
        if(auto r=preview.set_visible(visual.image_card&&preview_custom);!r)return r;if(auto r=inline_preview.set_visible(visual.image_card&&!preview_custom);!r)return r;if(auto r=icon.set_visible(!visual.image_card&&icon_custom);!r)return r;
        if(auto r=menu.set_visible(options.show_menu.value_or(!visual.image_card)&&!visual.preview_only);!r)return r;
        if(auto r=action.set_visible(options.show_action.value_or(true)&&!visual.image_card);!r)return r;
        if(auto r=menu.set_button_options({.text="",.tone=gui::ButtonTone::borderless,.icon=prepared.more,.icon_name="card/more",.accessibility_name=options.menu_accessibility_name,.icon_size=18,.icon_mode=true,.height=visual.image_card?28:22,.corner_radius=6,.left_padding=2,.right_padding=2,.indicator_size=18,.width=visual.image_card?28:22});!r)return r;
        if(auto r=action.set_button_options({.text=prepared.action_text,.tone=gui::ButtonTone::normal,.height=22,.horizontal_padding=8});!r)return r;
        ++revision;auto s=root.snapshot();if(!s)return std::unexpected(s.error());if(auto r=place(s->bounds);!r)return r;if(auto r=base.request_repaint();!r)return r;return overlay.request_repaint();
    }
};
core::Result<Card> Card::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,CardOptions options,CardEvents events,CardContent content){
    auto appearance=b.appearance();if(!appearance)return std::unexpected(appearance.error());auto prepared=card_detail::prepare(options,*appearance);if(!prepared)return std::unexpected(prepared.error());auto p=std::make_shared<Impl>();p->options=std::move(options);p->events=std::move(events);p->prepared=std::move(*prepared);p->appearance=*appearance;std::weak_ptr<Impl> weak=p;
    auto root=b.action_surface(parent,id,{},[weak]{
        auto p=weak.lock();if(!p)return;auto native=p->root.action_options();if(!native)return;const auto selected=native->checked,changed=p->options.selected!=selected;p->options.selected=selected;
        auto changed_callback=p->events.selected_changed;auto clicked=p->events.clicked;if(auto r=p->configure();!r){if(r.error().code==core::ErrorCode::closed)return;raise(r.error());}
        if(changed&&changed_callback)changed_callback(selected);if(p->root.snapshot()&&clicked)clicked();
    });if(!root)return std::unexpected(root.error());p->root=*root;
    const gui::WidgetPaintState initial{{0,0},p->options.enabled,false,false,false,p->options.selected};auto visual=card_detail::resolve(p->options,p->prepared,initial);const gui::Size size{p->options.width?p->options.width:visual.implicit_size.width,p->options.height?p->options.height:visual.implicit_size.height};
    auto chrome=b.absolute_stack(*root,std::string(id)+"/chrome",size);if(!chrome)return std::unexpected(chrome.error());p->chrome=*chrome;
    auto base=b.absolute_stack(*chrome,std::string(id)+"/surface",size);if(!base)return std::unexpected(base.error());p->base=*base;
    auto preview=b.content_host(*chrome,std::string(id)+"/preview",size);if(!preview)return std::unexpected(preview.error());p->preview=*preview;
    auto inline_preview=b.absolute_stack(*chrome,std::string(id)+"/inline",size);if(!inline_preview)return std::unexpected(inline_preview.error());p->inline_preview=*inline_preview;
    auto overlay=b.absolute_stack(*chrome,std::string(id)+"/overlay",size);if(!overlay)return std::unexpected(overlay.error());p->overlay=*overlay;
    auto icon=b.content_host(*chrome,std::string(id)+"/icon",{18,18});if(!icon)return std::unexpected(icon.error());p->icon=*icon;
    auto menu=b.button(*chrome,std::string(id)+"/menu",{},[weak]{if(auto p=weak.lock()){auto callback=p->events.menu_requested;if(callback)callback();}});if(!menu)return std::unexpected(menu.error());p->menu=*menu;
    auto action=b.button(*chrome,std::string(id)+"/action",{},[weak]{if(auto p=weak.lock()){auto callback=p->events.action_triggered;if(callback)callback();}});if(!action)return std::unexpected(action.error());p->action=*action;
    if(auto r=b.retain(p);!r)return std::unexpected(r.error());if(auto r=root->set_clip_content(true);!r)return std::unexpected(r.error());
    if(auto r=b.set_default_content(*root,*inline_preview);!r)return std::unexpected(r.error());
    if(auto r=base->set_paint_handler([weak](const auto& state)->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{auto p=weak.lock();if(!p)return core::fail(core::ErrorCode::closed,"Card paint owner retired");return p->paint(state,false);},*root);!r)return std::unexpected(r.error());
    if(auto r=overlay->set_paint_handler([weak](const auto& state)->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{auto p=weak.lock();if(!p)return core::fail(core::ErrorCode::closed,"Card paint owner retired");return p->paint(state,true);},*root);!r)return std::unexpected(r.error());
    if(auto r=root->set_layout_handler([weak](gui::Rect bounds){if(auto p=weak.lock())surface_detail::require(p->place(bounds));});!r)return std::unexpected(r.error());
    if(auto r=p->configure();!r)return std::unexpected(r.error());Card result;result.root_=*root;result.inline_=*inline_preview;result.state_=p;
    if(content.preview){if(auto r=result.set_preview_content(std::move(content.preview));!r)return std::unexpected(r.error());}
    if(content.icon){if(auto r=result.set_icon_content(std::move(content.icon));!r)return std::unexpected(r.error());}
    if(!root->snapshot())return core::fail(core::ErrorCode::closed,"Card factory retired its owner");return result;
}
core::Result<std::shared_ptr<Card::Impl>> Card::lock()const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"Card scope retired");return p;}
core::Result<CardOptions> Card::options()const{auto p=lock();if(!p)return std::unexpected(p.error());auto result=(*p)->options;auto native=root_.action_options();if(!native)return std::unexpected(native.error());result.selected=native->checked;return result;}
core::Result<void> Card::set_options(CardOptions options)const{
    auto p=lock();if(!p)return std::unexpected(p.error());if((*p)->configuring)return core::fail(core::ErrorCode::busy,"Card options are committing");auto prepared=card_detail::prepare(options,(*p)->appearance);if(!prepared)return std::unexpected(prepared.error());auto native=root_.action_options();if(!native)return std::unexpected(native.error());const bool changed=native->checked!=options.selected;auto callback=(*p)->events.selected_changed;
    {struct Guard{bool& value;explicit Guard(bool& v):value(v){value=true;}~Guard(){value=false;}}guard{(*p)->configuring};(*p)->options=std::move(options);(*p)->prepared=std::move(*prepared);if(auto r=(*p)->configure();!r)return r;}
    try{if(changed&&callback)callback((*p)->options.selected);}catch(...){return core::fail(core::ErrorCode::failed,"Card selection callback failed after update",std::current_exception());}return {};
}
core::Result<CardState> Card::state()const{auto p=lock();if(!p)return std::unexpected(p.error());auto root=root_.snapshot(),inline_view=inline_.snapshot();if(!root)return std::unexpected(root.error());const gui::WidgetPaintState state{{root->bounds.width,root->bounds.height},root->enabled,root->hovered,root->pressed,root->focused,root->checked};return card_detail::resolve((*p)->options,(*p)->prepared,state,(*p)->preview_custom||(inline_view&&inline_view->child_count>0));}
core::Result<gui::Widget> Card::preview_item()const{auto p=lock();if(!p)return std::unexpected(p.error());if((*p)->options.type!=CardType::file&&(*p)->options.type!=CardType::file_preview)return gui::Widget{};if((*p)->preview_custom)return (*p)->preview.content_item();auto inline_view=inline_.snapshot();if(inline_view&&inline_view->child_count)return inline_;return (*p)->base;}
core::Result<void> Card::set_selected(bool selected)const{auto o=options();if(!o)return std::unexpected(o.error());o->selected=selected;return set_options(std::move(*o));}
core::Result<void> Card::set_title(std::string_view title)const{auto o=options();if(!o)return std::unexpected(o.error());if(!surface_detail::text(title))return core::fail(core::ErrorCode::invalid_argument,"Invalid Card title");o->title=title;return set_options(std::move(*o));}
core::Result<void> Card::set_rows(std::vector<CardRow> rows)const{auto o=options();if(!o)return std::unexpected(o.error());o->rows=std::move(rows);return set_options(std::move(*o));}
core::Result<void> Card::set_preview_source(std::shared_ptr<const gui::RasterImage> source)const{auto o=options();if(!o)return std::unexpected(o.error());o->preview_source=std::move(source);return set_options(std::move(*o));}
core::Result<void> Card::set_preview_content(gui::WidgetFactory factory)const{auto p=lock();if(!p)return std::unexpected(p.error());const bool enabled=bool(factory);if(auto r=(*p)->preview.set_content(std::move(factory));!r)return r;(*p)->preview_custom=enabled;return (*p)->configure();}
core::Result<void> Card::set_icon_content(gui::WidgetFactory factory)const{auto p=lock();if(!p)return std::unexpected(p.error());const bool enabled=bool(factory);if(auto r=(*p)->icon.set_content(std::move(factory));!r)return r;(*p)->icon_custom=enabled;return (*p)->configure();}
core::Result<void> Card::set_events(CardEvents events)const{auto p=lock();if(!p)return std::unexpected(p.error());std::swap((*p)->events,events);return {};}
}
