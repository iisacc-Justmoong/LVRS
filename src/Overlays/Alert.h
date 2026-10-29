#pragma once
#include <LVRS/AlertButton.h>
#include <LVRS/Motion.h>
#include <array>
namespace lvrs {
enum class AlertIcon { adjustments,file_text };
enum class AlertShape { round_rect,cylinder };
[[nodiscard]] ii::core::Result<std::string_view> alert_icon_source(AlertIcon);
struct AlertOptions final {
    std::string title="Alert Dialog",message="It can have 2 or 3 actions depending on your needs.";
    int button_count=0;
    std::string primary_text="Button",secondary_text="Button",tertiary_text;
    bool primary_enabled=true,secondary_enabled=true,tertiary_enabled=true;
    std::optional<bool> secondary_destructive;
    bool dismiss_on_background=false,dismiss_on_escape=false;
    double minimum_width=280,maximum_width=500,preferred_width=500,side_padding=24,corner_radius=36;
    AlertShape shape=AlertShape::round_rect;
    ii::gui::WidgetColor backdrop_color{0,0,0,89./255},card_color{29./255,31./255,33./255,.72};
    bool glass_enabled=true;
    double glass_blur_radius=28;
    bool shadow_enabled=true;
    ii::gui::PopoverShadow shadow;
    bool motion_enabled=true;
    std::shared_ptr<Motion> motion;
    bool show_icon=true,use_builtin_image=true;
    std::shared_ptr<const ii::gui::RasterImage> image_source;
    std::optional<AlertIcon> builtin_icon;
    std::optional<double> icon_size,icon_frame_size;
    std::optional<ii::gui::WidgetColor> icon_background,icon_border,primary_color;
    ii::gui::WidgetColor icon_inner{244./255,245./255,247./255,1};
    std::string accessibility_name="Alert";
};
struct AlertEvents final {
    ButtonMethod button1_method,button2_method,button3_method;
    ii::runtime::Task primary_clicked,secondary_clicked,tertiary_clicked,dismissed,opened,closed;
};
class Alert final {
public:
    Alert()=default;
    [[nodiscard]] static ii::core::Result<Alert> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,AlertOptions={},AlertEvents={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<void> open()const;
    [[nodiscard]] ii::core::Result<void> close()const;
    [[nodiscard]] ii::core::Result<bool> dismiss()const;
    [[nodiscard]] ii::core::Result<int> resolved_button_count()const;
    [[nodiscard]] ii::core::Result<ii::gui::Widget> action_button(int index)const;
    [[nodiscard]] ii::core::Result<void> set_text(std::string title,std::string message)const;
    [[nodiscard]] ii::core::Result<void> set_button_count(int)const;
    [[nodiscard]] ii::core::Result<void> set_action_text(int,std::string)const;
    [[nodiscard]] ii::core::Result<void> set_action_enabled(int,bool)const;
    [[nodiscard]] ii::core::Result<void> set_action_method(int,ButtonMethod)const;
    [[nodiscard]] ii::core::Result<void> set_primary_color(ii::gui::WidgetColor)const;
    [[nodiscard]] ii::core::Result<void> set_show_icon(bool)const;
    [[nodiscard]] ii::core::Result<void> set_glass_enabled(bool)const;
    [[nodiscard]] ii::core::Result<void> set_shadow_enabled(bool)const;
    [[nodiscard]] ii::core::Result<void> set_motion_enabled(bool)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
