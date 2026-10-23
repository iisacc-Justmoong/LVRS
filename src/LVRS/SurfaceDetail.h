#pragma once
#include <iiGUI/NativeWidgets.h>
namespace lvrs::surface_detail {
bool metric(double)noexcept;
bool text(std::string_view);
bool color(ii::gui::WidgetColor)noexcept;
ii::gui::Color linear(ii::gui::WidgetColor);
ii::core::Result<void> paint(ii::gui::Widget,ii::gui::Rect,ii::gui::WidgetColor,double,bool=false);
void require(ii::core::Result<void>);
}
