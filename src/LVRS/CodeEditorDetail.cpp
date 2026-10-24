#include "CodeEditorDetail.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <cmath>

namespace lvrs::code_detail {
bool valid(const CodeEditorOptions& o){
    const auto metric=[](double n,double low=0,double high=8192){return std::isfinite(n)&&n>=low&&n<=high;};
    const auto color=[&](ii::gui::WidgetColor c){return metric(c.red,0,1)&&metric(c.green,0,1)&&metric(c.blue,0,1)&&metric(c.alpha,0,1);};
    if(!metric(o.width,32)||!metric(o.editor_height,16)||!metric(o.field_min_height,16)||!metric(o.header_height)||!metric(o.header_spacing)
        ||!metric(o.inset_horizontal)||!metric(o.inset_vertical)||!metric(o.corner_radius)||!metric(o.font_pixel_size,1,256)
        ||!metric(o.text_line_height,1,1024)||!metric(o.centered_text_height,1,1024)||!metric(o.text_margin)||!metric(o.tab_stop_distance,1)
        ||o.font_weight<400||o.font_weight>700||o.font_weight%100||!metric(o.placeholder_opacity,0,1))return false;
    const auto top=o.inset_vertical+(o.show_snippet_header?o.header_height+o.header_spacing:0);
    if(top+o.inset_vertical+o.text_margin*2>=std::max(o.field_min_height,o.editor_height)||o.inset_horizontal*2+o.text_margin*2+1>=o.width)return false;
    if(o.shape_cache_bytes<4096||o.shape_cache_bytes>512*1024*1024||o.shape_cache_entries<1||o.shape_cache_entries>4096||o.raster_cache_bytes<4096||o.raster_cache_bytes>256*1024*1024||o.selection_history_entries>4096)return false;
    for(auto s:{std::string_view(o.placeholder_text),std::string_view(o.snippet_title),std::string_view(o.snippet_language),std::string_view(o.font_family),std::string_view(o.accessibility_name)})
        if(s.size()>65536||!ii::core::valid_utf8(s)||s.find('\0')!=s.npos)return false;
    if(o.font_family.size()>1024||o.snippet_title.size()+o.snippet_language.size()+3>65536)return false;
    for(auto c:{o.text_color,o.text_color_disabled,o.placeholder_color,o.placeholder_color_disabled,o.selected_text_color,o.header_text_color,o.background_color,o.background_color_hover})if(!color(c))return false;
    for(auto c:{o.selection_color,o.background_color_focused,o.background_color_disabled})if(c&&!color(*c))return false;
    return o.text.size()<=64*1024*1024&&ii::core::valid_utf8(o.text);
}
std::string newlines(std::string_view text){std::string result;result.reserve(text.size());for(std::size_t i=0;i<text.size();++i){auto c=text[i];if(c=='\r'){if(i+1<text.size()&&text[i+1]=='\n')++i;c='\n';}result+=c;}return result;}
ii::gui::Color ink(ii::gui::WidgetColor c){const auto f=[](double v){return float(v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4));};return {f(c.red),f(c.green),f(c.blue),float(c.alpha)};}
std::string slice(const ii::gui::TextIndex& index,std::size_t first,std::size_t last){const auto a=take(index.byte_for_utf16(first)),b=take(index.byte_for_utf16(last));return std::string(index.text().substr(a,b-a));}
TextPosition Projection::length()const{return composition?state.character_count-(composition->last-composition->first)+composition->text.utf16_size():state.character_count;}
std::size_t Projection::lines()const{return composition?state.line_count-(composition->end.line-composition->begin.line)+composition->starts.size()-1:state.line_count;}
ProjectedLine Projection::line(std::size_t row)const{
    if(row>=lines())throw Failure{{ii::core::ErrorCode::out_of_range,"Projected line exceeds the document"}};
    auto source_row=row;
    if(composition){const auto& c=*composition;
        if(row>=c.begin.line&&row<c.begin.line+c.starts.size()){
            const auto part=row-c.begin.line;const auto first=c.starts[part],last=part+1<c.starts.size()?c.starts[part+1]-1:c.text.utf16_size();
            auto text=slice(c.text,first,last);
            if(!part){if(take(model->line_info(c.begin.line)).decoded_bytes>ii::gui::TextLine::maximum_text_bytes)throw Failure{{ii::core::ErrorCode::full,"Composition source paragraph exceeds the native shaping budget"}};auto prefix=take(model->line_index(c.begin.line));text=slice(prefix,0,std::size_t(c.begin.column))+text;}
            if(part+1==c.starts.size()){if(take(model->line_info(c.end.line)).decoded_bytes>ii::gui::TextLine::maximum_text_bytes)throw Failure{{ii::core::ErrorCode::full,"Composition source paragraph exceeds the native shaping budget"}};auto suffix=take(model->line_index(c.end.line));text+=slice(suffix,std::size_t(c.end.column),suffix.utf16_size());}
            const auto start=part?c.first+first:c.first-c.begin.column;
            return {{0,c.generation,part},start,take(ii::gui::TextIndex::create(std::move(text))),c.begin.line};
        }
        if(row>=c.begin.line+c.starts.size())source_row=row-c.starts.size()+(c.end.line-c.begin.line+1);
    }
    auto info=take(model->line_info(source_row));if(info.decoded_bytes>ii::gui::TextLine::maximum_text_bytes)throw Failure{{ii::core::ErrorCode::full,"Paragraph exceeds the 1 MiB native shaping budget"}};auto start=info.start;
    if(composition&&source_row>composition->end.line)start=start-(composition->last-composition->first)+composition->text.utf16_size();
    return {{info.identity,0,0},start,take(model->line_index(source_row)),source_row};
}
TextDocumentCursor Projection::locate(TextPosition position)const{
    if(position>length())throw Failure{{ii::core::ErrorCode::out_of_range,"Projected position exceeds the document"}};
    if(!composition)return take(model->line_column_for_position(position));const auto& c=*composition;
    if(position<c.first)return take(model->line_column_for_position(position));
    if(position<=c.first+c.text.utf16_size()){
        const auto local=std::size_t(position-c.first);take(c.text.byte_for_utf16(local));
        const auto part=std::size_t(std::upper_bound(c.starts.begin(),c.starts.end(),local)-c.starts.begin()-1);
        return {c.begin.line+part,local-c.starts[part]+(!part?c.begin.column:0),position};
    }
    auto original=take(model->line_column_for_position(position-c.text.utf16_size()+(c.last-c.first)));
    if(original.line==c.end.line)original.column=original.column-c.end.column+(c.text.utf16_size()-c.starts.back())+(c.starts.size()==1?c.begin.column:0);
    original.line=original.line-(c.end.line-c.begin.line)+c.starts.size()-1;original.position=position;return original;
}
std::string Projection::range(TextPosition first,TextPosition last)const{
    if(first>last||last>length())throw Failure{{ii::core::ErrorCode::out_of_range,"Projected range exceeds the document"}};
    if(!composition)return take(model->text_range(first,last));const auto& c=*composition;std::string result;
    if(first<c.first){result=take(model->text_range(first,std::min(last,c.first)));first=std::min(last,c.first);}
    const auto marked_end=c.first+c.text.utf16_size();
    if(first<last&&first<marked_end){const auto end=std::min(last,marked_end);result+=slice(c.text,std::size_t(first-c.first),std::size_t(end-c.first));first=end;}
    if(first<last)result+=take(model->text_range(first-c.text.utf16_size()+(c.last-c.first),last-c.text.utf16_size()+(c.last-c.first)));
    return result;
}
ii::gui::TextRange Projection::native_selection(TextPosition anchor,TextPosition cursor)const{
    if(composition)return {std::size_t(composition->first)+composition->selection.location,composition->selection.length};
    return {std::size_t(std::min(anchor,cursor)),std::size_t(std::max(anchor,cursor)-std::min(anchor,cursor))};
}
} // namespace lvrs::code_detail
