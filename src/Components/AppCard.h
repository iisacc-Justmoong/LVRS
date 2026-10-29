#pragma once
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
enum class AppCardShape {round_rect,cylinder};
struct AppCardOptions final {
    // Zero extents follow implicit content size. Explicit legacy extents remain supported.
    double width=0,height=0,padding=18,spacing=10,corner_radius=12;
    ii::gui::WidgetColor background{.1139791608,.1166428775,.1166428775,1};
    std::string title,subtitle;
    AppCardShape shape=AppCardShape::round_rect;
    bool enabled=true;
    ii::gui::WidgetColor title_color{1,1,1,.9},subtitle_color{1,1,1,.8};
};
using AppCardContentFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget)>;
class AppCard final {
public:
    AppCard()=default;
    [[nodiscard]] static ii::core::Result<AppCard> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,AppCardOptions={},AppCardContentFactory={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget content_root()const noexcept{return content_;}
    [[nodiscard]] ii::core::Result<AppCardOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(AppCardOptions)const;
    [[nodiscard]] ii::core::Result<void> set_title(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> set_subtitle(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> set_enabled(bool)const;
    [[nodiscard]] ii::core::Result<void> set_background(ii::gui::WidgetColor)const;
    [[nodiscard]] ii::core::Result<void> set_shape(AppCardShape)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_,content_;
    std::weak_ptr<Impl> state_;
};
}
