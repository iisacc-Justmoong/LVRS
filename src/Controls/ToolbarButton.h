#pragma once
#include <iiGUI/NativeWidgets.h>
#include <variant>

namespace lvrs {
using ToolbarId=std::variant<std::int64_t,std::string>;
struct ToolbarButtonOptions final {
    ToolbarId button_id=std::int64_t(-1);
    std::string icon_name,accessibility_name,text;
    std::shared_ptr<const ii::gui::RasterImage> icon;
    bool enabled=true,visible=true,fallback_icon=true,menu=false;
    double width=0,height=22,icon_size=18,left_padding=2,right_padding=2,content_spacing=0,corner_radius=8;
    ii::gui::ButtonTone tone=ii::gui::ButtonTone::borderless;
    std::optional<ii::gui::ButtonStyle> style;
};
class HierarchyToolbar;
class ToolbarButton final {
public:
    ToolbarButton()=default;
    [[nodiscard]] static ii::core::Result<ToolbarButton> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,ToolbarButtonOptions={},ii::runtime::Task={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<ToolbarButtonOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(ToolbarButtonOptions)const;
    [[nodiscard]] ii::core::Result<bool> active()const;
    [[nodiscard]] ii::core::Result<void> set_clicked(ii::runtime::Task)const;
private:
    friend class HierarchyToolbar;
    [[nodiscard]] ii::core::Result<void> attach(ii::runtime::Task,ii::runtime::Task)const;
    [[nodiscard]] ii::core::Result<void> set_active(bool)const;
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;
    std::weak_ptr<Impl> state_;
};
}
