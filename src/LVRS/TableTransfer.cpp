#include "TableTransfer.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <charconv>
#include <cmath>

namespace lvrs {
using ii::core::ErrorCode;
namespace {
std::string_view trimmed(std::string_view text){
    const auto first=text.find_first_not_of(" \t\r\n");
    return first==text.npos?std::string_view{}:text.substr(first,text.find_last_not_of(" \t\r\n")-first+1);
}
bool text_valid(std::string_view text){return text.find('\0')==text.npos&&ii::core::valid_utf8(text);}
}
ii::core::Result<std::string> table_column_name(std::size_t column){
    if(column>=TableLimits::columns)return ii::core::fail(ErrorCode::out_of_range,"Table column address exceeds the model domain");
    std::string result;
    do{result.push_back(char('A'+column%26));if(column<26)break;column=column/26-1;}while(true);
    std::reverse(result.begin(),result.end());return result;
}
ii::core::Result<std::size_t> table_column_index(std::string_view text){
    text=trimmed(text);if(text.empty())return ii::core::fail(ErrorCode::invalid_argument,"Empty table column address");
    std::size_t result=0;
    for(auto letter:text){
        if(letter>='a'&&letter<='z')letter=char(letter-'a'+'A');
        if(letter<'A'||letter>'Z')return ii::core::fail(ErrorCode::invalid_argument,"Table column address requires ASCII letters");
        const auto digit=std::size_t(letter-'A'+1);
        if(result>(TableLimits::columns-digit)/26)return ii::core::fail(ErrorCode::out_of_range,"Table column address exceeds the model domain");
        result=result*26+digit;
    }
    return result-1;
}
ii::core::Result<std::string> table_cell_reference(TablePosition position){
    if(position.row>=TableLimits::rows)return ii::core::fail(ErrorCode::out_of_range,"Table row address exceeds the model domain");
    auto column=table_column_name(position.column);if(!column)return std::unexpected(column.error());
    return std::move(*column)+std::to_string(position.row+1);
}
ii::core::Result<TablePosition> table_cell_coordinates(std::string_view text){
    text=trimmed(text);std::size_t boundary=0;
    while(boundary<text.size()&&((text[boundary]>='A'&&text[boundary]<='Z')||(text[boundary]>='a'&&text[boundary]<='z')))++boundary;
    const auto row_text=text.substr(boundary);
    if(!boundary||row_text.empty()||row_text.front()<'1'||row_text.front()>'9')return ii::core::fail(ErrorCode::invalid_argument,"Invalid A1 table address");
    auto column=table_column_index(text.substr(0,boundary));if(!column)return std::unexpected(column.error());
    std::size_t row=0;const auto parsed=std::from_chars(row_text.data(),row_text.data()+row_text.size(),row);
    if(parsed.ec!=std::errc{}||parsed.ptr!=row_text.data()+row_text.size()||!row||row>TableLimits::rows)return ii::core::fail(ErrorCode::out_of_range,"Invalid table row address");
    return TablePosition{row-1,*column};
}
namespace {
struct TsvWriter final {
    std::string output;
    std::size_t limit=TableLimits::transfer_bytes;
    bool append(char c){if(output.size()==limit)return false;output.push_back(c);return true;}
    ii::core::Result<void> field(const std::optional<TableValue>& value){
        std::string formatted;std::string_view text;
        if(value){
            if(const auto* number=std::get_if<double>(&*value);number&&!std::isfinite(*number))return ii::core::fail(ErrorCode::invalid_argument,"Nonfinite TSV value");
            if(const auto* string=std::get_if<std::string>(&*value))text=*string;else{formatted=table_value_text(*value);text=formatted;}
        }
        if(text.size()>TableLimits::cell_bytes||!text_valid(text))return ii::core::fail(ErrorCode::invalid_argument,"Invalid TSV cell text");
        const bool quote=text.find_first_of("\t\r\n\"")!=text.npos;
        if(quote&&!append('"'))return ii::core::fail(ErrorCode::full,"TSV exceeds the transfer budget");
        for(auto c:text)if(!append(c)||(quote&&c=='"'&&!append('"')))return ii::core::fail(ErrorCode::full,"TSV exceeds the transfer budget");
        if(quote&&!append('"'))return ii::core::fail(ErrorCode::full,"TSV exceeds the transfer budget");return {};
    }
};
}
ii::core::Result<std::string> table_values_as_tsv(const TableMatrix& values){
    if(values.size()>TableLimits::rows)return ii::core::fail(ErrorCode::full,"Too many TSV rows");
    TsvWriter writer;std::size_t cells=0;
    for(std::size_t r=0;r<values.size();++r){
        const auto& row=values[r];
        if(row.size()>TableLimits::columns||row.size()>TableLimits::transfer_cells-cells)return ii::core::fail(ErrorCode::full,"Too many TSV cells");
        cells+=row.size();if(r&&!writer.append('\n'))return ii::core::fail(ErrorCode::full,"TSV exceeds the transfer budget");
        for(std::size_t c=0;c<row.size();++c){
            if(c&&!writer.append('\t'))return ii::core::fail(ErrorCode::full,"TSV exceeds the transfer budget");
            if(auto result=writer.field(row[c]);!result)return std::unexpected(result.error());
        }
    }
    return std::move(writer.output);
}
ii::core::Result<std::string> table_range_as_tsv(const TableModel& model,TableRange range,std::size_t byte_limit){
    if(auto state=model.state();!state)return std::unexpected(state.error());
    if(!range.row_count||!range.column_count||range.row>=model.row_count()||range.column>=model.column_count()||range.row_count>model.row_count()-range.row||range.column_count>model.column_count()-range.column)return ii::core::fail(ErrorCode::out_of_range,"Invalid TSV source range");
    if(range.row_count>TableLimits::transfer_cells/range.column_count)return ii::core::fail(ErrorCode::full,"Too many TSV cells");
    if(!byte_limit||byte_limit>TableLimits::transfer_bytes)return ii::core::fail(ErrorCode::invalid_argument,"Invalid TSV output budget");
    for(std::size_t r=0;r<range.row_count;++r)if(range.column+range.column_count>model.options().rows[range.row+r].cells.size())return ii::core::fail(ErrorCode::out_of_range,"Table range includes a missing cell");
    TsvWriter writer{.limit=byte_limit};
    for(std::size_t r=0;r<range.row_count;++r){
        if(r&&!writer.append('\n'))return ii::core::fail(ErrorCode::full,"TSV exceeds the transfer budget");
        for(std::size_t c=0;c<range.column_count;++c){
            if(c&&!writer.append('\t'))return ii::core::fail(ErrorCode::full,"TSV exceeds the transfer budget");
            auto value=model.cell_raw_value({range.row+r,range.column+c});
            if(!value)return std::unexpected(value.error());
            if(auto result=writer.field(*value);!result)return std::unexpected(result.error());
        }
    }
    return std::move(writer.output);
}
ii::core::Result<TableInputMatrix> parse_table_tsv(std::string_view source){
    if(source.size()>TableLimits::transfer_bytes)return ii::core::fail(ErrorCode::full,"TSV exceeds the transfer budget");
    if(!text_valid(source))return ii::core::fail(ErrorCode::invalid_argument,"TSV must be valid UTF-8 without NUL");
    TableInputMatrix rows;std::vector<TableValue> row;std::string cell;bool quoted=false;std::size_t count=0;
    const auto finish_cell=[&]{if(row.size()==TableLimits::columns||count==TableLimits::transfer_cells)return false;row.emplace_back(std::move(cell));cell.clear();++count;return true;};
    const auto finish_row=[&]{if(rows.size()==TableLimits::rows)return false;rows.push_back(std::move(row));row.clear();return true;};
    for(std::size_t index=0;index<source.size();++index){
        const auto c=source[index];
        if(quoted){if(c=='"'){if(index+1<source.size()&&source[index+1]=='"'){cell+='"';++index;}else quoted=false;}else cell+=c;}
        else if(c=='"'&&cell.empty())quoted=true;
        else if(c=='\t'){if(!finish_cell())return ii::core::fail(ErrorCode::full,"Too many TSV cells");}
        else if(c=='\n'||c=='\r'){
            if(c=='\r'&&index+1<source.size()&&source[index+1]=='\n')++index;
            if(!finish_cell()||!finish_row())return ii::core::fail(ErrorCode::full,"Too many TSV rows/cells");
        }else cell+=c;
        if(cell.size()>TableLimits::cell_bytes)return ii::core::fail(ErrorCode::full,"TSV cell exceeds the text budget");
    }
    if(row.empty()&&cell.empty()&&!rows.empty())return rows;
    if(!finish_cell()||!finish_row())return ii::core::fail(ErrorCode::full,"Too many TSV rows/cells");
    if(rows.size()>1&&rows.back().size()==1&&std::get<std::string>(rows.back().front()).empty())rows.pop_back();
    return rows;
}
}
