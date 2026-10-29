#pragma once
#include <LVRS/Icons.h>
#include <LVRS/Motion.h>
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
enum class ModalShape { round_rect,cylinder };
struct ModalOptions final {
    std::string title,description,message,icon_name;
    std::shared_ptr<const ii::gui::RasterImage> icon_source;
    bool show_icon=true;
    double icon_size=72,icon_corner_radius=16;
    ii::gui::WidgetColor icon_background{.1411764771,.1411764771,.1411764771,1},icon_border{208./255,215./255,226./255,.18};
    int button_count=0;
    std::string primary_text="OK",secondary_text,tertiary_text;
    bool primary_enabled=true,secondary_enabled=true,tertiary_enabled=true;
    double minimum_width=520,maximum_width=980,preferred_width=760,side_padding=24,minimum_height=210,vertical_offset=-16,corner_radius=12;
    ModalShape shape=ModalShape::round_rect;
    ii::gui::WidgetColor frame_color{.1370945126,.1402995437,.1419020444,1},backdrop_color{0,0,0,89./255};
    bool dismiss_on_background=true;
    bool motion_enabled=true;
    std::shared_ptr<Motion> motion;
    std::string accessibility_name="Modal";
};
struct ModalEvents final {ii::runtime::Task primary_clicked,secondary_clicked,tertiary_clicked,canceled,opened,closed;};
using ModalContentFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget)>;
// The mount owns persistent chrome; each opening owns a fresh body activation.
// Actions emit without closing. Cancel is distinct from programmatic close.
class Modal final {
public:
    Modal()=default;
    [[nodiscard]] static ii::core::Result<Modal> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,ModalOptions={},ModalEvents={},ModalContentFactory={});
    [[nodiscard]] ii::gui::Widget root() const noexcept {return root_;}
    [[nodiscard]] ii::core::Result<void> open() const;
    [[nodiscard]] ii::core::Result<void> close() const;
    [[nodiscard]] ii::core::Result<bool> cancel() const;
    [[nodiscard]] ii::core::Result<bool> contains_frame_point(double x,double y) const;
    [[nodiscard]] ii::core::Result<bool> action_visible(int index) const;
    [[nodiscard]] ii::core::Result<bool> trigger_action(int index) const;
    [[nodiscard]] ii::core::Result<ii::gui::Widget> content_item() const;
    [[nodiscard]] ii::core::Result<void> set_motion_enabled(bool) const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock() const;
    ii::gui::Widget root_;
    std::weak_ptr<Impl> state_;
};
}
