#pragma once
#include <iiGUI/NativeWidgets.h>
#include "TableCellItem.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <cmath>
#include <exception>
#include <utility>
namespace lvrs::table_detail {
struct BuildError final : std::exception {
    ii::core::Error error;
    explicit BuildError(ii::core::Error value):error(std::move(value)){}
    const char* what() const noexcept override {return error.message.c_str();}
};
template<class T>T take(ii::core::Result<T> r){if(!r)throw BuildError{r.error()};return std::move(*r);}
void good(ii::core::Result<void>);
bool metric(double,double low=0);
bool text(std::string_view);
bool color(ii::gui::WidgetColor);
bool valid_cell(const TableCellOptions&);
std::shared_ptr<const ii::gui::RasterImage> empty_image();
ii::gui::ImageOptions ink(double,double,ii::gui::WidgetColor);
}
