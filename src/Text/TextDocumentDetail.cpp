#include <LVRS/TextDocumentDetail.h>
#include <iiGUI/TextBoundary.h>
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>

namespace lvrs::text_detail {
using namespace ii;
namespace {
std::uint64_t mixed(std::uint64_t value) {
    value += UINT64_C(0x9e3779b97f4a7c15);
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}
void refresh(LineNode& node) {
    node.count = 1 + count(node.left) + count(node.right);
    node.units = node.record.units + 1 + units(node.left) + units(node.right);
    node.bytes = node.record.decoded_bytes + 1 + bytes(node.left) + bytes(node.right);
    node.files = std::size_t(bool(node.record.source)) + files(node.left) + files(node.right);
    node.memory_bytes = (node.record.memory ? node.record.memory->size() : 0) +
        (node.left ? node.left->memory_bytes : 0) + (node.right ? node.right->memory_bytes : 0);
}
Tree editable(Tree tree, TextDocumentStats& stats) {
    if (tree.use_count() != 1) { tree = std::make_shared<LineNode>(*tree); ++stats.cloned_index_nodes; }
    return tree;
}
}
LineNode::LineNode(Record value) : record(std::move(value)), priority(mixed(record.id)) { refresh(*this); }
std::size_t count(const Tree& t) noexcept { return t ? t->count : 0; }
TextPosition units(const Tree& t) noexcept { return t ? t->units : 0; }
std::uint64_t bytes(const Tree& t) noexcept { return t ? t->bytes : 0; }
std::size_t files(const Tree& t) noexcept { return t ? t->files : 0; }
Tree merge(Tree a, Tree b, TextDocumentStats& stats) {
    if (!a) return b; if (!b) return a;
    if (a->priority >= b->priority) { a = editable(std::move(a), stats); a->right = merge(std::move(a->right), std::move(b), stats); refresh(*a); return a; }
    b = editable(std::move(b), stats); b->left = merge(std::move(a), std::move(b->left), stats); refresh(*b); return b;
}
std::pair<Tree,Tree> split(Tree root, std::size_t first, TextDocumentStats& stats) {
    if (!root) return {};
    root = editable(std::move(root), stats);
    const auto left = count(root->left);
    if (first <= left) { auto parts = split(std::move(root->left), first, stats); root->left = std::move(parts.second); refresh(*root); return {std::move(parts.first), std::move(root)}; }
    auto parts = split(std::move(root->right), first - left - 1, stats); root->right = std::move(parts.first); refresh(*root); return {std::move(root), std::move(parts.second)};
}
Tree splice(Tree root, std::size_t first, std::size_t removed, Tree inserted, TextDocumentStats& stats) {
    auto prefix = split(std::move(root), first, stats);
    auto suffix = split(std::move(prefix.second), removed, stats);
    return merge(merge(std::move(prefix.first), std::move(inserted), stats), std::move(suffix.second), stats);
}
Record at(const Tree& root, std::size_t index, TextDocumentStats& stats) {
    auto node = root.get();
    while (node) { ++stats.index_visits; const auto left = count(node->left); if (index == left) return node->record; if (index < left) node = node->left.get(); else { index -= left + 1; node = node->right.get(); } }
    return {};
}
TextPosition start(const Tree& root, std::size_t index, TextDocumentStats& stats) {
    auto node = root.get(); TextPosition result = 0;
    while (node) { ++stats.index_visits; const auto left = count(node->left); if (index < left) node = node->left.get(); else { result += units(node->left); if (index == left) return result; result += node->record.units + 1; index -= left + 1; node = node->right.get(); } }
    return result;
}
TextDocumentCursor locate(const Tree& root, TextPosition position, TextDocumentStats& stats) {
    position = std::min(position, units(root) ? units(root) - 1 : 0);
    const auto absolute = position;
    std::size_t line = 0; auto node = root.get();
    while (node) {
        ++stats.index_visits; const auto left = units(node->left);
        if (position < left) node = node->left.get();
        else if (position - left <= node->record.units) return {line + count(node->left), position - left, absolute};
        else { position -= left + node->record.units + 1; line += count(node->left) + 1; node = node->right.get(); }
    }
    return {};
}
core::Result<void> visit(const Tree& root, const std::function<core::Result<void>(const Record&)>& callback) {
    if (!root) return {};
    if (auto r = visit(root->left, callback); !r) return r;
    if (auto r = callback(root->record); !r) return r;
    return visit(root->right, callback);
}
core::Result<bool> all(const Tree& root, const std::function<core::Result<bool>(const Record&)>& callback) {
    if(!root)return true;
    auto left=all(root->left,callback);if(!left||!*left)return left;
    auto value=callback(root->record);if(!value||!*value)return value;
    return all(root->right,callback);
}
std::string newlines(std::string_view value) {
    std::string result; result.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) { if (value[i] == '\r') { result += '\n'; if (i + 1 < value.size() && value[i + 1] == '\n') ++i; } else result += value[i]; }
    return result;
}
TextPosition utf16_length(std::string_view value) noexcept {
    TextPosition result = 0;
    for (const unsigned char byte : value) if ((byte & 0xc0) != 0x80) result += byte >= 0xf0 ? 2 : 1;
    return result;
}
std::uint64_t fingerprint(std::string_view value) noexcept {
    std::uint64_t result = UINT64_C(14695981039346656037);
    for (const unsigned char byte : value) result = (result ^ byte) * UINT64_C(1099511628211);
    return result;
}
core::Result<Decoded> decode_line(std::string_view value, bool first_line) {
    if (first_line && value.starts_with("\xef\xbb\xbf")) value.remove_prefix(3);
    if (value.size() > gui::TextIndex::maximum_text_bytes) return core::fail(core::ErrorCode::out_of_range, "Decoded text line exceeds its byte budget");
    if (core::valid_utf8(value)) return Decoded{std::string(value), utf16_length(value)};
    Decoded result; result.text.reserve(value.size());
    for (std::size_t at = 0; at < value.size();) {
        const auto first = static_cast<unsigned char>(value[at]);
        const std::size_t length = first < 128 ? 1 : first >= 0xc2 && first <= 0xdf ? 2 : first >= 0xe0 && first <= 0xef ? 3 : first >= 0xf0 && first <= 0xf4 ? 4 : 0;
        if (length && length <= value.size() - at && core::valid_utf8(value.substr(at, length))) { result.text.append(value.substr(at, length)); result.units += length == 4 ? 2 : 1; at += length; }
        else { result.text += "\xef\xbf\xbd"; ++result.units; ++at; }
        if (result.text.size() > gui::TextIndex::maximum_text_bytes) return core::fail(core::ErrorCode::out_of_range, "Decoded text line exceeds its byte budget");
    }
    return result;
}
core::Result<std::filesystem::path> path(const std::filesystem::path& value, bool allow_empty) {
    if (value.empty()) { if (allow_empty) return value; return core::fail(core::ErrorCode::invalid_argument, "Text document path is empty"); }
    if (value.native().find(std::filesystem::path::value_type{}) != std::filesystem::path::string_type::npos) return core::fail(core::ErrorCode::invalid_argument, "Text document path contains NUL");
    std::error_code error;
    auto result = std::filesystem::absolute(value, error);
    if (error) return core::fail(core::ErrorCode::invalid_argument, "Text document path could not be resolved");
    return result.lexically_normal();
}
} // namespace lvrs::text_detail
