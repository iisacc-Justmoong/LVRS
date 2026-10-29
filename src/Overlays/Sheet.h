#pragma once
#include <LVRS/Motion.h>
namespace lvrs {
enum class SheetPresentation { automatic,mobile,desktop };
using SheetDetent=ii::gui::SheetDetent;
struct SheetOptions final {
    std::string title,description;
    SheetPresentation presentation=SheetPresentation::automatic;
    SheetDetent detent=SheetDetent::fit;
    std::optional<double> corner_radius;
    bool show_header=true,show_description=true,show_close_button=true;
    std::optional<bool> show_grabber;
    bool scroll_content=true;
    double preferred_width=560,preferred_height=420,desktop_margin=24,content_padding=20;
    double top_safe_inset=0,bottom_safe_inset=0;
    bool dismiss_on_background=true,dismiss_on_escape=true,dismiss_on_drag=true;
    bool motion_enabled=true;
    std::shared_ptr<Motion> motion;
    std::chrono::milliseconds animation_duration=MotionTokens::surface;
    ii::gui::WidgetColor surface_color{.1370945126,.1402995437,.1419020444,1},backdrop_color{0,0,0,.48};
    std::string accessibility_name="Sheet";
};
struct SheetEvents final {ii::runtime::Task opened,closed;};
using SheetContentFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget)>;
// The mount owns chrome and content. Closing hides the same body; opening
// resets only the scroll/drag state. No OS sheet or control is constructed.
class Sheet final {
public:
    Sheet()=default;
    [[nodiscard]] static ii::core::Result<Sheet> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,SheetOptions={},SheetEvents={},SheetContentFactory={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget viewport()const noexcept{return viewport_;}
    [[nodiscard]] ii::gui::Widget content_item()const noexcept{return content_;}
    [[nodiscard]] ii::core::Result<void> open()const;
    [[nodiscard]] ii::core::Result<void> close()const;
    [[nodiscard]] ii::core::Result<void> set_options(SheetOptions)const;
private:
    struct Impl;
    ii::gui::Widget root_,viewport_,content_;
    std::weak_ptr<Impl> state_;
};
}
