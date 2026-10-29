#include <LVRS/TextDocumentEdit.h>
#include <algorithm>
#include <limits>

namespace lvrs {
namespace {
bool valid(const TextDocumentEdit& e) {
    return e.first<=UINT64_MAX-e.removed&&e.first<=UINT64_MAX-e.inserted&&unsigned(e.operation)<=unsigned(TextEditOperation::redo);
}
}
ii::core::Result<TextPosition> map_text_position(TextPosition position,const TextDocumentEdit& e,TextPositionAffinity affinity) {
    if(!valid(e)||unsigned(affinity)>unsigned(TextPositionAffinity::after))
        return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid text edit range or insertion affinity");
    const auto end=e.first+e.removed;
    if(position<e.first)return position;
    if(position>end) {
        if(e.inserted>=e.removed) {
            const auto delta=e.inserted-e.removed;
            if(position>UINT64_MAX-delta)return ii::core::fail(ii::core::ErrorCode::out_of_range,"Mapped text position overflows its unit range");
            return position+delta;
        }
        return position-(e.removed-e.inserted);
    }
    if(e.removed&&position==end)return e.first+e.inserted;
    return affinity==TextPositionAffinity::after?e.first+e.inserted:e.first;
}
ii::core::Result<TextDocumentSelection> map_text_selection(TextDocumentSelection s,const TextDocumentEdit& e) {
    if(s.anchor==s.cursor) {
        auto position=map_text_position(s.cursor,e);if(!position)return std::unexpected(position.error());
        return TextDocumentSelection{*position,*position};
    }
    const bool forward=s.anchor<s.cursor;
    auto begin=map_text_position(std::min(s.anchor,s.cursor),e,TextPositionAffinity::after);
    auto end=map_text_position(std::max(s.anchor,s.cursor),e,TextPositionAffinity::before);
    if(!begin)return std::unexpected(begin.error());if(!end)return std::unexpected(end.error());
    if(*begin>*end)return TextDocumentSelection{*begin,*begin};
    return forward?TextDocumentSelection{*begin,*end}:TextDocumentSelection{*end,*begin};
}
} // namespace lvrs
