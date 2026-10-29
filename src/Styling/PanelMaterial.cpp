#include <LVRS/PanelMaterial.h>
namespace lvrs {
ii::core::Result<PanelMaterial> PanelMaterial::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,std::string_view id,MaterialOptions options,MaterialContentFactory content){auto surface=create_variant(b,parent,id,MaterialVariant::panel,std::move(options),std::move(content));if(!surface)return std::unexpected(surface.error());return PanelMaterial(std::move(*surface));}
}
