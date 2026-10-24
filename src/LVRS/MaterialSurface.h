#pragma once
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
enum class MaterialDensity{dense75,glass25};
enum class MaterialBackdrop{none,content,previous_layers};
enum class MaterialVariant{surface,panel,window};
struct MaterialOptions final {
    std::optional<MaterialDensity> density;
    std::optional<ii::gui::WidgetColor> primary_color,color,border_color;
    ii::gui::WidgetColor backdrop_base_color{0,0,0,0};
    std::optional<double> tint_opacity,blur_radius,radius,inner_highlight_opacity,shadow_radius,shadow_offset_y,shadow_opacity;
    double border_width=1,inner_highlight_offset_y=1,intense_opacity=.40,faint_opacity=.11;
    bool blur_enabled=true,motion_enabled=true,enabled=true;
    MaterialBackdrop backdrop=MaterialBackdrop::none;
    std::string outline_path;
    double width=0,height=0;
    std::shared_ptr<ii::gui::MotionPreferences> motion;
};
struct MaterialState final {
    MaterialVariant variant=MaterialVariant::surface;MaterialDensity density=MaterialDensity::glass25;
    double tint_opacity=0,blur_radius=0,radius=0,capture_padding=0,inner_highlight_opacity=0,shadow_radius=0,shadow_offset_y=0,shadow_opacity=0;
    ii::gui::WidgetColor displayed_tint,displayed_primary,displayed_base,border;
    ii::gui::Size implicit_size;
    bool capture_requested=false,animating=false;
    std::uint64_t mask_rasterizations=0,gradient_rasterizations=0,border_rasterizations=0;
};
using MaterialContentFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget)>;
class MaterialSurface {
public:
    MaterialSurface()=default;
    [[nodiscard]] static ii::core::Result<MaterialSurface> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,MaterialOptions={},MaterialContentFactory={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget content_root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<MaterialOptions> options()const;
    [[nodiscard]] ii::core::Result<MaterialState> state()const;
    [[nodiscard]] ii::core::Result<void> set_options(MaterialOptions)const;
    [[nodiscard]] ii::core::Result<void> set_primary_color(ii::gui::WidgetColor)const;
    [[nodiscard]] ii::core::Result<void> set_color(ii::gui::WidgetColor)const;
protected:
    [[nodiscard]] static ii::core::Result<MaterialSurface> create_variant(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,MaterialVariant,MaterialOptions,MaterialContentFactory);
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
