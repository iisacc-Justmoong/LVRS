#include "HierarchyItem.h"
#include "Icons.h"
#include "SurfaceDetail.h"
#include <algorithm>
#include <cmath>
#include <exception>
#include <tuple>
namespace lvrs {
using namespace ii;
namespace {
struct Failure final:std::exception {
    core::Error error;
    explicit Failure(core::Error value):error(std::move(value)){}
    const char* what()const noexcept override{return error.message.c_str();}
};
template<class T>T take(core::Result<T> r){if(!r)throw Failure{r.error()};return std::move(*r);}
void good(core::Result<void> r){if(!r)throw Failure{r.error()};}
template<class F>auto protect(F f)->decltype(f()){try{return f();}catch(const Failure& e){return std::unexpected(e.error);}catch(...){return core::fail(core::ErrorCode::failed,"Hierarchy item operation failed",std::current_exception());}}
core::Result<void> validate(const HierarchyItemOptions& o){
    for(const auto* text:{&o.row.key,&o.row.label,&o.row.icon_name,&o.row.icon_glyph})if(!surface_detail::text(*text))return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy item text");
    for(auto n:{o.width,o.row_height,o.indent_step,o.icon_size,o.chevron_size,o.left_padding,o.right_padding,o.spacing,o.corner_radius,o.count_width})if(!surface_detail::metric(n))return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy item geometry");
    if(o.row_height<=0||o.width<=0||o.icon_size>256||o.chevron_size>256||!std::isfinite(o.drag_opacity)||o.drag_opacity<0||o.drag_opacity>1)return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy item size or opacity");
    for(auto c:{o.text,o.disabled_text,o.description,o.idle,o.hover,o.pressed,o.active,o.inactive,o.drag})if(!surface_detail::color(c))return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy item color");return {};
}
gui::Rect box(double x,double y,double w,double h){return {float(x),float(y),float(std::max(0.,w)),float(std::max(0.,h))};}
bool equivalent(const HierarchyItemOptions& a,const HierarchyItemOptions& b){
    const auto values=[](const auto& o){return std::tie(o.row,o.width,o.row_height,o.indent_step,o.icon_size,o.chevron_size,o.left_padding,o.right_padding,o.spacing,o.corner_radius,o.count_width,o.drag_preview,o.drag_opacity,o.text,o.disabled_text,o.description,o.idle,o.hover,o.pressed,o.active,o.inactive,o.drag);};
    return a.controller.lock()==b.controller.lock()&&values(a)==values(b);
}
}
struct HierarchyItem::Impl final:std::enable_shared_from_this<Impl> {
    HierarchyItemOptions value;HierarchyItemEvents events;
    gui::Widget root,body,label,count,chevron,custom;
    std::shared_ptr<const gui::RasterImage> icon,open_icon,closed_icon;
    HierarchyCountFactory count_factory;
    core::Observable<std::uint64_t> epoch{0};
    double width=200,height=20,natural_count=0,chevron_x=0,count_x=0;
    bool has_custom=false,pressed=false,suppress=false,laying_out=false,configuring=false;
    bool show_chevron()const{return value.row.show_chevron&&value.row.has_child_items;}
    double opacity()const{return value.drag_preview?value.drag_opacity:1;}
    gui::ActionOptions action()const{return {.accessibility_name=value.row.label.empty()?value.row.key:value.row.label,.enabled=value.row.enabled,.focus_ring=true,.padding={},.width=value.width,.height=value.row_height,.checked=value.row.selected,.tab_focusable=value.row.activatable,.pointer_focusable=value.row.activatable};}
    void layout(){
        if(laying_out)return;laying_out=true;struct Reset{bool& b;~Reset(){b=false;}}reset{laying_out};if(!root.snapshot())return;
        good(body.set_extent({width,height}));
        const bool has_count=value.row.count>=0,anchor=show_chevron()||has_count;
        const auto right=std::max(0.,width-value.right_padding);
        chevron_x=anchor?std::max(0.,right-value.chevron_size):right;
        const auto count_width=has_count?(has_custom?value.count_width:std::max(0.,natural_count)):0;
        count_x=std::max(0.,chevron_x-(has_count?8+count_width:0));
        const auto left=std::min(width,value.left_padding+value.row.depth*value.indent_step+value.icon_size+value.spacing);
        const auto end=anchor?std::max(left,count_x-value.spacing):std::max(left,right);
        good(label.set_frame(box(left,0,end-left,height)));
        good(count.set_frame(box(count_x,(height-16)/2,count_width,16)));good(count.set_visible(has_count&&!has_custom));
        good(chevron.set_frame(box(chevron_x,(height-value.chevron_size)/2,value.chevron_size,value.chevron_size)));good(chevron.set_visible(show_chevron()));
        good(custom.set_frame(box(count_x,0,count_width,height)));good(custom.set_visible(has_count&&has_custom));
        good(root.request_repaint());
    }
    core::Result<std::shared_ptr<const gui::WidgetDrawing>> paint(const gui::WidgetPaintState& p)const{
        auto d=std::make_shared<gui::WidgetDrawing>();const auto alpha=opacity();const auto color=[&](gui::WidgetColor c){c.alpha*=alpha;return surface_detail::linear(c);};
        auto background=value.drag_preview?value.drag:!p.enabled||!value.row.enabled||!value.row.activatable?value.inactive:value.row.selected?value.active:pressed||p.pressed?value.pressed:p.hovered?value.hover:value.idle;
        auto ink=color(background);if(background.alpha>0)d->panels.push_back({box(0,0,p.size.width,p.size.height),ink,ink,float(std::min(value.corner_radius,std::min(p.size.width,p.size.height)/2))});
        const auto x=std::min(p.size.width,value.left_padding+value.row.depth*value.indent_step),y=(p.size.height-value.icon_size)/2;
        if(value.icon_size>0){
            if(icon){const auto ratio=std::min(value.icon_size/icon->width(),value.icon_size/icon->height());const auto w=icon->width()*ratio,h=icon->height()*ratio;d->images.push_back({.bounds=box(x+(value.icon_size-w)/2,y+(value.icon_size-h)/2,w,h),.image=icon,.opacity=float(alpha*(value.row.enabled?1:.45))});}
            else if(!value.row.icon_glyph.empty())d->texts.push_back({.bounds=box(x,y,value.icon_size,value.icon_size),.text=value.row.icon_glyph,.color=color(value.row.enabled?value.text:value.disabled_text),.font_size=float(value.icon_size),.alignment=gui::TextAlignment::center});
            else{const auto placeholder=color({1,1,1,.16});d->panels.push_back({box(x+value.icon_size*.12,y+value.icon_size*.22,value.icon_size*.76,value.icon_size*.62),placeholder,placeholder,float(value.icon_size*2/16)});}
        }
        if(show_chevron())d->images.push_back({.bounds=box(std::max(0.,p.size.width-value.right_padding-value.chevron_size),(p.size.height-value.chevron_size)/2,value.chevron_size,value.chevron_size),.image=value.row.expanded?open_icon:closed_icon,.opacity=float(alpha*(value.row.enabled?1:.45))});
        return std::shared_ptr<const gui::WidgetDrawing>(std::move(d));
    }
    void activate(){if(value.row.enabled&&value.row.activatable){auto callback=events.activated;if(callback)callback();}}
    void toggle(){
        if(!value.row.enabled||!show_chevron())return;const auto expanded=!value.row.expanded;value.row.expanded=expanded;epoch.set(epoch.get()+1);good(root.request_repaint());auto callback=events.expansion_changed;if(callback)callback(expanded);if(root.snapshot())activate();
    }
    core::Result<bool> input(const gui::WidgetInput& e){return protect([&]()->core::Result<bool>{
        if(!value.row.enabled)return false;
        const bool main=e.x<count_x||(!show_chevron()&&value.row.count<0);
        if(e.kind==gui::WidgetInputKind::pointer_down&&main&&!(e.buttons&2)){pressed=true;suppress=false;if(value.row.activatable)good(root.focus());if(events.pointer)suppress=take(events.pointer(e));good(root.request_repaint());return true;}
        if(pressed&&e.kind==gui::WidgetInputKind::pointer_move){if(events.pointer)suppress=take(events.pointer(e))||suppress;good(root.request_repaint());return true;}
        if(pressed&&e.kind==gui::WidgetInputKind::pointer_up){pressed=false;if(events.pointer)suppress=take(events.pointer(e))||suppress;const bool click=!suppress&&e.x>=0&&e.x<width&&e.y>=0&&e.y<height;good(root.request_repaint());if(click)activate();return true;}
        if(e.kind==gui::WidgetInputKind::focus_lost||e.kind==gui::WidgetInputKind::close_popup){const bool was_pressed=pressed;pressed=false;suppress=false;if(was_pressed&&events.pointer)(void)take(events.pointer(e));good(root.request_repaint());}
        return false;
    });}
    void configure(HierarchyItemOptions next){
        configuring=true;struct Reset{bool& value;~Reset(){value=false;}}reset{configuring};
        auto decoded=next.row.icon;if(!decoded&&!next.row.icon_name.empty())decoded=take(Icons::image(next.row.icon_name));
        const bool create_count=has_custom&&next.row.count>=0&&!custom.content_item();
        std::optional<HierarchyItemOptions> previous;HierarchyCountFactory factory;if(create_count){previous=value;factory=count_factory;}
        value=std::move(next);
        if(create_count){
            HierarchyItem item;item.root_=root;item.state_=weak_from_this();
            auto built=protect([&]()->core::Result<void>{return custom.set_content([factory,item](gui::WidgetBuilder& b){return factory(b,{},item);});});
            if(!built){value=std::move(*previous);throw Failure{built.error()};}
        }
        icon=std::move(decoded);width=value.width;height=value.row_height;
        auto text=value.row.enabled?value.text:value.disabled_text;text.alpha*=opacity();auto desc=value.row.enabled?value.description:value.disabled_text;desc.alpha*=opacity();
        good(root.set_action_options(action()));good(label.set_label_options({.text=value.row.label.empty()?value.row.key:value.row.label,.font_size=12,.selectable=false,.role=gui::TextRole::body,.color=text,.line_height=16,.font_weight=600,.elide_right=true}));
        good(count.set_label_options({.text=value.row.count>=0?std::to_string(value.row.count):"",.font_size=12,.selectable=false,.role=gui::TextRole::description,.color=desc,.alignment=gui::TextAlignment::right,.line_height=16,.font_weight=600}));
        auto expand_action=gui::ActionOptions{.accessibility_name=(value.row.expanded?"Collapse ":"Expand ")+(value.row.label.empty()?value.row.key:value.row.label),.enabled=value.row.enabled,.focus_ring=false,.tab_focusable=false,.pointer_focusable=false};good(chevron.set_action_options(std::move(expand_action)));
        if(has_custom){
            if(value.row.count<0&&custom.content_item())good(custom.clear_content());
        }
        const auto bounds=take(root.snapshot()).bounds;if(bounds.width>0&&bounds.height>0){width=bounds.width;height=bounds.height;}layout();epoch.set(epoch.get()+1);
    }
};
bool HierarchyItemOptions::operator==(const HierarchyItemOptions& other)const{return equivalent(*this,other);}
core::Result<void> HierarchyItem::validate_options(const HierarchyItemOptions& options){return validate(options);}
core::Result<HierarchyItem> HierarchyItem::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,HierarchyItemOptions options,HierarchyItemEvents events,HierarchyCountFactory count_factory){
    if(auto r=validate(options);!r)return std::unexpected(r.error());
    return protect([&]()->core::Result<HierarchyItem>{auto p=std::make_shared<Impl>();p->value=options;p->events=std::move(events);p->width=options.width;p->height=options.row_height;p->has_custom=bool(count_factory);p->count_factory=std::move(count_factory);auto weak=std::weak_ptr(p);const auto prefix=std::string(id);
        p->open_icon=take(Icons::image("generalchevronDown"));p->closed_icon=take(Icons::image("generalchevronRight"));
        p->root=take(b.action_surface(parent,id,p->action(),[weak]{if(auto p=weak.lock())p->activate();}));p->body=take(b.absolute_stack(p->root,prefix+".body",{options.width,options.row_height}));
        good(p->root.set_clip_content(true));
        p->label=take(b.label(p->body,prefix+".label",{.selectable=false}));p->count=take(b.label(p->body,prefix+".count",{.selectable=false}));
        p->chevron=take(b.action_surface(p->body,prefix+".chevron",{.focus_ring=false,.tab_focusable=false,.pointer_focusable=false},[weak]{if(auto p=weak.lock())p->toggle();}));
        p->custom=take(b.content_host(p->body,prefix+".countView",{options.count_width,options.row_height}));good(b.retain(p));HierarchyItem result;result.root_=p->root;result.state_=p;
        good(p->root.set_paint_handler([weak](const auto& state)->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{if(auto p=weak.lock())return p->paint(state);return core::fail(core::ErrorCode::closed,"Hierarchy item retired");}));
        good(p->root.set_input_handler([weak](const auto& input)->core::Result<bool>{if(auto p=weak.lock())return p->input(input);return false;}));
        good(p->root.set_layout_handler([weak](gui::Rect bounds){if(auto p=weak.lock()){p->width=bounds.width;p->height=bounds.height;p->layout();}}));
        good(p->count.set_measure_handler([weak](const auto& measurement){if(auto p=weak.lock())if(std::abs(p->natural_count-measurement.natural.width)>.01){p->natural_count=measurement.natural.width;p->layout();}}));p->configure(std::move(options));return result;
    });
}
core::Result<std::shared_ptr<HierarchyItem::Impl>> HierarchyItem::lock()const{return core::ObservableReadScope::pause([&]()->core::Result<std::shared_ptr<Impl>>{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"Hierarchy item retired");return p;});}
core::Result<HierarchyItemOptions> HierarchyItem::options()const{auto p=lock();if(!p)return std::unexpected(p.error());(void)(*p)->epoch.get();return (*p)->value;}
core::Result<void> HierarchyItem::set_options(HierarchyItemOptions options)const{auto p=lock();if(!p)return std::unexpected(p.error());if((*p)->configuring)return core::fail(core::ErrorCode::busy,"Hierarchy item configuration is in progress");if(auto r=validate(options);!r)return r;if(equivalent(options,(*p)->value))return {};return protect([&]()->core::Result<void>{(*p)->configure(std::move(options));return {};});}
core::Result<void> HierarchyItem::set_selected(bool value)const{auto options=this->options();if(!options)return std::unexpected(options.error());options->row.selected=value;return set_options(std::move(*options));}
core::Result<void> HierarchyItem::set_expanded(bool value)const{auto options=this->options();if(!options)return std::unexpected(options.error());options->row.expanded=value;return set_options(std::move(*options));}
core::Result<void> HierarchyItem::activate()const{auto p=lock();if(!p)return std::unexpected(p.error());return protect([&]()->core::Result<void>{(*p)->activate();return {};});}
core::Result<void> HierarchyItem::toggle_expanded()const{auto p=lock();if(!p)return std::unexpected(p.error());return protect([&]()->core::Result<void>{(*p)->toggle();return {};});}
core::Result<HierarchyMetadata> HierarchyItem::metadata()const{auto p=lock();if(!p)return std::unexpected(p.error());if(auto controller=(*p)->value.controller.lock())return controller->metadata((*p)->value.row.key);return core::fail(core::ErrorCode::not_found,"Standalone hierarchy item has no model metadata");}
core::Result<gui::Widget> HierarchyItem::count_content(std::string_view id)const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->custom.content_item(id);}
core::Result<std::shared_ptr<const gui::WidgetDrawing>> HierarchyItem::drawing(gui::WidgetPaintState state)const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->paint(state);}
}
