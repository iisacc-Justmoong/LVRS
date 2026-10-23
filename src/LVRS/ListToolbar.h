#pragma once
#include "ToolbarButton.h"
#include <array>
namespace lvrs {
struct ListToolbarOptions final {
    std::array<std::string,3> icons;
    std::array<std::shared_ptr<const ii::gui::RasterImage>,3> images;
    double icon_size=18,button_size=22,horizontal_padding=8,vertical_padding=4,spacing=4;
    ii::gui::WidgetColor background{.0951643139,.0965511352,.0979379490,1};
    bool interactive=true,enabled=true;
    double width=0,height=0;
};
struct ListToolbarEvents final {std::function<void(std::size_t,std::string)> icon_clicked;};
class ListToolbar final {
public:
    ListToolbar()=default;
    [[nodiscard]] static ii::core::Result<ListToolbar> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,ListToolbarOptions={},ListToolbarEvents={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<ListToolbarOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(ListToolbarOptions)const;
    [[nodiscard]] ii::core::Result<ii::gui::Widget> button_at(int)const;
    [[nodiscard]] ii::core::Result<std::string> resolved_icon(int)const;
    [[nodiscard]] ii::core::Result<void> set_events(ListToolbarEvents)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
