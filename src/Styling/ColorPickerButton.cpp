#include <LVRS/ColorPickerButton.h>
#include <LVRS/Widgets.h>
namespace lvrs {
namespace detail {std::span<const std::uint8_t> color_button_ring_png();}
ii::core::Result<std::shared_ptr<const ii::gui::RasterImage>> color_picker_button_hue_ring(){
    static const auto image=ii::gui::png_image(detail::color_button_ring_png());return image;
}
Widgets::Result<Widgets::Widget> Widgets::color_picker_button(Widget parent,std::string_view id,ColorPickerButtonOptions o,ii::gui::ControlEvents events){
    using namespace ii;
    if((o.button_size!=ColorPickerButtonSize::small&&o.button_size!=ColorPickerButtonSize::medium&&o.button_size!=ColorPickerButtonSize::large) || (o.model&&o.current_color))return core::fail(core::ErrorCode::invalid_argument,"ColorPickerButton requires Small/Medium/Large and either model or current color");
    if(!o.model){o.model=std::make_shared<ColorPickerModel>();if(auto r=o.model->set_color(o.current_color.value_or(Theme::accent_color(Accent::purple)));!r)return std::unexpected(r.error());}
    auto ring=color_picker_button_hue_ring();if(!ring)return std::unexpected(ring.error());
    return builder_.color_button(parent,id,{.model=std::move(o.model),.hue_ring=*ring,.size=double(o.button_size),.enabled=o.enabled,.show_hue_ring=o.show_hue_ring,.circular=o.circular,.border_opacity=o.border_opacity,.accessibility_name=std::move(o.accessibility_name)},std::move(events));
}
}
