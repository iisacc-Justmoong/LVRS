#include "Link.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <cmath>
#include <stdexcept>

namespace lvrs {
using namespace ii;
namespace {
bool color_valid(const std::optional<gui::WidgetColor>& color){if(!color)return true;for(auto n:{color->red,color->green,color->blue,color->alpha})if(!std::isfinite(n)||n<0||n>1)return false;return true;}
bool href_valid(std::string_view href){return href.size()<=RouteLimits::path_bytes&&href.find('\0')==href.npos&&core::valid_utf8(href);}
void raise(core::Error error){if(error.cause)std::rethrow_exception(error.cause);throw std::runtime_error(error.message);}
core::Result<core::Value> parameters(core::Value value){NavigationStackModel admission;auto descriptor=admission.create_component_path_entry({},std::move(value));if(!descriptor)return std::unexpected(descriptor.error());return std::move(descriptor->params);}
}
struct Link::Impl final {
    LinkOptions options;LinkEvents events;gui::Widget root;
    gui::WidgetColor primary;
    bool triggering=false;
    gui::ActionOptions visual()const{
        const auto base=options.link_color.value_or(primary);
        return {.text=options.text,.accessibility_name=options.accessibility_name,.enabled=options.enabled,.underline=options.underline,.focus_ring=options.show_focus_ring,
            .role=gui::ActionRole::link,.text_role=gui::TextRole::description,.font_size=12,.line_height=12,.padding={options.horizontal_padding,options.vertical_padding,options.horizontal_padding,options.vertical_padding},
            .minimum_height=options.minimum_height,.corner_radius=options.corner_radius,.width=options.width,.height=options.height,
            .text_color=base,.hover_text_color=options.hover_color.value_or(gui::WidgetColor{1,1,1,.9}),.pressed_text_color=options.pressed_color.value_or(gui::WidgetColor{base.red/1.2,base.green/1.2,base.blue/1.2,base.alpha}),.disabled_text_color=options.disabled_color.value_or(gui::WidgetColor{1,1,1,.6}),.interaction_enabled=options.tone!=gui::ButtonTone::disabled};
    }
    core::Result<bool> dispatch(){
        if(triggering)return core::fail(core::ErrorCode::busy,"Link navigation is already in progress");
        struct Guard final {bool& value;explicit Guard(bool& v):value(v){value=true;}~Guard(){value=false;}}guard{triggering};
        auto target=options.router.lock();auto navigator=options.navigator;auto page=options.target_component;auto params=options.params;const auto href=options.href;
        const auto operation=options.replace?NavigationOperation::replace:NavigationOperation::push;
        try{
            if(!page&&href.empty())return false;
            if(target)return page?target->go_to(std::move(page),std::move(params),operation):target->go(href,std::move(params),operation);
            if(!navigator)navigator=Navigator::current();
            return page?navigator->go_to(std::move(page),std::move(params),operation):navigator->go(href,std::move(params),operation);
        }catch(...){return core::fail(core::ErrorCode::failed,"Link navigation target failed",std::current_exception());}
    }
    core::Result<void> refresh_colors(){
        auto current=root.action_options();if(!current)return std::unexpected(current.error());const auto style=visual();
        current->text_color=style.text_color;current->hover_text_color=style.hover_text_color;current->pressed_text_color=style.pressed_text_color;current->disabled_text_color=style.disabled_text_color;
        return root.set_action_options(std::move(*current));
    }
};
core::Result<Link> Link::create(gui::WidgetBuilder& builder,gui::Widget parent,std::string_view id,LinkOptions options,LinkEvents events,LinkContentFactory content){
    if(!href_valid(options.href)||unsigned(options.tone)>4||events.methods.size()>256||!color_valid(options.link_color)||!color_valid(options.hover_color)||!color_valid(options.pressed_color)||!color_valid(options.disabled_color))return core::fail(core::ErrorCode::invalid_argument,"Invalid Link route, methods, tone or color");
    auto params=parameters(std::move(options.params));if(!params)return std::unexpected(params.error());options.params=std::move(*params);
    auto appearance=builder.appearance();if(!appearance)return std::unexpected(appearance.error());
    auto state=std::make_shared<Impl>();state->options=std::move(options);state->events=std::move(events);state->primary=appearance->primary;std::weak_ptr<Impl> weak=state;
    auto root=builder.action_surface(parent,id,state->visual(),[weak]{
        auto state=weak.lock();if(!state)return;const auto source=state->root;
        auto failed=state->events.navigation_failed;auto navigated=state->events.navigated;auto method=state->events.method;auto methods=state->events.methods;auto clicked=state->events.clicked;
        auto result=state->dispatch();
        if(!result){if(failed)failed(result.error());else raise(result.error());}
        else if(navigated)navigated(*result);
        if(!source.snapshot())return;if(method)method({source,"clicked"});
        for(const auto& callback:methods){if(!source.snapshot())return;if(callback)callback({source,"clicked"});}
        if(source.snapshot()&&clicked)clicked();
    });
    if(!root)return std::unexpected(root.error());state->root=*root;
    if(auto retained=builder.retain(state);!retained)return std::unexpected(retained.error());
    if(auto attached=root->set_control_events({.pressed=state->events.pressed,.released=state->events.released,.canceled=state->events.canceled});!attached)return std::unexpected(attached.error());
    if(content){try{auto built=content(builder,*root);if(!built)return std::unexpected(built.error());}catch(...){return core::fail(core::ErrorCode::failed,"Link content factory failed",std::current_exception());}}
    if(auto live=root->snapshot();!live)return std::unexpected(live.error());Link link;link.root_=*root;link.state_=state;return link;
}
core::Result<std::shared_ptr<Link::Impl>> Link::lock()const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto state=state_.lock();if(!state)return core::fail(core::ErrorCode::closed,"Link scope retired");return state;}
core::Result<bool> Link::trigger_navigation()const{auto state=lock();if(!state)return std::unexpected(state.error());return (*state)->dispatch();}
core::Result<LinkOptions> Link::options()const{auto state=lock();if(!state)return std::unexpected(state.error());auto result=(*state)->options;auto native=root_.action_options();if(!native)return std::unexpected(native.error());result.text=native->text;result.enabled=native->enabled;result.underline=native->underline;return result;}
core::Result<void> Link::set_text(std::string_view text)const{return root_.set_text(text);}
core::Result<void> Link::set_href(std::string_view href)const{auto state=lock();if(!state)return std::unexpected(state.error());if(!href_valid(href))return core::fail(core::ErrorCode::invalid_argument,"Link href requires bounded UTF-8 without NUL");(*state)->options.href=href;return {};}
core::Result<void> Link::set_params(core::Value params)const{auto state=lock();if(!state)return std::unexpected(state.error());auto next=parameters(std::move(params));if(!next)return std::unexpected(next.error());std::swap((*state)->options.params,*next);return {};}
core::Result<void> Link::set_router(std::weak_ptr<NavigationTarget> target)const{auto state=lock();if(!state)return std::unexpected(state.error());(*state)->options.router=std::move(target);return {};}
core::Result<void> Link::set_navigator(std::shared_ptr<Navigator> navigator)const{auto state=lock();if(!state)return std::unexpected(state.error());std::swap((*state)->options.navigator,navigator);return {};}
core::Result<void> Link::set_target_component(PageComponentRef page)const{auto state=lock();if(!state)return std::unexpected(state.error());std::swap((*state)->options.target_component,page);return {};}
core::Result<void> Link::set_replace(bool replace)const{auto state=lock();if(!state)return std::unexpected(state.error());(*state)->options.replace=replace;return {};}
core::Result<void> Link::set_enabled(bool enabled)const{return root_.set_enabled(enabled);}
core::Result<void> Link::set_tone(gui::ButtonTone tone)const{auto state=lock();if(!state)return std::unexpected(state.error());if(unsigned(tone)>4)return core::fail(core::ErrorCode::invalid_argument,"Invalid Link tone");auto native=root_.action_options();if(!native)return std::unexpected(native.error());native->interaction_enabled=tone!=gui::ButtonTone::disabled;auto result=root_.set_action_options(std::move(*native));if(result)(*state)->options.tone=tone;return result;}
core::Result<void> Link::set_underline(bool underline)const{auto native=root_.action_options();if(!native)return std::unexpected(native.error());native->underline=underline;return root_.set_action_options(std::move(*native));}
core::Result<void> Link::set_colors(std::optional<gui::WidgetColor> base,std::optional<gui::WidgetColor> hover,std::optional<gui::WidgetColor> pressed,std::optional<gui::WidgetColor> disabled)const{
    auto state=lock();if(!state)return std::unexpected(state.error());if(!color_valid(base)||!color_valid(hover)||!color_valid(pressed)||!color_valid(disabled))return core::fail(core::ErrorCode::invalid_argument,"Invalid Link color");
    auto& options=(*state)->options;options.link_color=base;options.hover_color=hover;options.pressed_color=pressed;options.disabled_color=disabled;return (*state)->refresh_colors();
}
core::Result<void> Link::set_method(ButtonMethod method)const{auto state=lock();if(!state)return std::unexpected(state.error());std::swap((*state)->events.method,method);return {};}
core::Result<void> Link::set_link_color(std::optional<gui::WidgetColor> color)const{auto state=lock();if(!state)return std::unexpected(state.error());const auto& o=(*state)->options;return set_colors(color,o.hover_color,o.pressed_color,o.disabled_color);}
core::Result<void> Link::set_hover_color(std::optional<gui::WidgetColor> color)const{auto state=lock();if(!state)return std::unexpected(state.error());const auto& o=(*state)->options;return set_colors(o.link_color,color,o.pressed_color,o.disabled_color);}
core::Result<void> Link::set_pressed_color(std::optional<gui::WidgetColor> color)const{auto state=lock();if(!state)return std::unexpected(state.error());const auto& o=(*state)->options;return set_colors(o.link_color,o.hover_color,color,o.disabled_color);}
core::Result<void> Link::set_disabled_color(std::optional<gui::WidgetColor> color)const{auto state=lock();if(!state)return std::unexpected(state.error());const auto& o=(*state)->options;return set_colors(o.link_color,o.hover_color,o.pressed_color,color);}
core::Result<void> Link::set_methods(std::vector<ButtonMethod> methods)const{auto state=lock();if(!state)return std::unexpected(state.error());if(methods.size()>256)return core::fail(core::ErrorCode::full,"Link method limit is 256");std::swap((*state)->events.methods,methods);return {};}
}
