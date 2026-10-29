#pragma once
#include <LVRS/TextDocumentModel.h>
#include <iiGUI/NativeWidgets.h>

namespace lvrs {
struct CodeEditorOptions final {
    std::shared_ptr<TextDocumentModel> model;
    std::string text, placeholder_text, snippet_title = "Snippet", snippet_language;
    std::string font_family; // Menlo on macOS; Monospace elsewhere.
    double width = 180, editor_height = 144, field_min_height = 144;
    double header_height = 22, header_spacing = 4, inset_horizontal = 10, inset_vertical = 8;
    double corner_radius = 8, font_pixel_size = 13, text_line_height = 13, centered_text_height = 16;
    double tab_stop_distance = 80, text_margin = 0;
    unsigned font_weight = 500;
    bool enabled = true, read_only = false, show_snippet_header = true, show_scroll_bar = true;
    bool auto_focus_on_press = true, select_by_mouse = true, select_by_keyboard = true, persistent_selection = true, overwrite_mode = false;
    ii::gui::WidgetColor text_color{1,1,1,.9}, text_color_disabled{1,1,1,.3};
    ii::gui::WidgetColor placeholder_color{1,1,1,.6}, placeholder_color_disabled{1,1,1,.3};
    ii::gui::WidgetColor selected_text_color{1,1,1,.9}, header_text_color{1,1,1,.6};
    ii::gui::WidgetColor background_color{.09516431391239166,.09655113518238068,.09793794900178909,1};
    ii::gui::WidgetColor background_color_hover{.12302529811859131,.12528030574321747,.12753531336784363,1};
    std::optional<ii::gui::WidgetColor> selection_color, background_color_focused, background_color_disabled;
    double placeholder_opacity = 1;
    std::size_t shape_cache_bytes = 96 * 1024 * 1024, shape_cache_entries = 512, raster_cache_bytes = 32 * 1024 * 1024;
    std::size_t selection_history_entries = 128; // 0 disables per-view undo selection metadata.
    std::string accessibility_name = "Code editor";
};
struct CodeEditorEvents final {
    // The change callback avoids whole-document string materialization. These
    // callbacks follow TextDocumentModel's read-only notification boundary.
    std::function<void(const TextDocumentChange&)> document_edited;
    std::function<void(std::string)> text_edited, submitted;
    std::function<void(ii::gui::TextRange)> selection_changed;
};
struct CodeEditorState final {
    std::uint64_t revision = 1, document_revision = 0;
    TextPosition cursor_position = 0, anchor_position = 0, length = 0;
    ii::gui::TextRange selection;
    std::optional<ii::gui::TextRange> marked_range;
    ii::gui::Rect cursor_rectangle;
    bool focused = false, input_method_composing = false, read_only = false, overwrite_mode = false, cursor_visible = false;
    std::size_t line_count = 1, first_visible_line = 0, visible_lines = 0, cached_lines = 0, cache_bytes = 0, raster_bytes = 0;
    std::size_t selection_history_size = 0;
    double content_width = 0, content_height = 0, scroll_x = 0, scroll_y = 0, centered_text_y = 0;
    std::uint64_t shaped_lines = 0, rasterizations = 0;
    std::string last_error;
};
// A native, self-rendered PlainText/NoWrap editor. Document data and history
// belong to the model; selection, composition, scrolling and caches belong to
// this View. Several editors can share a model without sharing view state.
class CodeEditor final {
public:
    CodeEditor() = default;
    [[nodiscard]] static ii::core::Result<CodeEditor> create(ii::gui::WidgetBuilder&, ii::gui::Widget, std::string_view, CodeEditorOptions = {}, CodeEditorEvents = {});
    [[nodiscard]] ii::gui::Widget root() const noexcept { return root_; }
    [[nodiscard]] ii::gui::Widget viewport() const noexcept { return viewport_; }
    [[nodiscard]] ii::gui::Widget body() const noexcept { return body_; }
    [[nodiscard]] ii::core::Result<std::shared_ptr<TextDocumentModel>> model() const;
    [[nodiscard]] ii::core::Result<CodeEditorState> state() const;
    [[nodiscard]] ii::core::Result<CodeEditorOptions> options() const;
    [[nodiscard]] ii::core::Result<void> set_options(CodeEditorOptions) const;
    [[nodiscard]] ii::core::Result<bool> set_text(std::string_view) const;
    [[nodiscard]] ii::core::Result<std::string> text() const;
    [[nodiscard]] ii::core::Result<std::string> selected_text() const;
    [[nodiscard]] ii::core::Result<bool> select(TextPosition anchor, TextPosition cursor) const;
    [[nodiscard]] ii::core::Result<bool> select_all() const;
    [[nodiscard]] ii::core::Result<bool> deselect() const;
    [[nodiscard]] ii::core::Result<bool> set_cursor_position(TextPosition, bool extend = false) const;
    [[nodiscard]] ii::core::Result<bool> move_cursor(TextCursorMove, bool extend = false) const;
    [[nodiscard]] ii::core::Result<bool> insert_text(std::string_view) const;
    [[nodiscard]] ii::core::Result<bool> replace_selection(std::string_view) const;
    [[nodiscard]] ii::core::Result<bool> clear() const;
    [[nodiscard]] ii::core::Result<bool> undo() const;
    [[nodiscard]] ii::core::Result<bool> redo() const;
    [[nodiscard]] ii::core::Result<void> copy() const;
    [[nodiscard]] ii::core::Result<bool> cut() const;
    [[nodiscard]] ii::core::Result<bool> paste() const;
    [[nodiscard]] ii::core::Result<void> submit() const;
    [[nodiscard]] ii::core::Result<void> force_editor_focus() const;
    [[nodiscard]] ii::core::Result<void> set_scroll_position(double x, double y) const;
    [[nodiscard]] ii::core::Result<void> reveal_cursor() const;
    [[nodiscard]] ii::core::Result<bool> commit_composition() const;
    [[nodiscard]] ii::core::Result<bool> cancel_composition() const;
private:
    struct Impl;
    template<class T,class F> [[nodiscard]] ii::core::Result<T> with_state(F&&) const;
    ii::gui::Widget root_,viewport_,body_;
    std::weak_ptr<Impl> state_;
};
} // namespace lvrs
