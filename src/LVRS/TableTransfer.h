#pragma once
#include "TableModel.h"

namespace lvrs {
using TableMatrix=std::vector<std::vector<std::optional<TableValue>>>;
using TableInputMatrix=std::vector<std::vector<TableValue>>;
// A1 references address the model's bounded domain. ASCII address whitespace
// is accepted; zero rows, leading zero row numbers and overflow are rejected.
[[nodiscard]] ii::core::Result<std::string> table_column_name(std::size_t);
[[nodiscard]] ii::core::Result<std::size_t> table_column_index(std::string_view);
[[nodiscard]] ii::core::Result<std::string> table_cell_reference(TablePosition);
[[nodiscard]] ii::core::Result<TablePosition> table_cell_coordinates(std::string_view);
// TSV is a text transfer, not a typed archive: missing values become empty
// strings and target column types are validated by TableModel during paste.
// Quote/CRLF/trailing-row handling follows the original Table.qml codec.
[[nodiscard]] ii::core::Result<std::string> table_values_as_tsv(const TableMatrix&);
// Streams cells into a bounded output instead of copying a complete selection.
[[nodiscard]] ii::core::Result<std::string> table_range_as_tsv(const TableModel&,TableRange,std::size_t byte_limit=TableLimits::transfer_bytes);
[[nodiscard]] ii::core::Result<TableInputMatrix> parse_table_tsv(std::string_view);
}
