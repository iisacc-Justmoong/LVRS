#pragma once
#include "CodeEditor.h"
#include <iiGUI/TextLine.h>
#include <compare>

namespace lvrs::code_detail {
struct Failure final { ii::core::Error error; };
template<class T>T take(ii::core::Result<T> r){if(!r)throw Failure{r.error()};return std::move(*r);}
inline void good(ii::core::Result<void> r){if(!r)throw Failure{r.error()};}
[[nodiscard]] bool valid(const CodeEditorOptions&);
[[nodiscard]] std::string newlines(std::string_view);
[[nodiscard]] ii::gui::Color ink(ii::gui::WidgetColor);
[[nodiscard]] std::string slice(const ii::gui::TextIndex&,std::size_t first,std::size_t last);
struct ShapeKey final {
    std::uint64_t record = 0, composition = 0;
    std::size_t part = 0;
    friend auto operator<=>(const ShapeKey&,const ShapeKey&) = default;
};
struct ProjectedLine final { ShapeKey key; TextPosition start = 0; ii::gui::TextIndex index; std::size_t source_line = 0; };
// The preedit replaces a source range in the view only. It never mutates the
// model until the native text service commits it.
struct Projection final {
    std::shared_ptr<TextDocumentModel> model;
    TextDocumentState state;
    struct Composition final {
        TextPosition first = 0, last = 0;
        TextDocumentCursor begin, end;
        ii::gui::TextIndex text;
        std::vector<std::size_t> starts{0};
        ii::gui::TextRange selection;
        std::uint64_t generation = 0;
    };
    std::optional<Composition> composition;
    [[nodiscard]] TextPosition length() const;
    [[nodiscard]] std::size_t lines() const;
    [[nodiscard]] ProjectedLine line(std::size_t) const;
    [[nodiscard]] TextDocumentCursor locate(TextPosition) const;
    [[nodiscard]] std::string range(TextPosition first,TextPosition last) const;
    [[nodiscard]] ii::gui::TextRange native_selection(TextPosition anchor,TextPosition cursor) const;
};
} // namespace lvrs::code_detail
