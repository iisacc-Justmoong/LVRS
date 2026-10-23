#include "SurfaceDetail.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace lvrs::surface_detail {
bool metric(double value)noexcept{return std::isfinite(value)&&value>=0&&value<=8192;}
bool text(std::string_view value){return value.size()<=65536&&value.find('\0')==value.npos&&ii::core::valid_utf8(value);}
bool color(ii::gui::WidgetColor value)noexcept{for(auto n:{value.red,value.green,value.blue,value.alpha})if(!std::isfinite(n)||n<0||n>1)return false;return true;}
ii::gui::Color linear(ii::gui::WidgetColor value){const auto channel=[](double x){return float(x<=.04045?x/12.92:std::pow((x+.055)/1.055,2.4));};return {channel(value.red),channel(value.green),channel(value.blue),float(value.alpha)};}
ii::core::Result<void> paint(ii::gui::Widget widget,ii::gui::Rect bounds,ii::gui::WidgetColor fill,double radius,bool cylinder){
    // Hidden ancestors have no allocated surface yet. Their first visible
    // layout callback supplies the real bounds; zero rectangles are not ink.
    if(bounds.width<=0||bounds.height<=0)return widget.set_drawing({});
    const auto half=std::min(bounds.width,bounds.height)/2;const auto ink=linear(fill);
    return widget.set_drawing({.panels={{{0,0,bounds.width,bounds.height},ink,ink,cylinder?half:std::min(float(radius),half)}}});
}
void require(ii::core::Result<void> result){if(!result&&result.error().code!=ii::core::ErrorCode::closed){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}}
}
