#include <LVRS/TableDetail.h>
namespace lvrs::table_detail {
void good(ii::core::Result<void> r){if(!r)throw BuildError{r.error()};}
bool metric(double v,double low){return std::isfinite(v)&&v>=low&&v<=8192;}
bool text(std::string_view s){return s.size()<=65536&&ii::core::valid_utf8(s)&&s.find('\0')==s.npos;}
bool color(ii::gui::WidgetColor c){for(auto v:{c.red,c.green,c.blue,c.alpha})if(!std::isfinite(v)||v<0||v>1)return false;return true;}
bool valid_cell(const TableCellOptions& o){return text(o.text)&&text(o.value_type)&&metric(o.width,1)&&metric(o.cell_height,1)&&metric(o.content_spacing)&&metric(o.current_border_width)&&unsigned(o.text_style)<=7&&color(o.divider_color)&&color(o.text_color)&&(!o.selection_color||color(*o.selection_color))&&(!o.current_border_color||color(*o.current_border_color));}
std::shared_ptr<const ii::gui::RasterImage> empty_image(){static auto image=take(ii::gui::rgba_image(1,1,{0,0,0,0}));return image;}
ii::gui::ImageOptions ink(double w,double h,ii::gui::WidgetColor c){return {.image=empty_image(),.width=std::max(1.,w),.height=std::max(1.,h),.background=c};}
}
