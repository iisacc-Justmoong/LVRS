#pragma once
#include <LVRS/TextDocumentModel.h>
#include <functional>
#include <string_view>

namespace lvrs::text_detail {
struct Record final {
    std::uint64_t id = 0, offset = 0, bytes = 0, decoded_bytes = 0, fingerprint = 0;
    TextPosition units = 0;
    bool strip_bom = false;
    std::shared_ptr<const TextDocumentSource> source;
    std::shared_ptr<const std::string> memory;
};
struct LineNode;
using Tree = std::shared_ptr<LineNode>;
struct LineNode final {
    Record record;
    Tree left, right;
    std::uint64_t priority = 0, units = 0, bytes = 0, memory_bytes = 0;
    std::size_t count = 1, files = 0;
    explicit LineNode(Record);
};
[[nodiscard]] std::size_t count(const Tree&) noexcept;
[[nodiscard]] TextPosition units(const Tree&) noexcept;
[[nodiscard]] std::uint64_t bytes(const Tree&) noexcept;
[[nodiscard]] std::size_t files(const Tree&) noexcept;
[[nodiscard]] Tree merge(Tree, Tree, TextDocumentStats&);
[[nodiscard]] std::pair<Tree,Tree> split(Tree, std::size_t, TextDocumentStats&);
[[nodiscard]] Tree splice(Tree, std::size_t first, std::size_t removed, Tree inserted, TextDocumentStats&);
[[nodiscard]] Record at(const Tree&, std::size_t, TextDocumentStats&);
[[nodiscard]] TextPosition start(const Tree&, std::size_t, TextDocumentStats&);
[[nodiscard]] TextDocumentCursor locate(const Tree&, TextPosition, TextDocumentStats&);
[[nodiscard]] ii::core::Result<void> visit(const Tree&, const std::function<ii::core::Result<void>(const Record&)>&);
[[nodiscard]] ii::core::Result<bool> all(const Tree&, const std::function<ii::core::Result<bool>(const Record&)>&);
struct Decoded final {std::string text; TextPosition units = 0;};
[[nodiscard]] std::string newlines(std::string_view);
[[nodiscard]] ii::core::Result<Decoded> decode_line(std::string_view, bool first_line);
[[nodiscard]] TextPosition utf16_length(std::string_view) noexcept;
[[nodiscard]] std::uint64_t fingerprint(std::string_view) noexcept;
[[nodiscard]] ii::core::Result<std::filesystem::path> path(const std::filesystem::path&, bool allow_empty = false);
} // namespace lvrs::text_detail
