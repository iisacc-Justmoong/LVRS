#pragma once
#include <iiGUI/Host.h>
#include <iiSDKCore/Core/Result/Result.h>
#include <string>
namespace lvrs {
enum class TooltipPlacement { automatic,above,below,left_side,right_side };
struct TooltipPoint final {double x=0,y=0;friend bool operator==(const TooltipPoint&,const TooltipPoint&)=default;};
struct TooltipLayoutOptions final {
    TooltipPoint anchor;
    ii::gui::Rect available;
    ii::gui::Size content;
    TooltipPlacement preferred=TooltipPlacement::automatic;
    double maximum_width=320,maximum_height=320,content_padding=12,edge_margin=8,tail_length=10;
};
struct TooltipLayout final {
    TooltipPlacement placement=TooltipPlacement::above;
    TooltipPoint anchor;
    ii::gui::Rect body,bounds;
    friend bool operator==(const TooltipLayout&,const TooltipLayout&)=default;
};
[[nodiscard]] ii::core::Result<TooltipLayout> layout_tooltip(TooltipLayoutOptions);
[[nodiscard]] ii::core::Result<std::string> tooltip_outline(const TooltipLayout&,double corner_radius=12,double tail_width=16);
} // namespace lvrs
