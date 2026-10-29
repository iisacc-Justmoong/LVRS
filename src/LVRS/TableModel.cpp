#include "TableModel.h"
#include "Icons.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <thread>
#include <utility>
#include <unordered_map>

namespace lvrs {
using ii::core::ErrorCode;
using ii::core::Result;
namespace {
constexpr std::size_t maximum_rows = TableLimits::rows, maximum_columns = TableLimits::columns, maximum_cells = TableLimits::cells;
constexpr std::size_t maximum_visible_cells = 1000000;
constexpr double maximum_dimension = 1e9;
bool valid_text(std::string_view text) {
    return text.size() <= TableLimits::cell_bytes && text.find('\0') == text.npos && ii::core::valid_utf8(text);
}
bool valid_value(const TableValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) return valid_text(*text);
    if (const auto* number = std::get_if<double>(&value)) return std::isfinite(*number);
    return true;
}
std::string normalized_name(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == text.npos) return {};
    std::string result(text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1));
    for (char& letter : result) if (letter >= 'A' && letter <= 'Z') letter += 'a' - 'A';
    return result;
}
bool fits(TableRange range, std::size_t rows, std::size_t columns) {
    return range.row_count && range.column_count && range.row < rows && range.column < columns
        && range.row_count <= rows - range.row && range.column_count <= columns - range.column;
}
bool intersects(TableRange left, TableRange right) {
    return left.row < right.row + right.row_count && right.row < left.row + left.row_count
        && left.column < right.column + right.column_count && right.column < left.column + left.column_count;
}
std::uint64_t key(TablePosition position) {
    return std::uint64_t(position.row) * maximum_columns + position.column;
}
double numeric_size(double value, double fallback, double minimum) {
    return std::max(minimum, std::round(value > 0 ? value : fallback));
}
std::size_t columns_for(const TableModelOptions& options) {
    std::size_t result = std::max(std::size_t(1), options.columns.size());
    for (const auto& row : options.rows) result = std::max(result, row.cells.size());
    return result;
}
std::size_t cells_for(const TableModelOptions& options, std::size_t row) {
    return options.rows[row].cells.empty() ? std::max(std::size_t(1), options.columns.size()) : options.rows[row].cells.size();
}
void order_merges(TableModelOptions& options) {
    std::sort(options.merges.begin(), options.merges.end(), [](const TableRange& a, const TableRange& b) {
        return a.row < b.row || (a.row == b.row && a.column < b.column);
    });
}
// Fenwick sums: dimension edits, prefix positions and coordinate lookup use
// O(log N). Building or replacing the complete axis costs O(N).
struct AxisIndex final {
    std::vector<double> sizes, sums;
    void build(std::vector<double> values) {
        sizes = std::move(values);
        sums.assign(sizes.size() + 1, 0);
        for (std::size_t index = 1; index < sums.size(); ++index) {
            sums[index] += sizes[index - 1];
            const auto parent = index + (index & -index);
            if (parent < sums.size()) sums[parent] += sums[index];
        }
    }
    double prefix(std::size_t count) const noexcept {
        double result = 0;
        for (; count; count -= count & -count) result += sums[count];
        return result;
    }
    void replace(std::size_t index, double value) {
        const auto delta = value - sizes[index];
        sizes[index] = value;
        for (++index; index < sums.size(); index += index & -index) sums[index] += delta;
    }
    // First cell whose trailing edge is strictly greater than the coordinate.
    std::size_t at(double coordinate) const noexcept {
        if (coordinate < 0) return 0;
        std::size_t index = 0, step = 1;
        while (step <= sizes.size() / 2) step *= 2;
        double accumulated = 0;
        for (; step; step /= 2) {
            const auto next = index + step;
            if (next <= sizes.size() && accumulated + sums[next] <= coordinate) {
                index = next;
                accumulated += sums[next];
            }
        }
        return index;
    }
};
struct Coverage final {
    std::size_t first = 0, end = 0, covered_before = 0;
    TablePosition anchor;
};
struct Layout final {
    AxisIndex rows, columns;
    std::vector<std::vector<Coverage>> covered;
    std::vector<std::size_t> visible_prefix;
    std::unordered_map<std::uint64_t, TableRange> anchors;
    const Coverage* coverage(TablePosition position) const {
        const auto& intervals = covered[position.row];
        auto found = std::upper_bound(intervals.begin(), intervals.end(), position.column,
            [](std::size_t column, const Coverage& item) { return column < item.first; });
        if (found == intervals.begin()) return nullptr;
        --found;
        return position.column < found->end ? &*found : nullptr;
    }
    std::size_t covered_before(TablePosition position) const {
        const auto& intervals = covered[position.row];
        auto found = std::upper_bound(intervals.begin(), intervals.end(), position.column,
            [](std::size_t column, const Coverage& item) { return column < item.first; });
        if (found == intervals.begin()) return 0;
        --found;
        return found->covered_before + std::min(position.column, found->end) - found->first;
    }
};
Result<Layout> make_layout(const TableModelOptions& options) {
    if (options.rows.size() > maximum_rows || options.columns.size() > maximum_columns)
        return ii::core::fail(ErrorCode::full, "Table dimensions exceed the native model limits");
    if (!valid_text(options.default_cell_text) || !valid_text(options.default_header_text))
        return ii::core::fail(ErrorCode::invalid_argument, "Invalid table default text");
    for (double value : {options.table_width, options.row_height, options.cell_width, options.minimum_column_width, options.minimum_row_height})
        if (!std::isfinite(value) || value < 0 || value > maximum_dimension)
            return ii::core::fail(ErrorCode::invalid_argument, "Invalid table geometry");
    if (options.row_height < 1 || options.minimum_column_width < 1 || options.minimum_row_height < 1)
        return ii::core::fail(ErrorCode::invalid_argument, "Table default/minimum dimensions must be positive");
    const auto columns = columns_for(options);
    if (columns > maximum_columns || options.column_widths.size() > columns || options.row_heights.size() > options.rows.size())
        return ii::core::fail(ErrorCode::invalid_argument, "Dimension overrides exceed the table dimensions");
    for (const auto* values : {&options.column_widths, &options.row_heights})
        for (double value : *values) if (!std::isfinite(value) || std::abs(value) > maximum_dimension)
            return ii::core::fail(ErrorCode::invalid_argument, "Invalid table dimension override");
    for (const auto& column : options.columns)
        if (!valid_text(column.text) || !valid_text(column.value_type)
            || !valid_text(column.leading_icon) || !valid_text(column.trailing_icon)
            || (column.padding && (!std::isfinite(*column.padding) || *column.padding < 0 || *column.padding > maximum_dimension)))
            return ii::core::fail(ErrorCode::invalid_argument, "Invalid table header");
    for (const auto& column : options.columns)
        if ((!column.leading_icon.empty() && !Icons::data(column.leading_icon))
            || (!column.trailing_icon.empty() && !Icons::data(column.trailing_icon)))
            return ii::core::fail(ErrorCode::invalid_argument, "Unknown table header icon");
    std::size_t total = 0;
    for (std::size_t row = 0; row < options.rows.size(); ++row) {
        const auto count = cells_for(options, row);
        if (count > maximum_cells - total) return ii::core::fail(ErrorCode::full, "Table has more than 16000000 logical cells");
        total += count;
        for (const auto& cell : options.rows[row].cells)
            if ((cell.value && !valid_value(*cell.value)) || (cell.text && !valid_text(*cell.text)))
                return ii::core::fail(ErrorCode::invalid_argument, "Invalid table cell value/text");
    }
    Layout result;
    std::vector<double> widths(columns), heights(options.rows.size());
    const auto automatic = std::max(options.minimum_column_width, std::floor(options.table_width / columns));
    const auto fallback = options.cell_width > 0 ? options.cell_width : automatic;
    for (std::size_t column = 0; column < columns; ++column)
        widths[column] = numeric_size(column < options.column_widths.size() ? options.column_widths[column] : 0, fallback, options.minimum_column_width);
    for (std::size_t row = 0; row < heights.size(); ++row)
        heights[row] = numeric_size(row < options.row_heights.size() ? options.row_heights[row] : 0, options.row_height, options.minimum_row_height);
    result.columns.build(std::move(widths));
    result.rows.build(std::move(heights));
    result.covered.resize(options.rows.size());
    // Import validates complete intervals, including anchors, before deriving
    // coverage. A lower-left marker cannot hide an overlapping anchor.
    std::vector<std::vector<std::pair<std::size_t, std::size_t>>> occupied(options.rows.size());
    std::size_t merged_cells = 0;
    for (const auto& merge : options.merges) {
        if (!fits(merge, options.rows.size(), columns) || (merge.row_count == 1 && merge.column_count == 1))
            return ii::core::fail(ErrorCode::invalid_argument, "Invalid table merge range");
        if (merge.row_count > (total - merged_cells) / merge.column_count)
            return ii::core::fail(ErrorCode::invalid_argument, "Merged cells exceed the table's logical cell count");
        merged_cells += merge.row_count * merge.column_count;
        for (std::size_t row = merge.row; row < merge.row + merge.row_count; ++row) {
            if (merge.column + merge.column_count > options.rows[row].cells.size())
                return ii::core::fail(ErrorCode::invalid_argument, "Merge extends into a missing cell");
            occupied[row].emplace_back(merge.column, merge.column + merge.column_count);
            const auto first = merge.column + (row == merge.row ? 1 : 0);
            if (first < merge.column + merge.column_count)
                result.covered[row].push_back({first, merge.column + merge.column_count, 0, {merge.row, merge.column}});
        }
        result.anchors.emplace(key({merge.row, merge.column}), merge);
    }
    result.visible_prefix.assign(options.rows.size() + 1, 0);
    for (std::size_t row = 0; row < options.rows.size(); ++row) {
        auto& intervals = occupied[row];
        std::sort(intervals.begin(), intervals.end());
        for (std::size_t index = 1; index < intervals.size(); ++index)
            if (intervals[index - 1].second > intervals[index].first)
                return ii::core::fail(ErrorCode::invalid_argument, "Table merge ranges overlap");
        auto& coverage = result.covered[row];
        std::sort(coverage.begin(), coverage.end(), [](const Coverage& a, const Coverage& b) { return a.first < b.first; });
        std::size_t covered = 0;
        for (auto& interval : coverage) {
            interval.covered_before = covered;
            covered += interval.end - interval.first;
        }
        result.visible_prefix[row + 1] = result.visible_prefix[row] + cells_for(options, row) - covered;
    }
    return result;
}
}

