#include <LVRS/AlertButton.h>
#include <algorithm>
#include <cmath>
namespace lvrs {
namespace {
using namespace ii;
bool rgba(gui::WidgetColor c){for(auto n:{c.red,c.green,c.blue,c.alpha})if(!std::isfinite(n)||n<0||n>1)return false;return true;}
gui::WidgetColor scaled(gui::WidgetColor c,double n){c.red=std::min(1.,c.red*n);c.green=std::min(1.,c.green*n);c.blue=std::min(1.,c.blue*n);return c;}
}
struct AlertButton::Impl final {
    AlertButtonOptions options;AlertButtonEvents events;ii::gui::Widget root;ii::gui::WidgetColor primary;
    ii::gui::ButtonStyle style()const{
        using namespace ii::gui;const auto p=options.primary_color.value_or(primary);const auto tone=options.tone;
        ButtonStyle s{.disabled_fill=WidgetColor{.148205325,.152569115,.152569115,1},.text=options.text_color.value_or(options.dialog_style?WidgetColor{244./255,245./255,247./255,1}:tone==ButtonTone::borderless?p:WidgetColor{1,1,1,.9}),.disabled_text=WidgetColor{1,1,1,.3},.line_height=13};
        if(tone==ButtonTone::primary||tone==ButtonTone::destructive){auto c=tone==ButtonTone::primary?p:WidgetColor{1,.276085496,.276085496,1};s.fill=c;s.hover_fill=scaled(c,1/1.12);s.pressed_fill=scaled(c,1/1.2);}
        else if(tone==ButtonTone::normal&&options.dialog_style){s.fill=WidgetColor{0,0,0,0};s.hover_fill=WidgetColor{1,1,1,.05};s.pressed_fill=WidgetColor{1,1,1,.09};s.border=WidgetColor{89./255,97./255,104./255,1};s.border_width=1;}
        else if(tone==ButtonTone::normal){auto c=WidgetColor{.193556845,.197807208,.199932396,1};s.fill=c;s.hover_fill=scaled(c,1.08);s.pressed_fill=WidgetColor{.155585155,.157072559,.158559948,1};}
        else {s.fill=WidgetColor{0,0,0,0};s.hover_fill=WidgetColor{.123025298,.125280306,.127535313,1};s.pressed_fill=WidgetColor{.141176477*.75+p.red*.25,.141176477*.75+p.green*.25,.141176477*.75+p.blue*.25,1};}
        return s;
    }
};
ii::core::Result<AlertButton> AlertButton::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,std::string_view id,AlertButtonOptions o,AlertButtonEvents e){
    using namespace ii;if(static_cast<unsigned>(o.tone)>4||(o.text_color&&!rgba(*o.text_color))||(o.primary_color&&!rgba(*o.primary_color)))return core::fail(core::ErrorCode::invalid_argument,"Invalid AlertButton tone or color");auto appearance=b.appearance();if(!appearance)return std::unexpected(appearance.error());
    auto s=std::make_shared<Impl>();s->options=std::move(o);s->events=std::move(e);s->primary=appearance->primary;std::weak_ptr<Impl> weak=s;
    auto root=b.button(parent,id,{.text=s->options.text,.enabled=s->options.enabled,.tone=s->options.tone,.height=s->options.height.value_or(20),.corner_radius=s->options.corner_radius.value_or(s->options.dialog_style?16:4),.horizontal_padding=s->options.horizontal_padding.value_or(s->options.dialog_style?16:8),.style=s->style()},[weak]{auto s=weak.lock();if(!s)return;auto source=s->root;auto method=s->events.method;auto clicked=s->events.clicked;if(method)method({source,"clicked"});if(source.snapshot()&&clicked)clicked();});
    if(!root)return std::unexpected(root.error());s->root=*root;if(auto r=b.retain(s);!r)return std::unexpected(r.error());if(auto r=root->set_control_events({.pressed=s->events.pressed,.released=s->events.released,.canceled=s->events.canceled});!r)return std::unexpected(r.error());AlertButton button;button.root_=*root;button.state_=s;return button;
}
ii::core::Result<std::shared_ptr<AlertButton::Impl>> AlertButton::lock()const{auto r=root_.snapshot();if(!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"AlertButton retired");return s;}
ii::core::Result<void> AlertButton::set_text(std::string_view s)const{return root_.set_text(s);}
ii::core::Result<void> AlertButton::set_enabled(bool v)const{auto s=lock();if(!s)return std::unexpected(s.error());return root_.set_enabled(v&&(*s)->options.tone!=ii::gui::ButtonTone::disabled);}
ii::core::Result<void> AlertButton::set_method(ButtonMethod v)const{auto s=lock();if(!s)return std::unexpected(s.error());std::swap((*s)->events.method,v);return {};}
ii::core::Result<void> AlertButton::set_text_color(ii::gui::WidgetColor c)const{if(!rgba(c))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid text color");auto s=lock();if(!s)return std::unexpected(s.error());(*s)->options.text_color=c;return root_.set_button_style((*s)->style());}
ii::core::Result<void> AlertButton::set_primary_color(ii::gui::WidgetColor c)const{if(!rgba(c))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid primary color");auto s=lock();if(!s)return std::unexpected(s.error());(*s)->options.primary_color=c;return root_.set_button_style((*s)->style());}
}
