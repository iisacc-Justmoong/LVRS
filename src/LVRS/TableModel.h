#pragma once
#include "TableHeaderModel.h"
#include "TableValue.h"
#include <iiSDKCore/Runtime/Query.h>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace lvrs {
struct TableLimits final {
    static constexpr std::size_t rows=1000000,columns=65536,cells=16000000,cell_bytes=65536;
    static constexpr std::size_t transfer_bytes=16*1024*1024,transfer_cells=1000000;
};
struct TablePosition final {
    std::size_t row = 0, column = 0;
    friend bool operator==(const TablePosition&, const TablePosition&) = default;
};
// Half-open range: [row, row + row_count), [column, column + column_count).
struct TableRange final {
    std::size_t row = 0, column = 0, row_count = 1, column_count = 1;
    friend bool operator==(const TableRange&, const TableRange&) = default;
};
struct TableCellData final {
    std::optional<TableValue> value = std::string("Text");
    std::optional<std::string> text;
    std::optional<bool> inputable;
    friend bool operator==(const TableCellData&, const TableCellData&) = default;
};
struct TableRowData final {
    std::vector<TableCellData> cells = std::vector<TableCellData>(3);
    std::optional<bool> inputable;
    friend bool operator==(const TableRowData&, const TableRowData&) = default;
};
struct TableModelOptions final {
    std::vector<TableRowData> rows = std::vector<TableRowData>(4);
    std::vector<TableHeaderColumn> columns = std::vector<TableHeaderColumn>(3);
    std::string default_cell_text = "Text", default_header_text = "Column";
    bool inputable = false;
    double table_width = 717, row_height = 24, cell_width = 0;
    double minimum_column_width = 32, minimum_row_height = 18;
    // Nonpositive values use the automatic/default dimension. Positive values
    // round to an integral logical pixel and respect the relevant minimum.
    std::vector<double> column_widths, row_heights;
    std::vector<TableRange> merges;
    friend bool operator==(const TableModelOptions&, const TableModelOptions&) = default;
};
struct TableCellGeometry final {
    double x = 0, y = 0, width = 0, height = 0;
    std::size_t row_span = 1, column_span = 1;
    bool covered = false;
};
struct TableVisibleCell final {
    std::size_t index = 0;
    TablePosition position;
    TableCellGeometry geometry;
    TableCellData data;
    std::string text, value_type;
    bool inputable = false;
};
struct TableModelState final {
    std::uint64_t revision = 1;
    std::size_t row_count = 0, column_count = 1, header_count = 0;
    std::size_t undo_depth = 0, redo_depth = 0, undo_limit = 128;
    std::optional<TablePosition> context_cell;
    std::optional<std::size_t> resizing_row, resizing_column;
};
struct TableChange final {
    std::uint64_t revision = 0;
    TableRange affected;
    bool cells = false, geometry = false, structure = false, headers = false;
    bool history = false, context = false, resize_state = false, reset = false;
};
using TableChangePort = ii::runtime::QueryPort<TableChange>;
enum class TableSortOrder { ascending, descending };
enum class TableContextAction { delete_row, delete_column };
struct TableContextItem final {
    std::string label, event_name;
    TableContextAction action = TableContextAction::delete_row;
    bool enabled = true, divider = false;
};

// Owner-thread, value-owned native model. No widget/OS/Qt types or polling.
// Read-only spans/references require that same thread and expire on mutation.
// Mutating methods return false for a valid no-op; invalid commands return an
// error without changing data/history. A receiver failure reports an already
// committed command; it must not be retried as if the command were rejected.
class TableModel final {
public:
    TableModel();
    ~TableModel();
    TableModel(const TableModel&) = delete;
    TableModel& operator=(const TableModel&) = delete;

