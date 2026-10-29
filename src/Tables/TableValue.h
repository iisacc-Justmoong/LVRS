#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <variant>

namespace lvrs {
using TableValue = std::variant<std::string, std::int64_t, double, bool>;
struct TableValidation final {
    bool accepted = true;
    std::string text;
    TableValue value;
};
using TableValidator = std::function<TableValidation(std::string_view, std::string_view)>;
// Explicit strict validator. Cells without a validator accept strings; the
// model additionally normalizes source type aliases and boolean spellings.
[[nodiscard]] TableValidation validate_table_value(std::string_view, std::string_view);
[[nodiscard]] std::string table_value_text(const TableValue&);
}