struct TableModel::Impl final {
    TableModelOptions options;
    Layout layout;
    std::thread::id owner = std::this_thread::get_id();
    std::uint64_t revision = 1;
    bool notifying = false,release_resize_requested = false;
    std::optional<TablePosition> context;
    struct Resize final { bool column; std::size_t index; double pointer, start; bool recorded = false; };
    std::optional<Resize> resize;
    std::vector<std::weak_ptr<const TableChangePort>> receivers;
    struct CellEdit final { TablePosition position; TableCellData other; };
    struct CellPatch final { std::vector<CellEdit> edits; };
    struct AxisPatch final { bool column; std::size_t index; double other; };
    struct StatePatch final { TableModelOptions other; Layout other_layout; };
    using Patch = std::variant<CellPatch, AxisPatch, StatePatch>;
    struct Command final { Patch patch; TableChange change; };
    std::vector<Command> history;
    std::size_t cursor = 0, limit = 128;

    Impl() : layout(std::move(*make_layout(options))) {
        options.column_widths.resize(layout.columns.sizes.size(), 0);
        options.row_heights.resize(options.rows.size(), 0);
    }
    Result<void> ready(bool mutation = false, bool resizing = false) const {
        if (owner != std::this_thread::get_id()) return ii::core::fail(ErrorCode::wrong_executor, "Table model requires its owning thread");
        if (mutation && notifying) return ii::core::fail(ErrorCode::busy, "Table mutation cannot recurse from a change receiver");
        if (mutation && resize && !resizing) return ii::core::fail(ErrorCode::busy, "Finish the active table resize before another command");
        return {};
    }
    bool logical_cell(TablePosition position) const {
        return position.row < options.rows.size() && position.column < cells_for(options, position.row);
    }
    bool stored_cell(TablePosition position) const {
        return position.row < options.rows.size() && position.column < options.rows[position.row].cells.size();
    }
    std::string type(std::size_t column) const {
        return column < options.columns.size() ? TableModel::normalize_cell_type(options.columns[column].value_type) : "string";
    }
    TableCellData default_cell(std::size_t column) const {
        const auto value_type = type(column);
        TableValue value = options.default_cell_text;
        if (value_type == "int") value = std::int64_t(0);
        else if (value_type == "float") value = 0.0;
        else if (value_type == "bool") value = false;
        auto validated = TableModel::coerce_cell_value(value, value_type);
        return {.value = std::move(validated.value), .text = std::move(validated.text)};
    }
    std::optional<TableValue> raw(TablePosition position) const {
        if (!stored_cell(position)) return {};
        const auto& cell = options.rows[position.row].cells[position.column];
        if (cell.value) return cell.value;
        if (cell.text) return *cell.text;
        return {};
    }
    std::string text(TablePosition position) const {
        const auto value = raw(position);
        if (!value) return options.default_cell_text;
        auto validated = TableModel::coerce_cell_value(*value, type(position.column));
        return validated.accepted ? std::move(validated.text) : table_value_text(*value);
    }
    bool inputable(TablePosition position) const {
        if (stored_cell(position)) {
            const auto& cell = options.rows[position.row].cells[position.column];
            if (cell.inputable) return *cell.inputable;
        }
        return options.rows[position.row].inputable.value_or(options.inputable);
    }
    TableCellGeometry geometry(TablePosition position) const {
        TableCellGeometry result;
        if (const auto* coverage = layout.coverage(position)) {
            result.covered = true;
            position = coverage->anchor;
        }
        if (const auto found = layout.anchors.find(key(position)); found != layout.anchors.end()) {
            result.row_span = found->second.row_count;
            result.column_span = found->second.column_count;
        }
        result.x = layout.columns.prefix(position.column);
        result.y = layout.rows.prefix(position.row);
        result.width = layout.columns.prefix(position.column + result.column_span) - result.x;
        result.height = layout.rows.prefix(position.row + result.row_span) - result.y;
        return result;
    }
    TableRange all() const { return {0, 0, options.rows.size(), layout.columns.sizes.size()}; }
    Result<void> notify(TableChange change) {
        change.revision = revision;
        std::array<std::shared_ptr<const TableChangePort>, 256> pending;
        std::size_t count = 0;
        std::erase_if(receivers, [](const auto& item) { return item.expired(); });
        for (const auto& weak : receivers) if (auto port = weak.lock()) pending[count++] = std::move(port);
        notifying = true;
        struct Reset final { bool& flag; ~Reset() { flag = false; } } reset{notifying};
        std::optional<ii::runtime::QueryError> failure;
        for (std::size_t index = 0; index < count; ++index) {
            auto result = pending[index]->send(change);
            if (!result && result.error().code != ii::runtime::QueryErrorCode::receiver_expired && !failure) failure = result.error();
        }
        if (failure) return ii::core::fail(ErrorCode::failed, "Table command committed; a change receiver failed", failure->cause);
        return {};
    }
    Result<bool> publish(TableChange change) {
        ++revision;
        auto result=notify(change);
        if(std::exchange(release_resize_requested,false)&&resize){
            resize.reset();++revision;
            auto ended=notify({.resize_state=true});
            if(result&&!ended)result=std::move(ended);
        }
        if(!result)return std::unexpected(result.error());
        return true;
    }
    void apply(Patch& patch) {
        std::visit([&](auto& command) {
            using T = std::decay_t<decltype(command)>;
            if constexpr (std::is_same_v<T, CellPatch>) {
                for (auto& edit : command.edits) std::swap(options.rows[edit.position.row].cells[edit.position.column], edit.other);
            } else if constexpr (std::is_same_v<T, AxisPatch>) {
                auto& values = command.column ? options.column_widths : options.row_heights;
                std::swap(values[command.index], command.other);
                update_axis(command.column, command.index);
            } else {
                const auto viewport_width = options.table_width;
                const auto cached_width = command.other.table_width;
                std::swap(options, command.other);
                std::swap(layout, command.other_layout);
                options.table_width = viewport_width;
                if (cached_width != viewport_width) rebuild_columns();
            }
        }, patch);
    }
    void rebuild_columns() {
        const auto count = layout.columns.sizes.size();
        const auto automatic = std::max(options.minimum_column_width, std::floor(options.table_width / count));
        const auto fallback = options.cell_width > 0 ? options.cell_width : automatic;
        std::vector<double> widths(count);
        for (std::size_t column = 0; column < count; ++column)
            widths[column] = numeric_size(options.column_widths[column], fallback, options.minimum_column_width);
        layout.columns.build(std::move(widths));
    }
    void update_axis(bool column, std::size_t index) {
        if (column) {
            const auto automatic = std::max(options.minimum_column_width, std::floor(options.table_width / layout.columns.sizes.size()));
            layout.columns.replace(index, numeric_size(options.column_widths[index], options.cell_width > 0 ? options.cell_width : automatic, options.minimum_column_width));
        } else layout.rows.replace(index, numeric_size(options.row_heights[index], options.row_height, options.minimum_row_height));
    }
    Result<bool> commit(Patch patch, TableChange change) {
        change.history = true;
        // Reserve before changing data. Redo is retired only for a valid edit.
        if (limit) history.reserve(std::min(limit + 1, history.size() + 1));
        history.erase(history.begin() + cursor, history.end());
        apply(patch);
        if (limit) {
            history.push_back({std::move(patch), change});
            if (history.size() > limit) history.erase(history.begin());
        }
        cursor = history.size();
        if (change.structure) { context.reset(); change.context = true; }
        return publish(change);
    }
    Result<bool> replace(TableModelOptions next, TableChange change) {
        order_merges(next);
        auto next_layout = make_layout(next);
        if (!next_layout) return std::unexpected(next_layout.error());
        next.column_widths.resize(next_layout->columns.sizes.size(), 0);
        next.row_heights.resize(next.rows.size(), 0);
        if (next == options) return false;
        return commit(StatePatch{std::move(next), std::move(*next_layout)}, change);
    }
};

