#pragma once
#include <iiGUI/Image.h>
#include <span>
#include <string>

namespace lvrs {
struct IconSource final { std::string_view name,svg; };
// Authored SVGs become immutable native data at build time. Runtime lookup never
// opens resource files and needs neither QML nor an installed source directory.
class Icons final {
public:
    [[nodiscard]] static std::span<const IconSource> sources();
    [[nodiscard]] static std::string normalize(std::string_view);
    [[nodiscard]] static ii::core::Result<std::string_view> data(std::string_view);
    [[nodiscard]] static ii::core::Result<std::shared_ptr<const ii::gui::RasterImage>> image(std::string_view,std::uint32_t pixels=64);
};
} // namespace lvrs
