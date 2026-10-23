#pragma once
#include "ColorPicker.h"
namespace lvrs {
enum class ColorPickerButtonSize { small=22,medium=28,large=36 };
struct ColorPickerButtonOptions final {
    std::shared_ptr<ColorPickerModel> model;
    // With no model, use this color or the original purple default. Supplying
    // both a model and an initial color is rejected instead of mutating a model.
    std::optional<ii::gui::WidgetColor> current_color;
    ColorPickerButtonSize button_size=ColorPickerButtonSize::medium;
    bool enabled=true,show_hue_ring=true;
    bool circular=false;
    double border_opacity=1;
    std::string accessibility_name="Choose color";
};
[[nodiscard]] ii::core::Result<std::shared_ptr<const ii::gui::RasterImage>> color_picker_button_hue_ring();
}
