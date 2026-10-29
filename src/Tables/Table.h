#pragma once
#include <LVRS/TableTransfer.h>
#include <iiGUI/NativeWidgets.h>

namespace lvrs {
enum class TableSelectionMode { none,single_cell,range };
struct TableOptions final {
    std::shared_ptr<TableModel> model;
    double width=717,height=121,header_height=24,content_spacing=8,border_width=1;
    bool selection_enabled=true,keyboard_navigation=true,sorting_enabled=false,fit_columns_to_viewport=true;
    TableSelectionMode selection_mode=TableSelectionMode::range;
    ii::gui::WidgetColor background{30./255,30./255,30./255,1};
    ii::gui::WidgetColor divider{.1555851549,.1570725590,.1585599482,1};
    ii::gui::WidgetColor header_text{1,1,1,.6},cell_text{1,1,1,.8};
    std::optional<ii::gui::WidgetColor> selection_color,current_color;
    bool resize_handles_visible=true,structure_controls_visible=false;
    bool add_row_controls_visible=true,add_column_controls_visible=true;
    bool delete_context_menu_enabled=true,select_on_context_click=true,clipboard_enabled=true;
    double column_resize_handle_width=6,row_resize_handle_height=6;
    double structure_gutter_width=20,structure_gutter_height=20;
    ii::gui::WidgetColor resize_handle_color{0,0,0,0};
    std::optional<ii::gui::WidgetColor> resize_handle_hover_color;
};
struct TableEvents final {
    std::function<void(std::size_t,std::size_t,std::string)> edited,submitted;
    std::function<void(std::size_t,std::size_t,std::string,std::string)> rejected;
    std::function<void(std::optional<TableRange>)> selection_changed;
    std::function<void(std::size_t,std::size_t)> activated;
    std::function<void(std::size_t,TableSortOrder)> sorted;
    std::function<void(TableRange)> range_values_changed;
    std::function<void(std::size_t,double)> column_resized,row_resized;
};
struct TableViewState final {
    std::optional<TablePosition> current;
    std::optional<TableRange> selection;
    bool editing=false;
    double scroll_offset=0;
    std::size_t visible_cells=0,first_visible_row=0;
    std::uint64_t model_revision=0;
    std::optional<std::size_t> sort_column;
    TableSortOrder sort_order=TableSortOrder::ascending;
    std::optional<std::size_t> resizing_column,resizing_row;
    std::optional<TablePosition> context_cell;
    double horizontal_scroll_offset=0;
    std::size_t first_visible_column=0;
};
// Native C++ composed widget: one clipped vector viewport and one reusable
// editor, independent of the number of data rows. Model data stays in Core's
// owner-thread contract; selection and scrolling do not create data history.
class Table final {
public:
    Table()=default;
    [[nodiscard]] static ii::core::Result<Table> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,TableOptions={},TableEvents={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget viewport()const noexcept{return viewport_;}
    [[nodiscard]] ii::gui::Widget body()const noexcept{return body_;}
    [[nodiscard]] ii::gui::Widget editor()const noexcept{return editor_;}
    [[nodiscard]] ii::gui::Widget context_menu()const noexcept{return menu_;}
    [[nodiscard]] ii::core::Result<TableViewState> snapshot()const;
    [[nodiscard]] ii::core::Result<void> set_options(TableOptions)const;
    [[nodiscard]] ii::core::Result<void> set_scroll_offset(double)const;
    [[nodiscard]] ii::core::Result<void> set_horizontal_scroll_offset(double)const;
    [[nodiscard]] ii::core::Result<void> set_scroll_position(double x,double y)const;
    [[nodiscard]] ii::core::Result<void> reveal_cell(TablePosition)const;
    [[nodiscard]] ii::core::Result<bool> select_cell(TablePosition,bool extend=false)const;
    [[nodiscard]] ii::core::Result<bool> select_range(TablePosition first,TablePosition last)const;
    [[nodiscard]] ii::core::Result<bool> select_row(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> select_column(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> select_all()const;
    [[nodiscard]] ii::core::Result<bool> is_cell_selected(TablePosition)const;
    [[nodiscard]] ii::core::Result<bool> clear_selection()const;
    [[nodiscard]] ii::core::Result<TableMatrix> selection_values()const;
    [[nodiscard]] ii::core::Result<std::string> selection_as_tsv()const;
    [[nodiscard]] ii::core::Result<bool> set_range_values(TablePosition,const TableInputMatrix&)const;
    [[nodiscard]] ii::core::Result<bool> paste_tsv(TablePosition,std::string_view)const;
    [[nodiscard]] ii::core::Result<bool> paste_selection_tsv(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> copy_selection()const;
    [[nodiscard]] ii::core::Result<bool> paste_from_clipboard()const;
    [[nodiscard]] ii::core::Result<void> begin_edit()const;
    [[nodiscard]] ii::core::Result<void> end_edit()const;
    [[nodiscard]] ii::core::Result<bool> sort_rows(std::size_t,TableSortOrder)const;
    [[nodiscard]] ii::core::Result<bool> merge_selection()const;
    [[nodiscard]] ii::core::Result<bool> split_cell(TablePosition)const;
    [[nodiscard]] ii::core::Result<bool> insert_row(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> delete_row(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> insert_column(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> delete_column(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> undo()const;
    [[nodiscard]] ii::core::Result<bool> redo()const;
    [[nodiscard]] ii::core::Result<void> open_context_menu(TablePosition,double x,double y)const;
private:
    struct Impl;
    template<class T,class F> [[nodiscard]] ii::core::Result<T> with_state(F&&)const;
    ii::gui::Widget root_,viewport_,body_,editor_,menu_;
    std::weak_ptr<Impl> state_;
};
}
