#pragma once
#include <LVRS/ButtonMethod.h>
#include <LVRS/Navigator.h>
namespace lvrs {
struct LinkOptions final {
    std::string text,href="/";
    ii::core::Value params;
    std::weak_ptr<NavigationTarget> router;
    std::shared_ptr<Navigator> navigator;
    PageComponentRef target_component;
    bool replace=false,enabled=true,underline=false;
    ii::gui::ButtonTone tone=ii::gui::ButtonTone::normal;
    std::optional<ii::gui::WidgetColor> link_color,hover_color,pressed_color,disabled_color;
    double horizontal_padding=14,vertical_padding=10,minimum_height=36,corner_radius=8;
    std::optional<double> width,height;
    bool show_focus_ring=true;
    std::string accessibility_name;
};
struct LinkEvents final {
    ButtonMethod method;
    std::vector<ButtonMethod> methods;
    ii::runtime::Task clicked,pressed,released,canceled;
    std::function<void(bool)> navigated;
    std::function<void(const ii::core::Error&)> navigation_failed;
};
using LinkContentFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget)>;
class Link final {
public:
    Link()=default;
    [[nodiscard]] static ii::core::Result<Link> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,LinkOptions={},LinkEvents={},LinkContentFactory={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<bool> trigger_navigation()const;
    [[nodiscard]] ii::core::Result<LinkOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_text(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> set_href(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> set_to(std::string_view href)const{return set_href(href);}
    [[nodiscard]] ii::core::Result<void> set_params(ii::core::Value)const;
    [[nodiscard]] ii::core::Result<void> set_router(std::weak_ptr<NavigationTarget>)const;
    [[nodiscard]] ii::core::Result<void> set_navigator(std::shared_ptr<Navigator>)const;
    [[nodiscard]] ii::core::Result<void> set_target_component(PageComponentRef)const;
    [[nodiscard]] ii::core::Result<void> set_replace(bool)const;
    [[nodiscard]] ii::core::Result<void> set_enabled(bool)const;
    [[nodiscard]] ii::core::Result<void> set_tone(ii::gui::ButtonTone)const;
    [[nodiscard]] ii::core::Result<void> set_underline(bool)const;
    [[nodiscard]] ii::core::Result<void> set_colors(std::optional<ii::gui::WidgetColor>,std::optional<ii::gui::WidgetColor> = {},std::optional<ii::gui::WidgetColor> = {},std::optional<ii::gui::WidgetColor> = {})const;
    [[nodiscard]] ii::core::Result<void> set_link_color(std::optional<ii::gui::WidgetColor>)const;
    [[nodiscard]] ii::core::Result<void> set_hover_color(std::optional<ii::gui::WidgetColor>)const;
    [[nodiscard]] ii::core::Result<void> set_pressed_color(std::optional<ii::gui::WidgetColor>)const;
    [[nodiscard]] ii::core::Result<void> set_disabled_color(std::optional<ii::gui::WidgetColor>)const;
    [[nodiscard]] ii::core::Result<void> set_method(ButtonMethod)const;
    [[nodiscard]] ii::core::Result<void> set_methods(std::vector<ButtonMethod>)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
