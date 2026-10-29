#pragma once
#include <LVRS/Card.h>
#include <array>
namespace lvrs::card_detail {
struct Prepared final {
    std::shared_ptr<const ii::gui::RasterImage> icon,placeholder,check,more,globe,transparent,white;
    std::vector<std::shared_ptr<const ii::gui::RasterImage>> row_icons;
    std::vector<double> value_widths;
    double action_width=0,progress_value_width=0;
    ii::gui::WidgetColor primary;
    std::string font,action_text,progress_label;
};
struct Layout final {
    ii::gui::Rect icon,menu,action;
    double width=0,height=0,inset=0,title_y=0,title_height=19,detail_y=0,detail_height=0,metadata_y=0,caption_height=0,preview_y=0,progress_y=0,row_y=0,summary_y=0;
};
[[nodiscard]] ii::core::Result<Prepared> prepare(const CardOptions&,const ii::gui::WidgetAppearance&);
[[nodiscard]] CardState resolve(const CardOptions&,const Prepared&,const ii::gui::WidgetPaintState&,bool preview_custom=false);
[[nodiscard]] Layout layout(const CardOptions&,const Prepared&,const CardState&,ii::gui::Size);
[[nodiscard]] std::shared_ptr<const ii::gui::WidgetDrawing> background(const CardOptions&,const Prepared&,const CardState&,const Layout&,bool preview_custom,bool icon_custom,bool enabled);
[[nodiscard]] std::shared_ptr<const ii::gui::WidgetDrawing> foreground(const CardOptions&,const Prepared&,const CardState&,const Layout&,bool enabled);
}