TableModel::TableModel() : impl_(std::make_unique<Impl>()) {}
TableModel::~TableModel() = default;
const TableModelOptions& TableModel::options() const noexcept { return impl_->options; }
std::size_t TableModel::row_count() const noexcept { return impl_->options.rows.size(); }
std::size_t TableModel::column_count() const noexcept { return impl_->layout.columns.sizes.size(); }
std::size_t TableModel::header_count() const noexcept { return impl_->options.columns.size(); }
Result<TableModelState> TableModel::state() const {
    const auto& model = *impl_;
    if (auto result = model.ready(); !result) return std::unexpected(result.error());
    TableModelState result{.revision = model.revision, .row_count = row_count(), .column_count = column_count(),
        .header_count = header_count(), .undo_depth = model.cursor, .redo_depth = model.history.size() - model.cursor,
        .undo_limit = model.limit, .context_cell = model.context};
    if (model.resize) {
        if (model.resize->column) result.resizing_column = model.resize->index;
        else result.resizing_row = model.resize->index;
    }
    return result;
}
Result<bool> TableModel::set_options(TableModelOptions options) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    order_merges(options);
    auto layout = make_layout(options);
    if (!layout) return std::unexpected(layout.error());
    options.column_widths.resize(layout->columns.sizes.size(), 0);
    options.row_heights.resize(options.rows.size(), 0);
    if (options == model.options) return false;
    model.options = std::move(options);
    model.layout = std::move(*layout);
    model.history.clear(); model.cursor = 0; model.context.reset();
    return model.publish({.affected = model.all(), .cells = true, .geometry = true, .structure = true, .headers = true, .history = true, .context = true, .reset = true});
}
Result<void> TableModel::observe(std::shared_ptr<const TableChangePort> port) {
    auto& model = *impl_;
    if (auto result = model.ready(); !result) return result;
    if (!port) return ii::core::fail(ErrorCode::invalid_argument, "Table receiver must be nonnull");
    std::erase_if(model.receivers, [](const auto& item) { return item.expired(); });
    for (const auto& weak : model.receivers) if (weak.lock() == port) return {};
    if (model.receivers.size() == 256) return ii::core::fail(ErrorCode::full, "Table model has 256 live change receivers");
    model.receivers.push_back(std::move(port));
    return {};
}
Result<TableRowData> TableModel::row_at(std::size_t row) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (row >= row_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid table row");
    return impl_->options.rows[row];
}
Result<TableCellData> TableModel::cell_at(TablePosition position) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (!impl_->logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid table cell");
    return impl_->stored_cell(position) ? impl_->options.rows[position.row].cells[position.column] : TableCellData{.value = std::nullopt};
}
Result<TableHeaderColumn> TableModel::header_at(std::size_t column) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (column >= header_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid table header");
    return impl_->options.columns[column];
}
std::string TableModel::normalize_cell_type(std::string_view name) {
    auto normalized = normalized_name(name);
    if (normalized == "int" || normalized == "integer") return "int";
    if (normalized == "float" || normalized == "real" || normalized == "double" || normalized == "number" || normalized == "decimal") return "float";
    if (normalized == "bool" || normalized == "boolean") return "bool";
    return "string";
}
TableValidation TableModel::coerce_cell_value(const TableValue& value, std::string_view name) {
    if (!valid_value(value)) return {false, {}, value};
    auto text = table_value_text(value);
    const auto type = normalize_cell_type(name);
    if (type == "bool") {
        const auto normalized = normalized_name(text);
        if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on") return {true, "true", true};
        if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off") return {true, "false", false};
        return {false, std::move(text), value};
    }
    auto result = validate_table_value(text, type);
    if (result.accepted && type == "float") result.text = table_value_text(result.value);
    return result;
}
Result<std::string> TableModel::column_type(std::size_t column) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (column >= column_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid table column");
    return impl_->type(column);
}
Result<TableValidation> TableModel::validate_cell_input(TablePosition position, const TableValue& value) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (!impl_->logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid table cell");
    return coerce_cell_value(value, impl_->type(position.column));
}
Result<std::optional<TableValue>> TableModel::cell_raw_value(TablePosition position) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (!impl_->logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid table cell");
    return impl_->raw(position);
}
Result<std::string> TableModel::cell_text(TablePosition position) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (!impl_->logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid table cell");
    return impl_->text(position);
}
Result<bool> TableModel::cell_inputable(TablePosition position) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (!impl_->logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid table cell");
    return impl_->inputable(position);
}
Result<double> TableModel::column_width(std::size_t column) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (column >= column_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid table column");
    return impl_->layout.columns.sizes[column];
}
Result<double> TableModel::column_x(std::size_t column) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (column >= column_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid table column");
    return impl_->layout.columns.prefix(column);
}
Result<double> TableModel::row_height(std::size_t row) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (row >= row_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid table row");
    return impl_->layout.rows.sizes[row];
}
Result<double> TableModel::row_y(std::size_t row) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (row >= row_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid table row");
    return impl_->layout.rows.prefix(row);
}
Result<std::optional<std::size_t>> TableModel::row_at_y(double y) const {
    if(auto result=impl_->ready();!result)return std::unexpected(result.error());
    if(!std::isfinite(y))return ii::core::fail(ErrorCode::invalid_argument,"Table row coordinate must be finite");
    if(y<0||y>=total_body_height())return std::nullopt;
    return impl_->layout.rows.at(y);
}
Result<std::optional<std::size_t>> TableModel::column_at_x(double x) const {
    if(auto result=impl_->ready();!result)return std::unexpected(result.error());
    if(!std::isfinite(x))return ii::core::fail(ErrorCode::invalid_argument,"Table column coordinate must be finite");
    if(x<0||x>=total_width())return std::nullopt;
    return impl_->layout.columns.at(x);
}
Result<std::optional<TablePosition>> TableModel::position_at(double x,double y,bool resolve_merge) const {
    if(auto result=impl_->ready();!result)return std::unexpected(result.error());
    if(!std::isfinite(x)||!std::isfinite(y))return ii::core::fail(ErrorCode::invalid_argument,"Table coordinates must be finite");
    if(x<0||y<0||x>=total_width()||y>=total_body_height())return std::nullopt;
    TablePosition position{impl_->layout.rows.at(y),impl_->layout.columns.at(x)};
    if(!impl_->logical_cell(position))return std::nullopt;
    if(resolve_merge)if(const auto* coverage=impl_->layout.coverage(position))position=coverage->anchor;
    return position;
}
double TableModel::total_body_height() const noexcept { return impl_->layout.rows.prefix(row_count()); }
double TableModel::total_width() const noexcept { return impl_->layout.columns.prefix(column_count()); }
std::span<const double> TableModel::normalized_column_widths() const noexcept { return impl_->layout.columns.sizes; }
std::span<const double> TableModel::normalized_row_heights() const noexcept { return impl_->layout.rows.sizes; }
Result<TableCellGeometry> TableModel::cell_geometry(TablePosition position) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (!impl_->logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid table cell");
    return impl_->geometry(position);
}
Result<TablePosition> TableModel::merge_anchor(TablePosition position) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (!impl_->logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid table cell");
    if (const auto* coverage = impl_->layout.coverage(position)) return coverage->anchor;
    return position;
}

