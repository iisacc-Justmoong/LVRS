#pragma once
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
enum class AvatarSize {small,medium,large};
using ImageStatus=ii::gui::ImageStatus;
struct AvatarOptions final {
    AvatarSize size=AvatarSize::large;
    std::shared_ptr<const ii::gui::RasterImage> source;
    ii::gui::WidgetColor background{.1935568452,.1978072077,.1999323964,1};
    double width=0,height=0;
    bool enabled=true;
    std::string accessibility_name="User avatar";
};
class Avatar final {
public:
    Avatar()=default;
    [[nodiscard]] static ii::core::Result<Avatar> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,AvatarOptions={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<AvatarOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(AvatarOptions)const;
    [[nodiscard]] ii::core::Result<void> set_source(std::shared_ptr<const ii::gui::RasterImage>)const;
    [[nodiscard]] ii::core::Result<ImageStatus> status()const;
    [[nodiscard]] ii::core::Result<double> progress()const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
