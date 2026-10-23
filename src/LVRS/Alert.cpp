#include "Alert.h"
#include "Theme.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <cmath>
namespace lvrs::detail {std::string_view alert_adjustments();std::string_view alert_file_text();}
namespace lvrs {
namespace {
using namespace ii;
struct BuildError final {core::Error error;};
template<class T>T take(core::Result<T> r){if(!r)throw BuildError{r.error()};return std::move(*r);}
void good(core::Result<void> r){if(!r)throw BuildError{r.error()};}
bool text(std::string_view s){return s.size()<=65536&&s.find('\0')==s.npos&&core::valid_utf8(s);}
bool metric(double v){return std::isfinite(v)&&v>=0&&v<=8192;}
bool rgba(gui::WidgetColor c){for(auto n:{c.red,c.green,c.blue,c.alpha})if(!std::isfinite(n)||n<0||n>1)return false;return true;}
gui::WidgetColor tint(gui::WidgetColor c,double a){const auto alpha=c.alpha*a;return {.1411764771*(1-alpha)+c.red*alpha,.1411764771*(1-alpha)+c.green*alpha,.1411764771*(1-alpha)+c.blue*alpha,1};}
auto solid(){static auto image=gui::rgba_image(1,1,{54,59,63,255});if(!image)throw BuildError{image.error()};return *image;}
auto builtin(AlertIcon i){static auto adjustments=gui::svg_image(take(alert_icon_source(AlertIcon::adjustments)),128,128);static auto file=gui::svg_image(take(alert_icon_source(AlertIcon::file_text)),128,128);return take(i==AlertIcon::adjustments?adjustments:file);}
}
ii::core::Result<std::string_view> alert_icon_source(AlertIcon i){if(i==AlertIcon::adjustments)return detail::alert_adjustments();if(i==AlertIcon::file_text)return detail::alert_file_text();return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Unknown Alert icon");}
struct Alert::Impl final {
    AlertOptions options;AlertEvents events;std::string id;ii::gui::WidgetColor primary;bool updating=false;
    ii::gui::Widget root,content,texts,title,message,icon_row,icon,divider,vertical,horizontal,single;
    std::array<AlertButton,3> v;std::array<AlertButton,2> h;AlertButton one;
    std::array<ButtonMethod,3> methods;
    std::shared_ptr<const ii::gui::RasterImage> adjustments,file,placeholder;
    int count()const{return options.button_count==2||options.button_count==3?options.button_count:!options.tertiary_text.empty()?3:!options.secondary_text.empty()?2:1;}
    std::string label(int i)const{const auto& s=i==1?options.primary_text:i==2?options.secondary_text:options.tertiary_text;return s.empty()?"Button":s;}
    bool enabled(int i)const{return i==1?options.primary_enabled:i==2?options.secondary_enabled:options.tertiary_enabled;}
    AlertButton button(int i)const{if(count()==3)return v[std::size_t(i-1)];if(count()==2)return h[std::size_t(i-1)];return one;}
    ii::gui::PopoverOptions surface()const{return {.width=options.preferred_width,.padding=0,.margins=options.side_padding,.corner_radius=options.corner_radius,.surface_opacity=1,.surface_color=options.card_color,.modal=true,.dim=true,.clip_content=true,.dismiss_on_escape=options.dismiss_on_escape,.dismiss_on_outside_press=options.dismiss_on_background,.accessibility_name=options.accessibility_name,.placement=ii::gui::PopoverPlacement::centered,.minimum_width=options.minimum_width,.maximum_width=options.maximum_width,.cylinder=options.shape==AlertShape::cylinder,.backdrop_color=options.backdrop_color,.fit_to_viewport=true,.opaque_base=false,.corner_limit=.4,.border_width=1,.border_color={1,1,1,.2},.blur_radius=options.glass_enabled?std::optional<double>{std::clamp(std::ceil(options.glass_blur_radius),2.,64.)}:std::nullopt,.shadow=options.shadow_enabled?std::optional<ii::gui::PopoverShadow>{options.shadow}:std::nullopt,.transition=alert_transition(options.motion,options.motion_enabled)};}
    ii::gui::ImageOptions icon_options()const{
        using namespace ii::gui;const auto side=options.icon_size.value_or(count()==3?56:64),frame=options.icon_frame_size.value_or(std::max(86.,side));const bool standard=primary==Theme::accent_color(Accent::blue);
        auto image=options.image_source?options.image_source:options.use_builtin_image?(options.builtin_icon.value_or(count()==3?AlertIcon::file_text:AlertIcon::adjustments)==AlertIcon::file_text?file:adjustments):placeholder;
        return {.image=image,.width=frame,.height=frame,.accessibility_name="Application icon",.padding=(frame-(options.image_source||options.use_builtin_image?side:32))/2,.corner_radius=28,.border_width=1,.background=options.icon_background.value_or(standard?WidgetColor{.1315810238,.2001844,.2785882353,1}:tint(primary,.16)),.border=options.icon_border.value_or(standard?WidgetColor{.1195867154,.2739443118,.4503529412,1}:tint(primary,.36)),.preserve_aspect=true};
    }
    ii::core::Result<void> apply(){
        using namespace ii;if(updating)return core::fail(core::ErrorCode::busy,"Alert update is reentrant");updating=true;struct Guard final {bool& flag;~Guard(){flag=false;}} guard{updating};
        try{int focus=0;for(int i=1;i<=3;++i){if(take(v[std::size_t(i-1)].root().snapshot()).focused)focus=i;if(i<=2&&take(h[std::size_t(i-1)].root().snapshot()).focused)focus=i;}if(take(one.root().snapshot()).focused)focus=1;const bool open=take(root.snapshot()).opened;
            good(title.set_text(options.title));good(title.set_visible(!options.title.empty()));good(message.set_text(options.message));good(message.set_visible(!options.message.empty()));good(texts.set_visible(!options.title.empty()||!options.message.empty()));good(icon_row.set_visible(options.show_icon));good(icon.set_image_options(icon_options()));
            good(vertical.set_visible(count()==3));good(horizontal.set_visible(count()==2));good(single.set_visible(count()==1));good(divider.set_stack_options({.spacing=0,.alignment=gui::StackAlignment::stretch,.insets=gui::Insets{count()==3?0.:options.side_padding,0,count()==3?0.:options.side_padding,0}}));
            const bool destructive=options.secondary_destructive.value_or(count()==3);const auto normal=gui::WidgetColor{244./255,245./255,247./255,1},danger=Theme::accent_color(Accent::red);
            for(int i=1;i<=3;++i){auto update=[&](const AlertButton& b){good(b.set_text(label(i)));good(b.set_enabled(enabled(i)));good(b.set_primary_color(primary));good(b.set_text_color(i==2&&destructive?danger:normal));};update(v[std::size_t(i-1)]);if(i<=2)update(h[std::size_t(i-1)]);else{}if(i==1)update(one);}
            good(root.set_popover_options(surface()));if(open&&focus){if(focus>count()||!enabled(focus))focus=1;if(enabled(focus))good(button(focus).root().focus());}return {};
        }catch(const BuildError& e){return std::unexpected(e.error);}
    }
};
ii::core::Result<Alert> Alert::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,std::string_view id,AlertOptions o,AlertEvents e){
    using namespace ii;for(const auto* s:{&o.title,&o.message,&o.primary_text,&o.secondary_text,&o.tertiary_text,&o.accessibility_name})if(!text(*s))return core::fail(core::ErrorCode::invalid_argument,"Alert requires bounded UTF-8 text");
    if(!metric(o.minimum_width)||!metric(o.maximum_width)||!o.maximum_width||o.minimum_width>o.maximum_width||!metric(o.preferred_width)||!o.preferred_width||!metric(o.side_padding)||!metric(o.corner_radius)||static_cast<unsigned>(o.shape)>1||!std::isfinite(o.glass_blur_radius)||std::abs(o.glass_blur_radius)>8192||!rgba(o.backdrop_color)||!rgba(o.card_color)||!rgba(o.icon_inner)||(o.icon_background&&!rgba(*o.icon_background))||(o.icon_border&&!rgba(*o.icon_border))||(o.primary_color&&!rgba(*o.primary_color))||(o.icon_size&&(!metric(*o.icon_size)||!*o.icon_size))||(o.icon_frame_size&&(!metric(*o.icon_frame_size)||!*o.icon_frame_size)))return core::fail(core::ErrorCode::invalid_argument,"Invalid Alert geometry or color");
    const auto largest=o.icon_size.value_or(64);if(o.icon_frame_size&&*o.icon_frame_size<std::max(32.,largest))return core::fail(core::ErrorCode::invalid_argument,"Icon frame must contain its image");
    if(o.builtin_icon&&static_cast<unsigned>(*o.builtin_icon)>1)return core::fail(core::ErrorCode::invalid_argument,"Unknown builtin Alert icon");
    try{auto s=std::make_shared<Impl>();s->options=std::move(o);s->events=std::move(e);s->methods={s->events.button1_method,s->events.button2_method,s->events.button3_method};s->id=id;s->primary=s->options.primary_color.value_or(take(b.appearance()).primary);
        s->adjustments=builtin(AlertIcon::adjustments);s->file=builtin(AlertIcon::file_text);auto c=s->options.icon_inner;const auto hex=[](double n){const char* digits="0123456789ABCDEF";auto v=std::clamp(int(std::lround(n*255)),0,255);return std::string{digits[v/16],digits[v%16]};};s->placeholder=take(gui::svg_image("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'><rect width='32' height='32' rx='8' fill='#"+hex(c.red)+hex(c.green)+hex(c.blue)+"' opacity='"+std::to_string(.42*c.alpha)+"'/></svg>",64,64));
        s->root=take(b.popover(parent,id,s->surface(),{.opened=s->events.opened,.closed=s->events.closed,.canceled=s->events.dismissed}));std::weak_ptr<Impl> weak=s;
        auto card=take(b.vstack(s->root,s->id+".card",{.spacing=0,.alignment=gui::StackAlignment::stretch}));s->content=take(b.vstack(card,s->id+".content",{.spacing=28,.alignment=gui::StackAlignment::stretch,.insets=gui::Insets{0,46,0,36}}));s->icon_row=take(b.hstack(s->content,s->id+".iconRow",{.spacing=0}));take(b.spacer(s->icon_row,s->id+".iconLeft"));s->icon=take(b.image(s->icon_row,s->id+".icon",s->icon_options()));take(b.spacer(s->icon_row,s->id+".iconRight"));s->texts=take(b.vstack(s->content,s->id+".texts",{.spacing=14,.alignment=gui::StackAlignment::stretch,.insets=gui::Insets{24,0,24,0}}));s->title=take(b.label(s->texts,s->id+".title",{.text=s->options.title,.font_size=26,.role=gui::TextRole::title,.color=gui::WidgetColor{244./255,245./255,247./255,1},.alignment=gui::TextAlignment::center,.line_height=26}));s->message=take(b.label(s->texts,s->id+".message",{.text=s->options.message,.font_size=13,.role=gui::TextRole::body,.color=gui::WidgetColor{214./255,217./255,223./255,1},.alignment=gui::TextAlignment::center,.line_height=13}));
        s->divider=take(b.vstack(card,s->id+".divider",{.spacing=0,.alignment=gui::StackAlignment::stretch}));take(b.image(s->divider,s->id+".dividerLine",{.image=solid(),.width=1,.height=1}));
        s->vertical=take(b.vstack(card,s->id+".vertical",{.spacing=12,.alignment=gui::StackAlignment::stretch,.insets=gui::Insets{24,18,24,18}}));s->horizontal=take(b.hstack(card,s->id+".horizontal",{.spacing=14,.alignment=gui::StackAlignment::leading,.insets=gui::Insets{24,28,24,32}}));s->single=take(b.vstack(card,s->id+".single",{.spacing=0,.alignment=gui::StackAlignment::stretch,.insets=gui::Insets{24,28,24,32}}));
        auto make=[&](gui::Widget p,int role,std::string suffix,double height){return take(AlertButton::create(b,p,s->id+suffix,{.text=s->label(role),.dialog_style=true,.tone=role==1?gui::ButtonTone::primary:role==3?gui::ButtonTone::borderless:gui::ButtonTone::normal,.height=height,.horizontal_padding=12},{.method=[weak,role](const ButtonEvent& event){if(auto s=weak.lock()){auto m=s->methods[std::size_t(role-1)];if(m)m(event);}},.clicked=[weak,role]{if(auto s=weak.lock()){auto action=role==1?s->events.primary_clicked:role==2?s->events.secondary_clicked:s->events.tertiary_clicked;if(action)action();}}}));};
        for(int i=1;i<=3;++i)s->v[std::size_t(i-1)]=make(s->vertical,i,".vertical."+std::to_string(i),i==3?44:56);s->h[1]=make(s->horizontal,2,".horizontal.2",56);s->h[0]=make(s->horizontal,1,".horizontal.1",56);good(s->h[0].root().set_flex(1));good(s->h[1].root().set_flex(1));s->one=make(s->single,1,".single.1",56);good(b.retain(s));good(s->apply());Alert alert;alert.root_=s->root;alert.state_=s;return alert;
    }catch(const BuildError& e){return std::unexpected(e.error);}
}
ii::core::Result<std::shared_ptr<Alert::Impl>> Alert::lock()const{auto r=root_.snapshot();if(!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Alert retired");if(s->updating)return ii::core::fail(ii::core::ErrorCode::busy,"Alert is updating");return s;}
ii::core::Result<void> Alert::open()const{return root_.open_at(0,0);}
ii::core::Result<void> Alert::close()const{return root_.close_popup();}
ii::core::Result<bool> Alert::dismiss()const{auto r=root_.snapshot();if(!r)return std::unexpected(r.error());if(!r->opened)return false;auto closed=root_.cancel_popup();if(!closed)return std::unexpected(closed.error());return true;}
ii::core::Result<int> Alert::resolved_button_count()const{auto s=lock();if(!s)return std::unexpected(s.error());return (*s)->count();}
ii::core::Result<ii::gui::Widget> Alert::action_button(int i)const{auto s=lock();if(!s)return std::unexpected(s.error());if(i<1||i>(*s)->count())return ii::core::fail(ii::core::ErrorCode::not_found,"Alert action is not present");return (*s)->button(i).root();}
ii::core::Result<void> Alert::set_text(std::string title,std::string message)const{if(!text(title)||!text(message))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid Alert text");auto s=lock();if(!s)return std::unexpected(s.error());(*s)->options.title=std::move(title);(*s)->options.message=std::move(message);return (*s)->apply();}
ii::core::Result<void> Alert::set_button_count(int n)const{auto s=lock();if(!s)return std::unexpected(s.error());(*s)->options.button_count=n;return (*s)->apply();}
ii::core::Result<void> Alert::set_action_text(int i,std::string v)const{if(i<1||i>3||!text(v))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid Alert action text");auto s=lock();if(!s)return std::unexpected(s.error());auto& o=(*s)->options;(i==1?o.primary_text:i==2?o.secondary_text:o.tertiary_text)=std::move(v);return (*s)->apply();}
ii::core::Result<void> Alert::set_action_enabled(int i,bool v)const{if(i<1||i>3)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid Alert action index");auto s=lock();if(!s)return std::unexpected(s.error());auto& o=(*s)->options;(i==1?o.primary_enabled:i==2?o.secondary_enabled:o.tertiary_enabled)=v;return (*s)->apply();}
ii::core::Result<void> Alert::set_action_method(int i,ButtonMethod v)const{if(i<1||i>3)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid Alert action index");auto s=lock();if(!s)return std::unexpected(s.error());std::swap((*s)->methods[std::size_t(i-1)],v);return {};}
ii::core::Result<void> Alert::set_primary_color(ii::gui::WidgetColor c)const{if(!rgba(c))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid primary color");auto s=lock();if(!s)return std::unexpected(s.error());(*s)->primary=c;return (*s)->apply();}
ii::core::Result<void> Alert::set_show_icon(bool v)const{auto s=lock();if(!s)return std::unexpected(s.error());(*s)->options.show_icon=v;return (*s)->apply();}
ii::core::Result<void> Alert::set_glass_enabled(bool v)const{auto s=lock();if(!s)return std::unexpected(s.error());(*s)->options.glass_enabled=v;return (*s)->apply();}
ii::core::Result<void> Alert::set_shadow_enabled(bool v)const{auto s=lock();if(!s)return std::unexpected(s.error());(*s)->options.shadow_enabled=v;return (*s)->apply();}
ii::core::Result<void> Alert::set_motion_enabled(bool v)const{auto s=lock();if(!s)return std::unexpected(s.error());(*s)->options.motion_enabled=v;return (*s)->apply();}

}
