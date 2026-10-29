#pragma once
#include <LVRS/MaterialSurface.h>
namespace lvrs {
class WindowMaterial final:public MaterialSurface {
public:
    WindowMaterial()=default;
    [[nodiscard]] static ii::core::Result<WindowMaterial> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,MaterialOptions={},MaterialContentFactory={});
private:
    explicit WindowMaterial(MaterialSurface value):MaterialSurface(std::move(value)){}
};
}
