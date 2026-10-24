#pragma once
#include "MaterialSurface.h"
namespace lvrs::material_detail {
struct Colors final {ii::gui::WidgetColor tint,primary,base;friend bool operator==(const Colors&,const Colors&)=default;};
struct Prepared final {MaterialState state;Colors target;};
struct RasterCache final {
    ii::gui::Size size;double scale=0,radius=0,padding=0,intense=0,faint=0,primary_alpha=0,border_width=0;
    ii::gui::WidgetColor border;std::string path;
    std::shared_ptr<const ii::gui::RasterImage> mask,gradient,stroke;
    std::uint64_t masks=0,gradients=0,strokes=0;
};
[[nodiscard]] ii::core::Result<Prepared> prepare(const MaterialOptions&,MaterialVariant,const ii::gui::WidgetAppearance&);
[[nodiscard]] ii::core::Result<std::shared_ptr<const ii::gui::WidgetDrawing>> paint(const MaterialOptions&,const Prepared&,const Colors&,const ii::gui::WidgetPaintState&,RasterCache&);
}
