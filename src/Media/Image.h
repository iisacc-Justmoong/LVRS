#pragma once
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
// View-only image control. Resource discovery/decoding belongs to its C++
// source service, so rendering never owns a hidden network or document model.
class Image final {
public:
    Image()=default;
    [[nodiscard]] static ii::core::Result<Image> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,ii::gui::ImageOptions={.allow_empty=true});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<ii::gui::ImageOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(ii::gui::ImageOptions)const;
    [[nodiscard]] ii::core::Result<void> set_source(std::shared_ptr<const ii::gui::RasterImage>)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;
    std::weak_ptr<Impl> state_;
};
}
