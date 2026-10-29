#pragma once
#include <iiGUI/Appearance.h>
#include <array>

namespace lvrs {
enum class Accent { blue, purple, rose, red, orange, yellow, green, gray, brown, cyan, teal, white };
enum class AccentRole { standard, hover, pressed, selected, disabled, subtle, outline, overlay, text };
class Theme final {
public:
    explicit Theme(Accent accent=Accent::blue):accent_(accent) {}
    [[nodiscard]] static ii::gui::WidgetColor accent_color(Accent);
    [[nodiscard]] static ii::gui::WidgetColor accent_role(Accent,AccentRole);
    [[nodiscard]] ii::gui::WidgetAppearance appearance() const;
    [[nodiscard]] Accent accent() const noexcept { return accent_; }
private:
    Accent accent_;
};
} // namespace lvrs
