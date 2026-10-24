#pragma once
#include "MaterialSurface.h"
namespace lvrs {
class PanelMaterial final:public MaterialSurface {
public:
    PanelMaterial()=default;
    [[nodiscard]] static ii::core::Result<PanelMaterial> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,MaterialOptions={},MaterialContentFactory={});
private:
    explicit PanelMaterial(MaterialSurface value):MaterialSurface(std::move(value)){}
};
}
