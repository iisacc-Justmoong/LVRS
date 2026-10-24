#include "WindowMaterial.h"
namespace lvrs {
ii::core::Result<WindowMaterial> WindowMaterial::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,std::string_view id,MaterialOptions options,MaterialContentFactory content){auto surface=create_variant(b,parent,id,MaterialVariant::window,std::move(options),std::move(content));if(!surface)return std::unexpected(surface.error());return WindowMaterial(std::move(*surface));}
}