Result<bool> TableModel::set_cell_value(TablePosition position, const TableValue& value) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (!model.stored_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Cannot edit a missing table cell");
    auto validated = coerce_cell_value(value, model.type(position.column));
    if (!validated.accepted) return ii::core::fail(ErrorCode::invalid_argument, "Cell value does not match its column type");
    auto next = model.options.rows[position.row].cells[position.column];
    next.value = std::move(validated.value);
    next.text = std::move(validated.text);
    if (next == model.options.rows[position.row].cells[position.column]) return false;
    Impl::CellPatch patch;
    patch.edits.push_back({position, std::move(next)});
    return model.commit(std::move(patch), {.affected = {position.row, position.column, 1, 1}, .cells = true});
}
Result<std::vector<std::vector<std::optional<TableValue>>>> TableModel::range_values(TableRange range) const {
    const auto& model = *impl_;
    if (auto result = model.ready(); !result) return std::unexpected(result.error());
    if (!fits(range, row_count(), column_count())) return ii::core::fail(ErrorCode::out_of_range, "Invalid table range");
    std::vector<std::vector<std::optional<TableValue>>> result(range.row_count);
    for (std::size_t offset = 0; offset < range.row_count; ++offset) {
        const auto row = range.row + offset;
        if (range.column + range.column_count > model.options.rows[row].cells.size())
            return ii::core::fail(ErrorCode::out_of_range, "Table range includes a missing cell");
        auto& values = result[offset];
        values.reserve(range.column_count);
        for (std::size_t column = range.column; column < range.column + range.column_count; ++column)
            values.push_back(model.raw({row, column}));
    }
    return result;
}
Result<bool> TableModel::set_range_values(TablePosition first, const std::vector<std::vector<TableValue>>& values) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (values.empty() || first.row >= row_count() || values.size() > row_count() - first.row || first.column >= column_count())
        return ii::core::fail(ErrorCode::out_of_range, "Invalid table paste range");
    Impl::CellPatch patch;
    std::size_t width = 0;
    for (std::size_t row = 0; row < values.size(); ++row) {
        if (values[row].empty() || values[row].size() > column_count() - first.column)
            return ii::core::fail(ErrorCode::invalid_argument, "Empty or out-of-range table paste row");
        width = std::max(width, values[row].size());
        for (std::size_t column = 0; column < values[row].size(); ++column) {
            const TablePosition position{first.row + row, first.column + column};
            if (!model.stored_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Table paste includes a missing cell");
            const auto geometry = model.geometry(position);
            if (geometry.covered || geometry.row_span > 1 || geometry.column_span > 1)
                return ii::core::fail(ErrorCode::invalid_argument, "Split merged cells before pasting a range");
            auto validated = coerce_cell_value(values[row][column], model.type(position.column));
            if (!validated.accepted) return ii::core::fail(ErrorCode::invalid_argument, "Table paste contains an invalid typed value");
            auto next = model.options.rows[position.row].cells[position.column];
            next.value = std::move(validated.value);
            next.text = std::move(validated.text);
            if (next != model.options.rows[position.row].cells[position.column]) patch.edits.push_back({position, std::move(next)});
        }
    }
    if (patch.edits.empty()) return false;
    return model.commit(std::move(patch), {.affected = {first.row, first.column, values.size(), width}, .cells = true});
}
Result<std::vector<TableVisibleCell>> TableModel::visible_cells(double top, double bottom, double left, double right) const {
    const auto& model = *impl_;
    if (auto result = model.ready(); !result) return std::unexpected(result.error());
    for (double coordinate : {top, bottom, left, right}) if (!std::isfinite(coordinate))
        return ii::core::fail(ErrorCode::invalid_argument, "Table viewport coordinates must be finite");
    std::vector<TableVisibleCell> result;
    if (bottom <= top || right <= left || bottom <= 0 || right <= 0 || top >= total_body_height() || left >= total_width()) return result;
    const auto first_row = model.layout.rows.at(top), first_column = model.layout.columns.at(left);
    const auto last_row = std::min(row_count(), model.layout.rows.at(std::nextafter(bottom, -std::numeric_limits<double>::infinity())) + 1);
    const auto last_column = std::min(column_count(), model.layout.columns.at(std::nextafter(right, -std::numeric_limits<double>::infinity())) + 1);
    auto append = [&](TablePosition position) -> Result<void> {
        const auto geometry = model.geometry(position);
        if (geometry.covered || geometry.x >= right || geometry.y >= bottom || geometry.x + geometry.width <= left || geometry.y + geometry.height <= top) return {};
        if (result.size() >= maximum_visible_cells) return ii::core::fail(ErrorCode::full, "Table viewport has more than 1000000 visible anchors");
        TableCellData data{.value = std::nullopt};
        if (model.stored_cell(position)) data = model.options.rows[position.row].cells[position.column];
        const auto index = model.layout.visible_prefix[position.row] + position.column - model.layout.covered_before(position);
        result.push_back({index, position, geometry, std::move(data), model.text(position), model.type(position.column), model.inputable(position)});
        return {};
    };
    for (std::size_t row = first_row; row < last_row; ++row) {
        const auto count = std::min(last_column, cells_for(model.options, row));
        for (std::size_t column = first_column; column < count; ++column)
            if (auto appended = append({row, column}); !appended) return std::unexpected(appended.error());
    }
    // An anchor may start before the first ordinary visible row/column while
    // its merged rectangle still intersects the viewport.
    for (const auto& merge : model.options.merges)
        if (merge.row < first_row || merge.column < first_column)
            if (auto appended = append({merge.row, merge.column}); !appended) return std::unexpected(appended.error());
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return a.position.row < b.position.row || (a.position.row == b.position.row && a.position.column < b.position.column);
    });
    return result;
}
Result<bool> TableModel::sort_rows(std::size_t column, TableSortOrder order) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (column >= column_count() || (order != TableSortOrder::ascending && order != TableSortOrder::descending))
        return ii::core::fail(ErrorCode::invalid_argument, "Invalid table sort column/order");
    if (row_count() < 2 && model.options.merges.empty()) return false;
    const auto type = model.type(column);
    struct SortKey final { int group = 0; TableValue value; std::string text; };
    std::vector<SortKey> keys;
    keys.reserve(row_count());
    for (std::size_t row = 0; row < row_count(); ++row) {
        const auto value = model.raw({row, column});
        if (!value) { keys.push_back({2, std::string{}, {}}); continue; }
        auto validated = coerce_cell_value(*value, type);
        keys.push_back({validated.accepted ? 0 : 1, std::move(validated.value), table_value_text(*value)});
    }
    std::vector<std::size_t> permutation(row_count());
    std::iota(permutation.begin(), permutation.end(), 0);
    std::stable_sort(permutation.begin(), permutation.end(), [&](std::size_t left, std::size_t right) {
        const auto& a = keys[left]; const auto& b = keys[right];
        if (a.group != b.group) return a.group < b.group; // Empty/invalid groups stay last in both orders.
        if (a.group == 2) return false;
        int compared = 0;
        if (a.group == 0 && type == "int") {
            const auto x = std::get<std::int64_t>(a.value), y = std::get<std::int64_t>(b.value);
            compared = x < y ? -1 : x > y ? 1 : 0;
        } else if (a.group == 0 && type == "float") {
            const auto x = std::get<double>(a.value), y = std::get<double>(b.value);
            compared = x < y ? -1 : x > y ? 1 : 0;
        } else if (a.group == 0 && type == "bool") {
            const auto x = std::get<bool>(a.value), y = std::get<bool>(b.value);
            compared = x == y ? 0 : x ? 1 : -1;
        } else compared = a.text.compare(b.text);
        return order == TableSortOrder::ascending ? compared < 0 : compared > 0;
    });
    bool changed = !model.options.merges.empty();
    for (std::size_t index = 0; index < permutation.size(); ++index) changed |= permutation[index] != index;
    if (!changed) return false;
    auto next = model.options;
    for (std::size_t index = 0; index < permutation.size(); ++index) {
        next.rows[index] = model.options.rows[permutation[index]];
        next.row_heights[index] = model.options.row_heights[permutation[index]];
    }
    next.merges.clear();
    return model.replace(std::move(next), {.affected = model.all(), .cells = true, .geometry = true, .structure = true});
}
Result<bool> TableModel::set_column_width(std::size_t column, double value) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (column >= column_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid table column");
    if (!std::isfinite(value) || std::abs(value) > maximum_dimension) return ii::core::fail(ErrorCode::invalid_argument, "Invalid table width");
    const auto next = numeric_size(value, model.layout.columns.sizes[column], model.options.minimum_column_width);
    if (next == model.layout.columns.sizes[column]) return false;
    return model.commit(Impl::AxisPatch{true, column, next}, {.affected = {0, column, row_count(), 1}, .geometry = true});
}
Result<bool> TableModel::set_row_height(std::size_t row, double value) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (row >= row_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid table row");
    if (!std::isfinite(value) || std::abs(value) > maximum_dimension) return ii::core::fail(ErrorCode::invalid_argument, "Invalid table height");
    const auto next = numeric_size(value, model.layout.rows.sizes[row], model.options.minimum_row_height);
    if (next == model.layout.rows.sizes[row]) return false;
    return model.commit(Impl::AxisPatch{false, row, next}, {.affected = {row, 0, 1, column_count()}, .geometry = true});
}
Result<bool> TableModel::set_table_width(double width) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (!std::isfinite(width) || width < 0 || width > maximum_dimension)
        return ii::core::fail(ErrorCode::invalid_argument, "Invalid table viewport width");
    if (width == model.options.table_width) return false;
    model.options.table_width = width;
    model.rebuild_columns();
    return model.publish({.affected = model.all(), .geometry = true});
}
Result<bool> TableModel::begin_column_resize(std::size_t column, double pointer) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (column >= column_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid resize column");
    if (!std::isfinite(pointer) || std::abs(pointer) > 1e15) return ii::core::fail(ErrorCode::invalid_argument, "Invalid resize pointer");
    model.resize = Impl::Resize{true, column, pointer, model.layout.columns.sizes[column]};
    return model.publish({.resize_state = true});
}
Result<bool> TableModel::begin_row_resize(std::size_t row, double pointer) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (row >= row_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid resize row");
    if (!std::isfinite(pointer) || std::abs(pointer) > 1e15) return ii::core::fail(ErrorCode::invalid_argument, "Invalid resize pointer");
    model.resize = Impl::Resize{false, row, pointer, model.layout.rows.sizes[row]};
    return model.publish({.resize_state = true});
}
Result<bool> TableModel::update_column_resize(double pointer) {
    auto& model = *impl_;
    if (auto result = model.ready(true, true); !result) return std::unexpected(result.error());
    if (!model.resize || !model.resize->column) return ii::core::fail(ErrorCode::invalid_argument, "No active column resize");
    if (!std::isfinite(pointer) || std::abs(pointer) > 1e15) return ii::core::fail(ErrorCode::invalid_argument, "Invalid resize pointer");
    const auto next = std::max(model.options.minimum_column_width, std::round(model.resize->start + pointer - model.resize->pointer));
    if (next > maximum_dimension) return ii::core::fail(ErrorCode::out_of_range, "Resized column exceeds the dimension limit");
    const auto column = model.resize->index;
    if (next == model.layout.columns.sizes[column]) return false;
    TableChange change{.affected = {0, column, row_count(), 1}, .geometry = true};
    if (!model.resize->recorded) {
        model.resize->recorded = true;
        return model.commit(Impl::AxisPatch{true, column, next}, change);
    }
    model.options.column_widths[column] = next;
    model.update_axis(true, column);
    return model.publish(change);
}
Result<bool> TableModel::update_row_resize(double pointer) {
    auto& model = *impl_;
    if (auto result = model.ready(true, true); !result) return std::unexpected(result.error());
    if (!model.resize || model.resize->column) return ii::core::fail(ErrorCode::invalid_argument, "No active row resize");
    if (!std::isfinite(pointer) || std::abs(pointer) > 1e15) return ii::core::fail(ErrorCode::invalid_argument, "Invalid resize pointer");
    const auto next = std::max(model.options.minimum_row_height, std::round(model.resize->start + pointer - model.resize->pointer));
    if (next > maximum_dimension) return ii::core::fail(ErrorCode::out_of_range, "Resized row exceeds the dimension limit");
    const auto row = model.resize->index;
    if (next == model.layout.rows.sizes[row]) return false;
    TableChange change{.affected = {row, 0, 1, column_count()}, .geometry = true};
    if (!model.resize->recorded) {
        model.resize->recorded = true;
        return model.commit(Impl::AxisPatch{false, row, next}, change);
    }
    model.options.row_heights[row] = next;
    model.update_axis(false, row);
    return model.publish(change);
}
Result<bool> TableModel::end_resize() {
    auto& model = *impl_;
    if (auto result = model.ready(true, true); !result) return std::unexpected(result.error());
    if (!model.resize) return false;
    model.release_resize_requested=false;model.resize.reset();
    return model.publish({.resize_state = true});
}
Result<bool> TableModel::request_end_resize(){
    auto& model=*impl_;if(auto result=model.ready();!result)return std::unexpected(result.error());
    if(!model.resize)return false;
    if(model.notifying){model.release_resize_requested=true;return true;}
    return end_resize();
}

