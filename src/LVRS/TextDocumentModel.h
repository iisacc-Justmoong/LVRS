#pragma once
#include "TextDocumentStorage.h"
#include "TextDocumentEdit.h"
#include <iiGUI/TextBoundary.h>
#include <iiSDKCore/Runtime/Query.h>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>

namespace lvrs {
struct TextDocumentOptions final {
    std::size_t load_chunk_size = 64 * 1024;
    std::size_t maximum_line_bytes = 64 * 1024 * 1024;
    std::uint64_t maximum_file_bytes = std::uint64_t(1) << 40;
    std::size_t maximum_lines = 4000000;
    std::size_t maximum_materialized_bytes = 64 * 1024 * 1024;
    std::size_t decoded_cache_bytes = 16 * 1024 * 1024, decoded_cache_entries = 4096;
    std::size_t undo_entries = 128, undo_bytes = 64 * 1024 * 1024;
};
struct TextDocumentCursor final {
    std::size_t line = 0;
    TextPosition column = 0, position = 0;
    friend bool operator==(const TextDocumentCursor&, const TextDocumentCursor&) = default;
};
struct TextDocumentState final {
    std::uint64_t revision = 1, document_revision = 1;
    std::filesystem::path file_path;
    bool has_file_path = false, dirty = false, loading = false;
    TextPosition character_count = 0;
    std::size_t line_count = 1, file_backed_line_count = 0, memory_line_count = 1;
    TextDocumentCursor cursor;
    std::string last_error;
    std::uint64_t loaded_byte_count = 0, total_byte_count = 0;
    double load_progress = 1;
    std::size_t load_chunk_size = 64 * 1024, undo_depth = 0, redo_depth = 0;
};
struct TextDocumentLine final {
    std::size_t index = 0, number = 1;
    TextPosition length = 0;
    std::string text;
    bool file_backed = false;
};
struct TextDocumentLineInfo final {
    std::size_t index = 0;
    std::uint64_t identity = 0, decoded_bytes = 0;
    TextPosition start = 0, length = 0;
    bool file_backed = false;
};
struct TextDocumentStats final {
    std::uint64_t source_reads = 0, source_bytes = 0, decoded_lines = 0, cache_hits = 0;
    std::uint64_t index_visits = 0, cloned_index_nodes = 0, materializations = 0;
    std::size_t cached_lines = 0, cache_bytes = 0, history_bytes = 0;
};
enum class TextDocumentEventKind { changed, load_progress, loaded, load_failed, load_cancelled, saved, save_failed };
struct TextDocumentChange final {
    TextDocumentEventKind kind = TextDocumentEventKind::changed;
    TextDocumentState state;
    std::filesystem::path path; // Operation path; may differ from a rebound file_path.
    std::size_t first_line = 0, removed_lines = 0, inserted_lines = 0;
    bool content = false, cursor = false, storage = false, reset = false, history = false;
    bool edited = false; // Edits only; false for loading, navigation and saving.
    std::optional<TextDocumentEdit> edit;
};
using TextDocumentPort = ii::runtime::QueryPort<TextDocumentChange>;
enum class TextCursorMove { left, right, up, down, line_start, line_end, document_start, document_end, word_left, word_right };

// Owner-thread model. The supplied Core loop must outlive the model and
// dispatch on that same thread.
// Loading uses bounded owned callbacks; each callback reads at most one chunk.
// Reads are legal during observation; reentrant mutations report busy. Receiver
// errors report an already committed change and must not trigger blind retry.
class TextDocumentModel final {
public:
    [[nodiscard]] static ii::core::Result<std::shared_ptr<TextDocumentModel>> create(
        ii::runtime::EventLoop&, TextDocumentOptions = {}, std::shared_ptr<TextDocumentStorage> = {});
    ~TextDocumentModel();
    TextDocumentModel(const TextDocumentModel&) = delete;
    TextDocumentModel& operator=(const TextDocumentModel&) = delete;
    [[nodiscard]] ii::core::Result<TextDocumentState> state() const;
    [[nodiscard]] ii::core::Result<TextDocumentStats> stats() const;
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const TextDocumentPort>);
    [[nodiscard]] ii::core::Result<bool> set_file_path(std::filesystem::path);
    [[nodiscard]] ii::core::Result<bool> set_load_chunk_size(std::size_t);
    [[nodiscard]] ii::core::Result<std::string> text() const;
    [[nodiscard]] ii::core::Result<std::string> text_range(TextPosition first, TextPosition last) const;
    [[nodiscard]] ii::core::Result<bool> set_text(std::string_view);
    [[nodiscard]] ii::core::Result<TextDocumentLine> line_at(std::size_t) const;
    // Metadata does not decode or read the source. Immutable indices share the
    // model's bounded Unicode cache and remain valid after subsequent edits.
    [[nodiscard]] ii::core::Result<TextDocumentLineInfo> line_info(std::size_t) const;
    [[nodiscard]] ii::core::Result<ii::gui::TextIndex> line_index(std::size_t) const;
    [[nodiscard]] ii::core::Result<std::string> line_text(std::size_t) const;
    [[nodiscard]] ii::core::Result<TextPosition> line_length(std::size_t) const;
    [[nodiscard]] ii::core::Result<TextPosition> position_for_line_column(std::size_t, TextPosition) const;
    [[nodiscard]] ii::core::Result<TextDocumentCursor> line_column_for_position(TextPosition) const;
    [[nodiscard]] ii::core::Result<TextPosition> previous_word_boundary_position(TextPosition) const;
    [[nodiscard]] ii::core::Result<TextPosition> next_word_boundary_position(TextPosition) const;
    [[nodiscard]] ii::core::Result<bool> move_cursor(std::size_t, TextPosition);
    [[nodiscard]] ii::core::Result<bool> set_cursor_position(TextPosition);
    [[nodiscard]] ii::core::Result<bool> move_cursor(TextCursorMove);
    [[nodiscard]] ii::core::Result<bool> insert_text(std::string_view);
    [[nodiscard]] ii::core::Result<bool> insert_newline();
    [[nodiscard]] ii::core::Result<bool> replace_range(TextPosition first, TextPosition last, std::string_view);
    [[nodiscard]] ii::core::Result<bool> remove_previous_character();
    [[nodiscard]] ii::core::Result<bool> remove_next_character();
    [[nodiscard]] ii::core::Result<bool> load_file(std::filesystem::path = {});
    [[nodiscard]] ii::core::Result<bool> reload_file();
    [[nodiscard]] ii::core::Result<bool> cancel_load();
    [[nodiscard]] ii::core::Result<bool> save_file(std::filesystem::path = {});
    [[nodiscard]] ii::core::Result<bool> mark_clean();
    [[nodiscard]] ii::core::Result<bool> clear();
    [[nodiscard]] ii::core::Result<bool> undo();
    [[nodiscard]] ii::core::Result<bool> redo();
    [[nodiscard]] ii::core::Result<bool> clear_history();
    [[nodiscard]] ii::core::Result<bool> close();
private:
    struct Impl;
    explicit TextDocumentModel(std::shared_ptr<Impl>);
    std::shared_ptr<Impl> impl_;
};
} // namespace lvrs
