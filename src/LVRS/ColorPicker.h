#pragma once
#include <iiGUI/ColorPicker.h>
#include <iiGUI/Image.h>
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
using ColorPickerModel=ii::gui::ColorPickerModel;
using ColorChannel=ii::gui::ColorChannel;
using ColorField=ii::gui::ColorField;
// The original Figma PNG is compiled into LVRS; no runtime source/resource path.
[[nodiscard]] ii::core::Result<std::shared_ptr<const ii::gui::RasterImage>> color_picker_hue_ring();
enum class ColorPickerType { wheel,hue_saturation,saturation_brightness,grayscale,rgb,cmyk };
struct ColorPickerOptions final {
    ColorPickerType type=ColorPickerType::wheel;
    std::shared_ptr<ColorPickerModel> model;
    double width=320,padding=12;
    bool enabled=true,show_recent_colors=true,show_actions=true;
    std::optional<std::vector<ii::gui::WidgetColor>> recent_colors;
    std::string apply_text="Apply",cancel_text="Cancel";
};
struct ColorPickerEvents final {
    std::function<void(ii::gui::WidgetColor)> color_edited,accepted;
    ii::runtime::Task canceled,eyedropper_requested;
};
// Mount-owned composition. Handles borrow their generation and never keep the
// view/backend alive after unmount. The model can be shared explicitly.
class ColorPicker final {
public:
    ColorPicker()=default;
    [[nodiscard]] static ii::core::Result<ColorPicker> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,ColorPickerOptions={},ColorPickerEvents={});
    [[nodiscard]] ii::gui::Widget root() const noexcept {return root_;}
    [[nodiscard]] ii::core::Result<std::shared_ptr<ColorPickerModel>> model() const;
    [[nodiscard]] ii::core::Result<void> set_type(ColorPickerType) const;
    [[nodiscard]] ii::core::Result<void> set_width(double) const;
    [[nodiscard]] ii::core::Result<void> set_enabled(bool) const;
    [[nodiscard]] ii::core::Result<void> show_recent_colors(bool) const;
    [[nodiscard]] ii::core::Result<void> show_actions(bool) const;
    [[nodiscard]] ii::core::Result<void> set_recent_colors(std::vector<ii::gui::WidgetColor>) const;
    [[nodiscard]] ii::core::Result<void> set_color(ii::gui::WidgetColor) const;
    [[nodiscard]] ii::core::Result<bool> set_hex(std::string_view) const;
    [[nodiscard]] ii::core::Result<void> accept() const;
    [[nodiscard]] ii::core::Result<void> cancel() const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock() const;
    ii::gui::Widget root_;
    std::weak_ptr<Impl> state_;
};
}