bool TableModel::can_merge_cells(TableRange range) const noexcept {
    if (!fits(range, row_count(), column_count())) return false;
    for (std::size_t row = range.row; row < range.row + range.row_count; ++row)
        if (range.column + range.column_count > impl_->options.rows[row].cells.size()) return false;
    return true;
}
Result<bool> TableModel::merge_cells(TableRange range) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (!can_merge_cells(range)) return ii::core::fail(ErrorCode::out_of_range, "Invalid merge range");
    auto next = model.options;
    std::erase_if(next.merges, [&](const auto& merge) { return intersects(merge, range); });
    if (range.row_count > 1 || range.column_count > 1) next.merges.push_back(range);
    return model.replace(std::move(next), {.affected = model.all(), .cells = true, .geometry = true});
}
Result<bool> TableModel::split_cell(TablePosition position) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (!model.logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid split cell");
    if (const auto* coverage = model.layout.coverage(position)) position = coverage->anchor;
    auto found = model.layout.anchors.find(key(position));
    if (found == model.layout.anchors.end()) return false;
    const auto range = found->second;
    auto next = model.options;
    std::erase(next.merges, range);
    return model.replace(std::move(next), {.affected = range, .cells = true, .geometry = true});
}
Result<bool> TableModel::insert_row(std::size_t row) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (row > row_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid inserted row");
    if (row_count() == maximum_rows) return ii::core::fail(ErrorCode::full, "Table row limit reached");
    auto next = model.options;
    TableRowData added;
    added.cells.clear(); added.cells.reserve(column_count());
    for (std::size_t column = 0; column < column_count(); ++column) added.cells.push_back(model.default_cell(column));
    next.rows.insert(next.rows.begin() + row, std::move(added));
    next.row_heights.insert(next.row_heights.begin() + row, 0);
    next.merges.clear();
    return model.replace(std::move(next), {.affected = {row, 0, row_count() + 1 - row, column_count()}, .cells = true, .geometry = true, .structure = true});
}
Result<bool> TableModel::delete_row(std::size_t row) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (row >= row_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid deleted row");
    auto next = model.options;
    next.rows.erase(next.rows.begin() + row);
    next.row_heights.erase(next.row_heights.begin() + row);
    next.merges.clear();
    return model.replace(std::move(next), {.affected = {row, 0, row_count() - row, column_count()}, .cells = true, .geometry = true, .structure = true});
}
Result<bool> TableModel::insert_column(std::size_t column) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (column > column_count()) return ii::core::fail(ErrorCode::out_of_range, "Invalid inserted column");
    if (column_count() == maximum_columns) return ii::core::fail(ErrorCode::full, "Table column limit reached");
    auto next = model.options;
    for (auto& row : next.rows) {
        while (row.cells.size() < column_count()) row.cells.push_back(model.default_cell(row.cells.size()));
        row.cells.insert(row.cells.begin() + column, model.default_cell(column));
    }
    while (next.columns.size() < column_count()) next.columns.push_back({.text = next.default_header_text, .value_type = "string"});
    next.columns.insert(next.columns.begin() + column, {.text = next.default_header_text, .value_type = "string"});
    next.column_widths.insert(next.column_widths.begin() + column, 0);
    next.merges.clear();
    return model.replace(std::move(next), {.affected = {0, column, row_count(), column_count() + 1 - column}, .cells = true, .geometry = true, .structure = true, .headers = true});
}
Result<bool> TableModel::delete_column(std::size_t column) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (column >= column_count() || column_count() == 1) return ii::core::fail(ErrorCode::out_of_range, "Cannot delete that table column");
    auto next = model.options;
    for (auto& row : next.rows) if (column < row.cells.size()) row.cells.erase(row.cells.begin() + column);
    if (column < next.columns.size()) next.columns.erase(next.columns.begin() + column);
    next.column_widths.erase(next.column_widths.begin() + column);
    next.merges.clear();
    // Ragged rows may leave fewer logical columns than the removed override.
    next.column_widths.resize(columns_for(next));
    return model.replace(std::move(next), {.affected = {0, column, row_count(), column_count() - column}, .cells = true, .geometry = true, .structure = true, .headers = true});
}
Result<bool> TableModel::set_context_cell(TablePosition position) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (!model.logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid context cell");
    if (model.context == position) return false;
    model.context = position;
    return model.publish({.affected = {position.row, position.column, 1, 1}, .context = true});
}
Result<bool> TableModel::clear_context_cell() {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (!model.context) return false;
    model.context.reset();
    return model.publish({.context = true});
}
Result<std::vector<TableContextItem>> TableModel::context_menu(TablePosition position) const {
    if (auto result = impl_->ready(); !result) return std::unexpected(result.error());
    if (!impl_->logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid context cell");
    return std::vector<TableContextItem>{
        {"Delete row", "table.deleteRow", TableContextAction::delete_row, true, false},
        {{}, {}, TableContextAction::delete_row, false, true},
        {"Delete column", "table.deleteColumn", TableContextAction::delete_column, column_count() > 1, false}
    };
}
Result<bool> TableModel::trigger_context_action(TableContextAction action, TablePosition position) {
    if (auto result = impl_->ready(true); !result) return std::unexpected(result.error());
    if (!impl_->logical_cell(position)) return ii::core::fail(ErrorCode::out_of_range, "Invalid context target");
    switch (action) {
    case TableContextAction::delete_row: return delete_row(position.row);
    case TableContextAction::delete_column: return delete_column(position.column);
    }
    return ii::core::fail(ErrorCode::invalid_argument, "Unknown table context action");
}
Result<bool> TableModel::undo() {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (model.cursor == 0) return false;
    auto& command = model.history[--model.cursor];
    model.apply(command.patch);
    auto change = command.change;
    if (change.structure) { model.context.reset(); change.context = true; }
    return model.publish(change);
}
Result<bool> TableModel::redo() {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (model.cursor == model.history.size()) return false;
    auto& command = model.history[model.cursor++];
    model.apply(command.patch);
    auto change = command.change;
    if (change.structure) { model.context.reset(); change.context = true; }
    return model.publish(change);
}
Result<bool> TableModel::clear_history() {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (model.history.empty()) return false;
    model.history.clear(); model.cursor = 0;
    return model.publish({.history = true});
}
Result<bool> TableModel::set_undo_limit(std::size_t limit) {
    auto& model = *impl_;
    if (auto result = model.ready(true); !result) return std::unexpected(result.error());
    if (limit > 65536) return ii::core::fail(ErrorCode::invalid_argument, "Table undo limit exceeds 65536 commands");
    if (model.limit == limit) return false;
    model.limit = limit;
    // Keep the nearest undo and redo commands; dropping the oldest undo state
    // must not leave a redo chain whose preceding state was discarded.
    if (model.cursor > limit) {
        const auto excess = model.cursor - limit;
        model.history.erase(model.history.begin(), model.history.begin() + excess);
        model.cursor -= excess;
    }
    if (model.history.size() - model.cursor > limit) model.history.resize(model.cursor + limit);
    return model.publish({.history = true});
}
}
