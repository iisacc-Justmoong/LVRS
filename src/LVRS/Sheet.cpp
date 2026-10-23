#include "Sheet.h"
#include "Icons.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <cmath>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
namespace lvrs {
namespace {
using namespace ii;
struct BuildError final{core::Error error;};
template<class T>T take(core::Result<T> value){if(!value)throw BuildError{value.error()};return std::move(*value);}
void good(core::Result<void> value){if(!value)throw BuildError{value.error()};}
bool metric(double v){return std::isfinite(v)&&v>=0&&v<=8192;}
bool rgba(gui::WidgetColor c){return metric(c.red)&&c.red<=1&&metric(c.green)&&c.green<=1&&metric(c.blue)&&c.blue<=1&&metric(c.alpha)&&c.alpha<=1;}
bool valid(const SheetOptions& o){
    for(const auto* s:{&o.title,&o.description,&o.accessibility_name})if(s->size()>65536||s->find('\0')!=s->npos||!core::valid_utf8(*s))return false;
    return unsigned(o.presentation)<=2&&unsigned(o.detent)<=2&&(!o.corner_radius||metric(*o.corner_radius))&&metric(o.preferred_width)&&metric(o.preferred_height)&&metric(o.desktop_margin)&&metric(o.content_padding)&&metric(o.top_safe_inset)&&metric(o.bottom_safe_inset)&&o.content_padding+o.bottom_safe_inset<=8192&&o.animation_duration>=std::chrono::milliseconds::zero()&&o.animation_duration<=std::chrono::hours(24)&&rgba(o.surface_color)&&rgba(o.backdrop_color);
}
bool mobile(const SheetOptions& o){
#if defined(__ANDROID__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    constexpr bool target=true;
#else
    constexpr bool target=false;
#endif
    return o.presentation==SheetPresentation::mobile||(o.presentation==SheetPresentation::automatic&&target);
}
}
struct Sheet::Impl final {
    SheetOptions options;SheetEvents events;std::string id;
    ii::gui::Widget root,view,grabber,header,row,title,description,close_slot,viewport,body_wrap,content;
    ii::gui::PopoverOptions surface(const SheetOptions& o)const {
        const auto m=mobile(o);auto transition=modal_transition(o.motion,o.motion_enabled);transition.enter_duration=transition.exit_duration=o.animation_duration;transition.collapsed_scale=m?1:.94;
        return {.width=o.preferred_width,.padding=0,.margins=o.desktop_margin,.corner_radius=o.corner_radius.value_or(m?24:16),.surface_opacity=1,.surface_color=o.surface_color,.modal=true,.dim=true,.clip_content=true,.dismiss_on_escape=o.dismiss_on_escape,.dismiss_on_outside_press=o.dismiss_on_background,.accessibility_name=o.accessibility_name,.placement=ii::gui::PopoverPlacement::centered,.backdrop_color=o.backdrop_color,.fit_to_viewport=true,.opaque_base=false,.transition=std::move(transition),.sheet_layout=ii::gui::SheetLayoutOptions{.mobile=m,.detent=o.detent,.preferred_height=o.preferred_height,.desktop_margin=o.desktop_margin,.top_safe_inset=o.top_safe_inset,.viewport_id=id+".viewport",.grabber_id=id+".grabber",.dismiss_on_drag=o.dismiss_on_drag}};
    }
    ii::core::Result<void> apply(SheetOptions o){
        using namespace ii;const auto m=mobile(o);
        try {
            good(root.set_popover_options(surface(o)));
            good(view.set_stack_options({.spacing=0,.alignment=gui::StackAlignment::stretch,.insets=gui::Insets{0,0,0,o.content_padding+(m?o.bottom_safe_inset:0)}}));
            good(grabber.set_visible(o.show_grabber.value_or(m)));
            good(header.set_stack_options({.spacing=0,.alignment=gui::StackAlignment::stretch,.insets=gui::Insets{o.content_padding,m?0.:24.,o.content_padding,o.show_header?16.:0.}}));
            good(row.set_visible(o.show_header));good(row.set_stack_options({.spacing=12,.alignment=o.show_close_button?gui::StackAlignment::center:gui::StackAlignment::leading}));
            good(title.set_text(o.title));good(title.set_visible(!o.title.empty()));good(description.set_text(o.description));good(description.set_visible(o.show_description&&!o.description.empty()));good(close_slot.set_visible(o.show_close_button));
            good(body_wrap.set_stack_options({.spacing=0,.alignment=gui::StackAlignment::stretch,.insets=gui::Insets{o.content_padding,0,o.content_padding,0}}));
            good(viewport.set_scroll_options({.enabled=o.scroll_content}));options=std::move(o);return {};
        }catch(const BuildError& error){return std::unexpected(error.error);}
    }
};
ii::core::Result<Sheet> Sheet::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,std::string_view id,SheetOptions o,SheetEvents events,SheetContentFactory content){
    using namespace ii;if(!valid(o))return core::fail(core::ErrorCode::invalid_argument,"Invalid Sheet text, geometry, presentation or color");
    try {
        auto s=std::make_shared<Impl>();s->id=id;s->events=std::move(events);s->options=o;
        s->root=take(b.popover(parent,id,s->surface(o),{.opened=s->events.opened,.closed=s->events.closed}));
        s->view=take(b.vstack(s->root,s->id+".view",{.spacing=0,.alignment=gui::StackAlignment::stretch}));
        s->grabber=take(b.hstack(s->view,s->id+".grabber",{.spacing=0,.insets=gui::Insets{0,12,0,12}}));take(b.spacer(s->grabber,s->id+".grabberLeft"));
        auto grabber=take(gui::svg_image("<svg xmlns='http://www.w3.org/2000/svg' width='36' height='4' viewBox='0 0 36 4'><rect width='36' height='4' rx='2' fill='white' fill-opacity='.6'/></svg>",72,8));
        take(b.image(s->grabber,s->id+".grabberInk",{.image=grabber,.width=36,.height=4}));take(b.spacer(s->grabber,s->id+".grabberRight"));
        s->header=take(b.vstack(s->view,s->id+".header",{.spacing=0,.alignment=gui::StackAlignment::stretch}));
        s->row=take(b.hstack(s->header,s->id+".headerRow",{.spacing=12}));auto labels=take(b.vstack(s->row,s->id+".labels",{.spacing=8,.alignment=gui::StackAlignment::stretch}));good(labels.set_flex(1));
        s->title=take(b.label(labels,s->id+".title",{.text=o.title,.font_size=17,.role=gui::TextRole::header}));s->description=take(b.label(labels,s->id+".description",{.text=o.description,.font_size=13,.role=gui::TextRole::body,.color=gui::WidgetColor{1,1,1,.6}}));
        s->close_slot=take(b.absolute_stack(s->row,s->id+".closeSlot",{44,44}));std::weak_ptr<Impl> weak=s;
        auto close=take(b.button(s->close_slot,s->id+".close",{.text="",.tone=gui::ButtonTone::normal,.icon=take(Icons::image("generalclose")),.accessibility_name="Close sheet",.icon_size=16,.icon_mode=true,.height=44,.corner_radius=16,.style=gui::ButtonStyle{.fill=gui::WidgetColor{.1753742844,.1819191724,.1841008067,1},.hover_fill=gui::WidgetColor{.1753742844,.1819191724,.1841008067,1},.pressed_fill=gui::WidgetColor{.1935568452,.1978072077,.1999323964,1},.text=gui::WidgetColor{1,1,1,.6}}},[weak]{if(auto state=weak.lock()){auto r=state->root.close_popup();if(!r)throw std::runtime_error(r.error().message);}}));good(close.set_frame({0,0,44,44}));
        s->body_wrap=take(b.vstack(s->view,s->id+".bodyFrame",{.spacing=0,.alignment=gui::StackAlignment::stretch}));good(s->body_wrap.set_flex(1));
        s->viewport=take(b.scroll_view(s->body_wrap,s->id+".viewport"));good(s->viewport.set_flex(1));s->content=take(b.zstack(s->viewport,s->id+".content",{.spacing=0,.alignment=gui::StackAlignment::leading}));
        if(content)good(content(b,s->content));good(b.retain(s));good(s->apply(std::move(o)));
        Sheet result;result.root_=s->root;result.viewport_=s->viewport;result.content_=s->content;result.state_=s;return result;
    }catch(const BuildError& error){return std::unexpected(error.error);}
}
ii::core::Result<void> Sheet::open()const{return root_.open_at(0,0);}
ii::core::Result<void> Sheet::close()const{return root_.close_popup();}
ii::core::Result<void> Sheet::set_options(SheetOptions value)const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto state=state_.lock();if(!state)return ii::core::fail(ii::core::ErrorCode::closed,"Sheet owner retired");if(!valid(value))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid Sheet options");return state->apply(std::move(value));}
}
