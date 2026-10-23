#include "ListToolbar.h"
#include "ToolbarDetail.h"
#include "SurfaceDetail.h"
#include "Icons.h"
#include <algorithm>
#include <cmath>
namespace lvrs {
using namespace ii;
namespace {
core::Result<std::array<ToolbarButtonOptions,3>> prepare(const ListToolbarOptions& o){
    for(auto n:{o.icon_size,o.button_size,o.horizontal_padding,o.vertical_padding,o.spacing,o.width,o.height})if(!surface_detail::metric(n))return core::fail(core::ErrorCode::invalid_argument,"Invalid ListToolbar metric");
    if(o.icon_size<1||o.icon_size>256||o.button_size<1||o.button_size*3+o.spacing*2+o.horizontal_padding*2>8192||o.button_size+o.vertical_padding*2>8192||!surface_detail::color(o.background))return core::fail(core::ErrorCode::invalid_argument,"Invalid ListToolbar extent or color");
    std::array<ToolbarButtonOptions,3> result;
    const gui::WidgetColor empty{0,0,0,0},hover{.1230252981,.1252803057,.1275353134,1};
    for(std::size_t i=0;i<3;++i){auto& b=result[i];b.icon_name=o.icons[i];b.icon=o.images[i];b.enabled=o.interactive&&(!Icons::normalize(o.icons[i]).empty()||bool(o.images[i]));b.fallback_icon=false;b.width=o.button_size;b.height=o.button_size;b.icon_size=o.icon_size;b.left_padding=b.right_padding=std::max(0.,std::floor((o.button_size-o.icon_size)*.5));b.style=gui::ButtonStyle{.fill=empty,.hover_fill=hover,.pressed_fill=hover,.disabled_fill=empty};auto native=toolbar_detail::prepare(b);if(!native)return std::unexpected(native.error());}
    return result;
}
}
struct ListToolbar::Impl final {
    ListToolbarOptions options;ListToolbarEvents events;gui::Widget root;std::array<ToolbarButton,3> buttons;
    core::Result<void> paint(gui::Rect bounds)const{return surface_detail::paint(root,bounds,options.background,8);}
    core::Result<void> configure(const std::array<ToolbarButtonOptions,3>& prepared){
        for(std::size_t i=0;i<3;++i)if(auto r=buttons[i].set_options(prepared[i]);!r)return r;
        if(auto r=root.set_stack_options({.spacing=options.spacing,.alignment=gui::StackAlignment::center,.insets=gui::Insets{options.horizontal_padding,options.vertical_padding,options.horizontal_padding,options.vertical_padding}});!r)return r;
        if(auto r=root.set_extent({options.width,options.height});!r)return r;if(auto r=root.set_enabled(options.enabled);!r)return r;auto s=root.snapshot();if(!s)return std::unexpected(s.error());return paint(s->bounds);
    }
};
core::Result<ListToolbar> ListToolbar::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,ListToolbarOptions options,ListToolbarEvents events){
    auto prepared=prepare(options);if(!prepared)return std::unexpected(prepared.error());auto p=std::make_shared<Impl>();p->options=std::move(options);p->events=std::move(events);const auto weak=std::weak_ptr<Impl>{p};
    auto root=b.hstack(parent,id);if(!root)return std::unexpected(root.error());p->root=*root;
    for(std::size_t i=0;i<3;++i){auto button=ToolbarButton::create(b,*root,std::string(id)+"/button_"+std::to_string(i),(*prepared)[i],[weak,i]{if(auto p=weak.lock()){auto callback=p->events.icon_clicked;auto source=Icons::normalize(p->options.icons[i]);if(callback)callback(i,std::move(source));}});if(!button)return std::unexpected(button.error());p->buttons[i]=*button;}
    if(auto r=b.retain(p);!r)return std::unexpected(r.error());if(auto r=root->set_layout_handler([weak](gui::Rect bounds){if(auto p=weak.lock())surface_detail::require(p->paint(bounds));});!r)return std::unexpected(r.error());if(auto r=p->configure(*prepared);!r)return std::unexpected(r.error());ListToolbar result;result.root_=*root;result.state_=p;return result;
}
core::Result<std::shared_ptr<ListToolbar::Impl>> ListToolbar::lock()const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"ListToolbar scope retired");return p;}
core::Result<ListToolbarOptions> ListToolbar::options()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options;}
core::Result<void> ListToolbar::set_options(ListToolbarOptions options)const{auto p=lock();if(!p)return std::unexpected(p.error());auto prepared=prepare(options);if(!prepared)return std::unexpected(prepared.error());(*p)->options=std::move(options);return (*p)->configure(*prepared);}
core::Result<gui::Widget> ListToolbar::button_at(int index)const{auto p=lock();if(!p)return std::unexpected(p.error());if(index<0||index>=3)return core::fail(core::ErrorCode::out_of_range,"ListToolbar slot index must be 0..2");return (*p)->buttons[index].root();}
core::Result<std::string> ListToolbar::resolved_icon(int index)const{auto p=lock();if(!p)return std::unexpected(p.error());if(index<0||index>=3)return core::fail(core::ErrorCode::out_of_range,"ListToolbar slot index must be 0..2");return Icons::normalize((*p)->options.icons[index]);}
core::Result<void> ListToolbar::set_events(ListToolbarEvents events)const{auto p=lock();if(!p)return std::unexpected(p.error());std::swap((*p)->events,events);return {};}
}
