#pragma once
#include <LVRS/TableCellItem.h>
namespace lvrs {
struct TableRowOptions final {
    double width=717,cell_width=234,cell_height=24,content_spacing=8;
    bool inputable=false,enabled=true;
    ii::gui::WidgetColor divider_color{.1555851549,.1570725590,.1585599482,1},text_color{1,1,1,.8};
    std::vector<TableCellOptions> cells=std::vector<TableCellOptions>(3);
};
struct TableRowEvents final {std::function<void(std::size_t,std::string)> edited,submitted;};
class TableRow final {
public:
    TableRow()=default;
    [[nodiscard]] static ii::core::Result<TableRow> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,TableRowOptions={},TableRowEvents={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<TableCellItem> cell(std::size_t)const;
    // Same-count data/style/geometry replacement preserves native handles.
    [[nodiscard]] ii::core::Result<void> set_options(TableRowOptions)const;
private:
    struct Impl;ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
