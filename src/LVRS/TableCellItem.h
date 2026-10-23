#pragma once
#include <iiGUI/NativeWidgets.h>
#include "TableValue.h"
namespace lvrs {
struct TableCellOptions final {
    std::string text="Text";
    double width=234,cell_height=24,content_spacing=8;
    ii::gui::WidgetColor divider_color{.0839416310,.0839416310,.0863130987,1},text_color{1,1,1,.8};
    bool show_divider=true,clip_content=true;
    ii::gui::TextRole text_style=ii::gui::TextRole::body;
    std::optional<bool> inputable;
    bool enabled=true,selected=false,current=false;
    std::optional<ii::gui::WidgetColor> selection_color,current_border_color;
    double current_border_width=1;
    std::string value_type="string";
    TableValidator validator;
};
struct TableCellEvents final {
    std::function<void(std::string)> edited,submitted;
    std::function<void(std::string,std::string)> rejected;
};
struct TableCellState final {
    std::string input_result;
    TableValue typed_value;
    bool input_accepted=true,has_input_override=false;
};
class TableCellItem final {
public:
    TableCellItem()=default;
    [[nodiscard]] static ii::core::Result<TableCellItem> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,TableCellOptions={},TableCellEvents={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget editor()const noexcept{return editor_;}
    [[nodiscard]] ii::core::Result<TableCellState> snapshot()const;
    [[nodiscard]] ii::core::Result<std::string> apply_input_result(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> set_options(TableCellOptions)const;
    [[nodiscard]] ii::core::Result<void> set_selected(bool selected,bool current=false)const;
private:
    struct Impl;
    ii::gui::Widget root_,editor_;
    std::weak_ptr<Impl> state_;
};
}
