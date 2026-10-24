#pragma once
#include <iiSDKCore/Core/Result/Result.h>
#include <cstddef>
#include <string>
#include <string_view>

namespace lvrs {
// LVRS inline markup data conversion, not an HTML browser or LVUI interpreter.
class TextMarkup final {
public:
    static constexpr std::size_t maximum_input_bytes = 8 * 1024 * 1024;
    static constexpr std::size_t maximum_output_bytes = 64 * 1024 * 1024;
    [[nodiscard]] static ii::core::Result<std::string> normalize(std::string_view);
    [[nodiscard]] static ii::core::Result<std::string> render_html(std::string_view);
    // Legacy contract: normalize HTML/newlines but retain markdown markers.
    [[nodiscard]] static ii::core::Result<std::string> render_plain_text(std::string_view);
};
} // namespace lvrs
