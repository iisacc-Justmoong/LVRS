#pragma once
#include "TooltipLayout.h"
#include "PanelMaterial.h"
#include "Motion.h"
namespace lvrs {
struct TooltipOptions final {
    ii::gui::Widget target;
    std::string text;
    std::optional<TooltipPoint> anchor_point;
    std::optional<ii::gui::Rect> available_rect;
    TooltipPlacement preferred_placement=TooltipPlacement::automatic;
    bool automatic=true,scroll_content=true,motion_enabled=true;
    double maximum_width=320,maximum_height=320,content_padding=12,edge_margin=8,corner_radius=12,tail_length=10,tail_width=16;
    std::chrono::milliseconds delay{500},timeout{5000},hide_delay{120},animation_duration{420};
    std::optional<ii::gui::WidgetColor> surface_color,border_color,primary_color;
    double surface_opacity=.25;
    MaterialBackdrop backdrop=MaterialBackdrop::content;
    std::shared_ptr<Motion> motion;
    std::string accessibility_name;
};
struct TooltipEvents final {ii::runtime::Task opened,closed;};
struct TooltipState final {
    TooltipLayout layout;
    ii::gui::Size content_size;
    bool opened=false,pending=false,automatic_latched=false;
    std::string last_error;
};
using TooltipContentFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget)>;
[[nodiscard]] ii::core::Result<std::chrono::milliseconds> tooltip_milliseconds(double);
// Retained self-rendered bubble. Native window/IME services stay in iiGUI;
// all delays belong to the host's iiSDKCore event loop and this View's scope.
class Tooltip final {
public:
    Tooltip()=default;
    [[nodiscard]] static ii::core::Result<Tooltip> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,TooltipOptions={},TooltipEvents={},TooltipContentFactory={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget viewport()const noexcept{return viewport_;}
    [[nodiscard]] ii::gui::Widget content_item()const noexcept{return content_;}
    [[nodiscard]] ii::core::Result<TooltipState> state()const;
    [[nodiscard]] ii::core::Result<TooltipOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(TooltipOptions)const;
    [[nodiscard]] ii::core::Result<void> set_target(ii::gui::Widget,std::optional<TooltipPoint> = {})const;
    [[nodiscard]] ii::core::Result<void> open()const;
    [[nodiscard]] ii::core::Result<void> open_at(ii::gui::Widget,TooltipPoint)const;
    [[nodiscard]] ii::core::Result<void> close()const;
    [[nodiscard]] ii::core::Result<void> refresh()const;
private:
    struct Impl;
    template<class T,class F>[[nodiscard]] ii::core::Result<T> with_state(F&&)const;
    ii::gui::Widget root_,viewport_,content_;
    std::weak_ptr<Impl> state_;
};
} // namespace lvrs
