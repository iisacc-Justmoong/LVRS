#include <LVRS/Modal.h>
#include <LVRS/AlertButton.h>
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <cmath>
namespace lvrs {
namespace {
using namespace ii;
struct BuildError final {core::Error error;};
template<class T>T take(core::Result<T> r){if(!r)throw BuildError{r.error()};return std::move(*r);}
void good(core::Result<void> r){if(!r)throw BuildError{r.error()};}
bool metric(double n){return std::isfinite(n)&&n>=0&&n<=8192;}
bool rgba(gui::WidgetColor c){return metric(c.red)&&metric(c.green)&&metric(c.blue)&&metric(c.alpha)&&c.red<=1&&c.green<=1&&c.blue<=1&&c.alpha<=1;}
std::string trim(std::string_view v){auto first=v.find_first_not_of(" \t\r\n");return first==v.npos?std::string{}:std::string(v.substr(first,v.find_last_not_of(" \t\r\n")-first+1));}
}
struct Modal::Impl final {
    ModalOptions options;ModalEvents events;ModalContentFactory content;ii::gui::Widget root,body;std::string id;int count=0;
    std::shared_ptr<const ii::gui::RasterImage> icon;
    ii::gui::PopoverOptions surface()const {
        using namespace ii;const auto& q=options;
        return {.width=q.preferred_width,.padding=20,.margins=q.side_padding,.corner_radius=q.corner_radius,.surface_opacity=1,.surface_color=q.frame_color,.modal=true,.dim=true,.dismiss_on_outside_press=q.dismiss_on_background,.accessibility_name=q.accessibility_name,.placement=gui::PopoverPlacement::centered,.minimum_width=q.minimum_width,.maximum_width=q.maximum_width,.minimum_height=q.minimum_height,.vertical_offset=q.vertical_offset,.cylinder=q.shape==ModalShape::cylinder,.backdrop_color=q.backdrop_color,.opaque_base=false,.transition=modal_transition(q.motion,q.motion_enabled)};
    }
    ii::core::Result<void> build_chrome(ii::gui::WidgetBuilder& b){
        using namespace ii;
        try {
            auto view=take(b.vstack(root,id+".view",{.spacing=20,.alignment=gui::StackAlignment::stretch}));
            auto header=take(b.hstack(view,id+".header",{.spacing=16,.alignment=gui::StackAlignment::leading}));
            if(options.show_icon)take(b.image(header,id+".icon",{.image=icon,.width=options.icon_size,.height=options.icon_size,.accessibility_name=options.icon_name,.padding=std::min(8.,options.icon_size/2),.corner_radius=options.icon_corner_radius,.border_width=1,.background=options.icon_background,.border=options.icon_border,.preserve_aspect=true}));
            auto texts=take(b.vstack(header,id+".texts",{.spacing=8,.alignment=gui::StackAlignment::leading}));good(texts.set_flex(1));
            if(!options.title.empty())take(b.label(texts,id+".title",{.text=options.title,.font_size=22,.role=gui::TextRole::title2,.line_height=22}));
            const auto& description=options.description.empty()?options.message:options.description;
            if(!description.empty())take(b.label(texts,id+".description",{.text=description,.font_size=13,.role=gui::TextRole::body,.line_height=18}));
            if(!options.show_icon&&options.title.empty()&&description.empty())good(header.set_visible(false));
            if(content)body=take(b.vstack(view,id+".bodySlot",{.spacing=0,.alignment=gui::StackAlignment::stretch}));
            if(count){auto actions=take(b.hstack(view,id+".actions",{.spacing=12,.alignment=gui::StackAlignment::leading}));take(b.spacer(actions,id+".actionSpacer"));
                if(count>=3)take(AlertButton::create(b,actions,id+".tertiary",{.text=options.tertiary_text.empty()?"More":options.tertiary_text,.enabled=options.tertiary_enabled,.tone=gui::ButtonTone::normal,.height=20,.corner_radius=4},{.clicked=events.tertiary_clicked}));
                if(count>=2)take(AlertButton::create(b,actions,id+".secondary",{.text=options.secondary_text.empty()?"Cancel":options.secondary_text,.enabled=options.secondary_enabled,.tone=gui::ButtonTone::normal,.height=20,.corner_radius=4},{.clicked=events.secondary_clicked}));
                take(AlertButton::create(b,actions,id+".primary",{.text=options.primary_text.empty()?"OK":options.primary_text,.enabled=options.primary_enabled,.tone=gui::ButtonTone::primary,.height=20,.corner_radius=4},{.clicked=events.primary_clicked}));
            }
            return {};
        }catch(const BuildError& e){return std::unexpected(e.error);}
    }
};
ii::core::Result<Modal> Modal::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,std::string_view id,ModalOptions o,ModalEvents events,ModalContentFactory content){
    using namespace ii;
    for(const auto* text:{&o.title,&o.description,&o.message,&o.icon_name,&o.primary_text,&o.secondary_text,&o.tertiary_text,&o.accessibility_name})if(text->size()>65536||text->find('\0')!=text->npos||!core::valid_utf8(*text))return core::fail(core::ErrorCode::invalid_argument,"Modal text must be bounded UTF-8 without NUL");
    if(!metric(o.icon_size)||o.icon_size<1||!metric(o.icon_corner_radius)||!metric(o.minimum_width)||!metric(o.maximum_width)||!o.maximum_width||o.minimum_width>o.maximum_width||!metric(o.preferred_width)||!o.preferred_width||!metric(o.side_padding)||!metric(o.minimum_height)||!std::isfinite(o.vertical_offset)||std::abs(o.vertical_offset)>32767||!metric(o.corner_radius)||static_cast<unsigned>(o.shape)>1||!rgba(o.frame_color)||!rgba(o.backdrop_color)||!rgba(o.icon_background)||!rgba(o.icon_border))return core::fail(core::ErrorCode::invalid_argument,"Invalid Modal geometry, shape or color");
    try {
        auto s=std::make_shared<Impl>();s->options=std::move(o);s->options.icon_name=trim(s->options.icon_name);s->events=std::move(events);s->content=std::move(content);s->id=id;
        s->count=s->options.button_count>0?std::clamp(s->options.button_count,1,3):!s->options.tertiary_text.empty()?3:!s->options.secondary_text.empty()?2:!s->options.primary_text.empty()?1:0;
        if(s->options.show_icon){s->icon=s->options.icon_source;if(!s->icon){if(!s->options.icon_name.empty())s->icon=take(Icons::image(s->options.icon_name));else s->icon=take(gui::svg_image("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 56 56'><rect x='18' y='18' width='20' height='20' rx='4' fill='#b4b8bf' opacity='.72'/></svg>",112,112));}}
        auto popup=take(b.popover(parent,id,s->surface(),{.opened=s->events.opened,.closed=s->events.closed,.canceled=s->events.canceled}));
        s->root=popup;good(s->build_chrome(b));good(b.retain(s));
        if(s->content){std::weak_ptr<Impl> weak=s;good(popup.set_popup_content_at(s->body,[weak](gui::WidgetBuilder& child)->core::Result<void>{
            auto state=weak.lock();if(!state)return core::fail(core::ErrorCode::closed,"Modal owner retired");
            auto parent=child.vstack({},state->id+".content",{.spacing=0,.alignment=gui::StackAlignment::stretch});if(!parent)return std::unexpected(parent.error());return state->content(child,*parent);
        }));}
        Modal modal;modal.root_=popup;modal.state_=s;return modal;
    }catch(const BuildError& e){return std::unexpected(e.error);}
}
ii::core::Result<std::shared_ptr<Modal::Impl>> Modal::lock() const {auto r=root_.snapshot();if(!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Modal owner retired");return s;}
ii::core::Result<void> Modal::open() const{return root_.open_at(0,0);}
ii::core::Result<void> Modal::close() const{return root_.close_popup();}
ii::core::Result<bool> Modal::cancel() const {auto r=root_.snapshot();if(!r)return std::unexpected(r.error());if(!r->opened)return false;auto closed=root_.cancel_popup();if(!closed)return std::unexpected(closed.error());return true;}
ii::core::Result<bool> Modal::contains_frame_point(double x,double y) const {auto r=root_.snapshot();if(!r)return std::unexpected(r.error());if(!std::isfinite(x)||!std::isfinite(y))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Modal point must be finite");const auto a=r->bounds;return x>=a.x&&y>=a.y&&x<=a.x+a.width&&y<=a.y+a.height;}
ii::core::Result<bool> Modal::action_visible(int index) const {auto s=lock();if(!s)return std::unexpected(s.error());return index>=1&&index<=(*s)->count;}
ii::core::Result<bool> Modal::trigger_action(int index) const {auto s=lock();if(!s)return std::unexpected(s.error());if(index<1||index>(*s)->count)return false;const auto& o=(*s)->options;if(!(index==1?o.primary_enabled:index==2?o.secondary_enabled:o.tertiary_enabled))return false;auto action=index==1?(*s)->events.primary_clicked:index==2?(*s)->events.secondary_clicked:(*s)->events.tertiary_clicked;if(action)action();return true;}
ii::core::Result<void> Modal::set_motion_enabled(bool value)const {auto s=lock();if(!s)return std::unexpected(s.error());(*s)->options.motion_enabled=value;return root_.set_popover_options((*s)->surface());}
ii::core::Result<ii::gui::Widget> Modal::content_item() const {auto s=lock();if(!s)return std::unexpected(s.error());if(!(*s)->content)return ii::core::fail(ii::core::ErrorCode::not_found,"Modal has no content factory");return root_.popup_content((*s)->id+".content");}
}