    [[nodiscard]] ii::core::Result<TableModelState> state() const;
    [[nodiscard]] const TableModelOptions& options() const noexcept;
    [[nodiscard]] std::size_t row_count() const noexcept;
    [[nodiscard]] std::size_t column_count() const noexcept;
    [[nodiscard]] std::size_t header_count() const noexcept;
    [[nodiscard]] ii::core::Result<bool> set_options(TableModelOptions);
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const TableChangePort>);
    [[nodiscard]] ii::core::Result<TableRowData> row_at(std::size_t) const;
    [[nodiscard]] ii::core::Result<TableCellData> cell_at(TablePosition) const;
    [[nodiscard]] ii::core::Result<TableHeaderColumn> header_at(std::size_t) const;
    [[nodiscard]] ii::core::Result<std::string> column_type(std::size_t) const;
    [[nodiscard]] static std::string normalize_cell_type(std::string_view);
    [[nodiscard]] static TableValidation coerce_cell_value(const TableValue&, std::string_view);
    [[nodiscard]] ii::core::Result<TableValidation> validate_cell_input(TablePosition, const TableValue&) const;
    [[nodiscard]] ii::core::Result<std::optional<TableValue>> cell_raw_value(TablePosition) const;
    [[nodiscard]] ii::core::Result<std::string> cell_text(TablePosition) const;
    [[nodiscard]] ii::core::Result<bool> cell_inputable(TablePosition) const;
    [[nodiscard]] ii::core::Result<bool> set_cell_value(TablePosition, const TableValue&);
    [[nodiscard]] ii::core::Result<std::vector<std::vector<std::optional<TableValue>>>> range_values(TableRange) const;
    [[nodiscard]] ii::core::Result<bool> set_range_values(TablePosition, const std::vector<std::vector<TableValue>>&);
    [[nodiscard]] ii::core::Result<bool> sort_rows(std::size_t column, TableSortOrder);

    [[nodiscard]] ii::core::Result<double> column_width(std::size_t) const;
    [[nodiscard]] ii::core::Result<double> column_x(std::size_t) const;
    [[nodiscard]] ii::core::Result<double> row_height(std::size_t) const;
    [[nodiscard]] ii::core::Result<double> row_y(std::size_t) const;
    [[nodiscard]] ii::core::Result<std::optional<std::size_t>> row_at_y(double) const;
    [[nodiscard]] ii::core::Result<std::optional<std::size_t>> column_at_x(double) const;
    [[nodiscard]] ii::core::Result<std::optional<TablePosition>> position_at(double x,double y,bool resolve_merge=true) const;
    [[nodiscard]] double total_body_height() const noexcept;
    [[nodiscard]] double total_width() const noexcept;
    [[nodiscard]] std::span<const double> normalized_column_widths() const noexcept;
    [[nodiscard]] std::span<const double> normalized_row_heights() const noexcept;
    [[nodiscard]] ii::core::Result<TableCellGeometry> cell_geometry(TablePosition) const;
    [[nodiscard]] ii::core::Result<TablePosition> merge_anchor(TablePosition) const;
    // Includes intersecting merged anchors above/left of the viewport. Default
    // right covers the entire horizontal extent. Bounds are finite logical pixels.
    [[nodiscard]] ii::core::Result<std::vector<TableVisibleCell>> visible_cells(double top, double bottom, double left = 0, double right = 1e15) const;
    [[nodiscard]] ii::core::Result<bool> set_column_width(std::size_t, double);
    [[nodiscard]] ii::core::Result<bool> set_row_height(std::size_t, double);
    // A view/window resize updates automatic columns without resetting data or
    // undo history. Structure undo/redo keeps the current viewport width.
    [[nodiscard]] ii::core::Result<bool> set_table_width(double);
    [[nodiscard]] ii::core::Result<bool> begin_column_resize(std::size_t, double pointer);
    [[nodiscard]] ii::core::Result<bool> begin_row_resize(std::size_t, double pointer);
    [[nodiscard]] ii::core::Result<bool> update_column_resize(double pointer);
    [[nodiscard]] ii::core::Result<bool> update_row_resize(double pointer);
    [[nodiscard]] ii::core::Result<bool> end_resize();
    // Ownership cleanup may be requested while a change is being delivered.
    // It finishes at that delivery boundary; data mutations remain nonreentrant.
    [[nodiscard]] ii::core::Result<bool> request_end_resize();

    [[nodiscard]] bool can_merge_cells(TableRange) const noexcept;
    [[nodiscard]] ii::core::Result<bool> merge_cells(TableRange);
    [[nodiscard]] ii::core::Result<bool> split_cell(TablePosition);
    [[nodiscard]] ii::core::Result<bool> insert_row(std::size_t);
    [[nodiscard]] ii::core::Result<bool> delete_row(std::size_t);
    [[nodiscard]] ii::core::Result<bool> insert_column(std::size_t);
    [[nodiscard]] ii::core::Result<bool> delete_column(std::size_t);
    [[nodiscard]] ii::core::Result<bool> set_context_cell(TablePosition);
    [[nodiscard]] ii::core::Result<bool> clear_context_cell();
    [[nodiscard]] ii::core::Result<std::vector<TableContextItem>> context_menu(TablePosition) const;
    [[nodiscard]] ii::core::Result<bool> trigger_context_action(TableContextAction, TablePosition);
    [[nodiscard]] ii::core::Result<bool> undo();
    [[nodiscard]] ii::core::Result<bool> redo();
    [[nodiscard]] ii::core::Result<bool> clear_history();
    [[nodiscard]] ii::core::Result<bool> set_undo_limit(std::size_t);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
