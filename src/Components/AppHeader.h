#pragma once
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
struct AppHeaderOptions final {
    std::string title,subtitle;
    bool menu_visible=false,compact=false,enabled=true;
    std::optional<double> content_horizontal_padding,content_vertical_padding,row_spacing,action_spacing,menu_button_padding;
    double width=0,height=0;
    ii::gui::WidgetColor background{.083941631,.083941631,.086313099,1};
    ii::gui::WidgetColor title_color{1,1,1,.9},subtitle_color{1,1,1,.8};
    ii::gui::WidgetColor menu_background{.1139791608,.1166428775,.1166428775,1},menu_pressed_background{.1230252981,.1252803057,.1275353134,1};
};
struct AppHeaderEvents final {ii::runtime::Task menu_clicked;};
using AppHeaderActionFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget)>;
class AppHeader final {
public:
    AppHeader()=default;
    [[nodiscard]] static ii::core::Result<AppHeader> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,AppHeaderOptions={},AppHeaderEvents={},AppHeaderActionFactory={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget actions_root()const noexcept{return actions_;}
    [[nodiscard]] ii::core::Result<AppHeaderOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(AppHeaderOptions)const;
    [[nodiscard]] ii::core::Result<void> set_title(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> set_subtitle(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> set_menu_visible(bool)const;
    [[nodiscard]] ii::core::Result<void> set_compact(bool)const;
    [[nodiscard]] ii::core::Result<void> set_enabled(bool)const;
    [[nodiscard]] ii::core::Result<void> set_background(ii::gui::WidgetColor)const;
    [[nodiscard]] ii::core::Result<void> set_menu_clicked(ii::runtime::Task)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_,actions_;
    std::weak_ptr<Impl> state_;
};
}
