#pragma once
#include <iiGUI/Appearance.h>
#include <array>

namespace lvrs {
enum class Accent { blue, purple, rose, red, orange, yellow, green, gray, brown, cyan, teal, white };
enum class AccentRole { standard, hover, pressed, selected, disabled, subtle, outline, overlay, text };
enum class BackgroundRole {
    canvas, surface_ghost, window_alt, sub_surface, surface_solid, surface_alt,
    surface_interactive, surface_panel, surface_control, surface_input,
    surface_input_hover, surface_raised, glass_fill, text_field_fill,
    text_field_disabled, text_field_inline, dense_fill, surface_glass_fill,
    backdrop_base, window, alert_canvas, alert_surface
};
class Theme final {
public:
    explicit Theme(Accent accent=Accent::blue):accent_(accent) {}
    [[nodiscard]] static ii::gui::WidgetColor accent_color(Accent);
    [[nodiscard]] static ii::gui::WidgetColor accent_role(Accent,AccentRole);
    [[nodiscard]] static ii::gui::WidgetColor icon_accent_role(Accent,AccentRole);
    [[nodiscard]] static ii::gui::WidgetColor background(BackgroundRole);
    [[nodiscard]] ii::gui::WidgetAppearance appearance() const;
    [[nodiscard]] Accent accent() const noexcept { return accent_; }
private:
    Accent accent_;
};
} // namespace lvrs
