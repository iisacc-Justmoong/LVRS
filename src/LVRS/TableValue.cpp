#include "TableValue.h"
#include <iiSDKCore/Core/Text/Number.h>
#include <charconv>
#include <cmath>
#include <type_traits>

namespace lvrs {
TableValidation validate_table_value(std::string_view value, std::string_view type) {
    TableValidation result{true, std::string(value), std::string(value)};
    if (type == "string" || type == "text") return result;
    const auto first = value.find_first_not_of(" \t\r\n"), last = value.find_last_not_of(" \t\r\n");
    const auto text = first == value.npos ? std::string_view{} : value.substr(first, last - first + 1);
    if (text.empty()) { result.accepted = false; return result; }
    if (type == "int" || type == "integer") {
        std::int64_t number = 0;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), number);
        result.accepted = !text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
        if (result.accepted) { result.value = number; result.text = std::to_string(number); }
    } else if (type == "number" || type == "real" || type == "float" || type == "double") {
        const auto number = ii::core::parse_decimal(text);
        result.accepted = number.has_value();
        if (number) { result.value = *number; result.text = std::string(text); }
    } else if (type == "bool" || type == "boolean") {
        result.accepted = text == "true" || text == "false" || text == "1" || text == "0";
        if (result.accepted) {
            const bool boolean = text == "true" || text == "1";
            result.value = boolean;
            result.text = boolean ? "true" : "false";
        }
    } else result.accepted = false;
    return result;
}

std::string table_value_text(const TableValue& value) {
    return std::visit([](const auto& item) -> std::string {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::string>) return item;
        else if constexpr (std::is_same_v<T, bool>) return item ? "true" : "false";
        else if constexpr (std::is_same_v<T, double>) return ii::core::format_decimal(item);
        else {
            char buffer[128];
            const auto result = std::to_chars(buffer, buffer + sizeof(buffer), item);
            return result.ec == std::errc{} ? std::string(buffer, result.ptr) : std::string{};
        }
    }, value);
}
}
