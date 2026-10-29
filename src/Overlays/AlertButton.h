#pragma once
#include <LVRS/ButtonMethod.h>
namespace lvrs {
struct AlertButtonOptions final {
    std::string text="Button";
    bool enabled=true,dialog_style=false;
    ii::gui::ButtonTone tone=ii::gui::ButtonTone::normal;
    std::optional<double> height,horizontal_padding,corner_radius;
    std::optional<ii::gui::WidgetColor> text_color,primary_color;
};
struct AlertButtonEvents final {ButtonMethod method;ii::runtime::Task clicked,pressed,released,canceled;};
class AlertButton final {
public:
    AlertButton()=default;
    [[nodiscard]] static ii::core::Result<AlertButton> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,AlertButtonOptions={},AlertButtonEvents={});
    [[nodiscard]] ii::gui::Widget root() const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<void> set_text(std::string_view) const;
    [[nodiscard]] ii::core::Result<void> set_enabled(bool) const;
    [[nodiscard]] ii::core::Result<void> set_method(ButtonMethod) const;
    [[nodiscard]] ii::core::Result<void> set_text_color(ii::gui::WidgetColor) const;
    [[nodiscard]] ii::core::Result<void> set_primary_color(ii::gui::WidgetColor) const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock() const;
    ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
