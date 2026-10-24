#pragma once
#include <iiSDKCore/Core/Result/Result.h>
#include <cstdint>

namespace lvrs {
using TextPosition = std::uint64_t; // UTF-16 code units; never split a scalar.
enum class TextPositionAffinity { before, after };
enum class TextEditOperation { replace, undo, redo };
struct TextDocumentEdit final {
    TextPosition first = 0, removed = 0, inserted = 0;
    // Reuses the model's unique clean/history stamp. Undo/redo preserve the
    // original replacement's identity; this is not a View identity.
    std::uint64_t transaction = 0;
    TextEditOperation operation = TextEditOperation::replace;
    bool undoable = false;
};
struct TextDocumentSelection final {
    TextPosition anchor = 0, cursor = 0;
    friend bool operator==(const TextDocumentSelection&,const TextDocumentSelection&) = default;
};
// Unit-only mapping; callers supply scalar-valid source positions/edits.
// A collapsed caret follows an insertion. Selection boundaries exclude text
// inserted exactly at the boundary and preserve direction where possible.
[[nodiscard]] ii::core::Result<TextPosition> map_text_position(TextPosition,const TextDocumentEdit&,TextPositionAffinity = TextPositionAffinity::after);
[[nodiscard]] ii::core::Result<TextDocumentSelection> map_text_selection(TextDocumentSelection,const TextDocumentEdit&);
} // namespace lvrs
