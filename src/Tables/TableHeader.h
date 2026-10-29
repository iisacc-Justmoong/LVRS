#pragma once
#include <LVRS/TableHeaderModel.h>
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
struct TableHeaderOptions final {
    TableHeaderModelOptions layout;
    bool interactive=false,enabled=true;
    double separator_height=1;
    ii::gui::WidgetColor text_color{1,1,1,.6},separator_color{.1555851549,.1570725590,.1585599482,1};
};
struct TableHeaderEvents final {std::function<void(std::size_t,const TableHeaderColumn&)> column_clicked;};
class TableHeader final {
public:
    TableHeader()=default;
    [[nodiscard]] static ii::core::Result<TableHeader> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,TableHeaderOptions={},TableHeaderEvents={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<ii::gui::Widget> column(std::size_t)const;
    [[nodiscard]] ii::core::Result<void> set_options(TableHeaderOptions)const;
private:
    struct Impl;ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
