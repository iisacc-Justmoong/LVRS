#include <LVRS/ListFooter.h>
#include <LVRS/ToolbarDetail.h>
#include <LVRS/SurfaceDetail.h>
#include <LVRS/Theme.h>
namespace lvrs {
using namespace ii;
namespace {
gui::WidgetColor muted(gui::WidgetColor primary){
    if(primary==Theme::accent_color(Accent::blue))return {.1261835851,.2333763603,.3558823529,1};
    const auto alpha=primary.alpha*.25,base=.1411764770746231;return {base*(1-alpha)+primary.red*alpha,base*(1-alpha)+primary.green*alpha,base*(1-alpha)+primary.blue*alpha,1};
}
core::Result<std::array<ToolbarButtonOptions,3>> prepare(const ListFooterOptions& o,gui::WidgetColor pressed){
    for(auto n:{o.horizontal_padding,o.vertical_padding,o.spacing,o.width,o.height})if(!surface_detail::metric(n))return core::fail(core::ErrorCode::invalid_argument,"Invalid ListFooter metric");
    if(o.horizontal_padding*2+o.spacing*2>8192||o.vertical_padding*2+22>8192)return core::fail(core::ErrorCode::invalid_argument,"ListFooter insets exceed native bounds");
    std::array<ToolbarButtonOptions,3> result;
    const gui::WidgetColor empty{0,0,0,0},hover{.1230252981,.1252803057,.1275353134,1};
    for(std::size_t i=0;i<3;++i){result[i]=o.buttons[i].button;result[i].enabled&=o.interactive;if(!result[i].style)result[i].style=gui::ButtonStyle{.fill=empty,.hover_fill=hover,.pressed_fill=pressed,.disabled_fill=empty};auto native=toolbar_detail::prepare(result[i]);if(!native)return std::unexpected(native.error());}
    return result;
}
}
struct ListFooter::Impl final {
    ListFooterOptions options;ListFooterEvents events;gui::Widget root;std::array<ToolbarButton,3> buttons;gui::WidgetColor pressed;
    core::Result<void> configure(const std::array<ToolbarButtonOptions,3>& prepared){
        for(std::size_t i=0;i<3;++i)if(auto r=buttons[i].set_options(prepared[i]);!r)return r;
        if(auto r=root.set_stack_options({.spacing=options.spacing,.alignment=gui::StackAlignment::center,.insets=gui::Insets{options.horizontal_padding,options.vertical_padding,options.horizontal_padding,options.vertical_padding}});!r)return r;
        if(auto r=root.set_extent({options.width,options.height});!r)return r;return root.set_enabled(options.enabled);
    }
};
const ListFooterSlot& footer_slot(const ListFooterSlot& slot){return slot;}
const ListFooterSlot& footer_slot(const std::shared_ptr<const ListFooterSlot>& slot){static const ListFooterSlot empty;return slot?*slot:empty;}
core::Result<ListFooter> ListFooter::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,ListFooterOptions options,ListFooterEvents events){
    auto appearance=b.appearance();if(!appearance)return std::unexpected(appearance.error());auto prepared=prepare(options,muted(appearance->primary));if(!prepared)return std::unexpected(prepared.error());auto p=std::make_shared<Impl>();p->options=std::move(options);p->events=std::move(events);p->pressed=muted(appearance->primary);const auto weak=std::weak_ptr<Impl>{p};
    auto root=b.hstack(parent,id);if(!root)return std::unexpected(root.error());p->root=*root;
    for(std::size_t i=0;i<3;++i){auto button=ToolbarButton::create(b,*root,std::string(id)+"/button_"+std::to_string(i),(*prepared)[i],[weak,i]{auto p=weak.lock();if(!p)return;auto config=p->options.buttons[i];auto callback=p->events.button_clicked;if(config.on_clicked)config.on_clicked();if(p->root.snapshot()&&callback)callback(i,config);});if(!button)return std::unexpected(button.error());p->buttons[i]=*button;}
    if(auto r=b.retain(p);!r)return std::unexpected(r.error());if(auto r=p->configure(*prepared);!r)return std::unexpected(r.error());ListFooter result;result.root_=*root;result.state_=p;return result;
}
core::Result<std::shared_ptr<ListFooter::Impl>> ListFooter::lock()const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"ListFooter scope retired");return p;}
core::Result<ListFooterOptions> ListFooter::options()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options;}
core::Result<void> ListFooter::set_options(ListFooterOptions o)const{auto p=lock();if(!p)return std::unexpected(p.error());auto prepared=prepare(o,(*p)->pressed);if(!prepared)return std::unexpected(prepared.error());(*p)->options=std::move(o);return (*p)->configure(*prepared);}
core::Result<void> ListFooter::set_slot(int index,ListFooterSlot slot)const{auto o=options();if(!o)return std::unexpected(o.error());if(index<0||index>=3)return core::fail(core::ErrorCode::out_of_range,"ListFooter slot index must be 0..2");o->buttons[index]=std::move(slot);return set_options(std::move(*o));}
core::Result<gui::Widget> ListFooter::button_at(int index)const{auto p=lock();if(!p)return std::unexpected(p.error());if(index<0||index>=3)return core::fail(core::ErrorCode::out_of_range,"ListFooter slot index must be 0..2");return (*p)->buttons[index].root();}
core::Result<void> ListFooter::set_events(ListFooterEvents events)const{auto p=lock();if(!p)return std::unexpected(p.error());std::swap((*p)->events,events);return {};}
}
