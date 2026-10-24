#include "TextMarkup.h"
#include "TextDocumentDetail.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <array>
#include <charconv>
#include <deque>
#include <optional>
#include <stdexcept>
#include <vector>

namespace lvrs {
using namespace ii;
namespace {
bool space(char c){return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v';}
void append(std::string& output,std::string_view value){if(value.size()>TextMarkup::maximum_output_bytes-output.size())throw std::length_error("Text markup output exceeds its byte budget");output+=value;}
struct Tag final{std::size_t first=0,last=0;std::string name;bool closing=false,self_closing=false,replaced=false;};
std::optional<Tag> tag(std::string_view value,std::size_t first){
    if(value[first]!='<')return {};std::size_t at=first+1;Tag result{first,0,{}};
    auto whitespace=[&]{while(at<value.size()&&space(value[at]))++at;};whitespace();
    if(at<value.size()&&value[at]=='/'){result.closing=true;++at;whitespace();}
    while(at<value.size()){char c=value[at];if(c>='A'&&c<='Z')c=char(c-'A'+'a');if(c<'a'||c>'z')break;
        if(result.name.size()==6)return {};result.name+=c;++at;}
    if(result.name.empty())return {};whitespace();
    if(at<value.size()&&value[at]=='/'){result.self_closing=true;++at;whitespace();}
    if(at==value.size()||value[at]!='>')return {};result.last=at+1;
    return result;
}
std::string breaks(std::string_view input){
    std::string output;output.reserve(input.size());
    for(std::size_t at=0;at<input.size();){
        if(input[at]=='<'){auto found=tag(input,at);if(found&&((found->name=="br"&&!found->closing)||(found->name=="p"&&!found->self_closing))){output+='\n';at=found->last;continue;}}
        output+=input[at++];
    }
    return output;
}
std::string paired(std::string_view input,std::string_view a,std::string_view b,std::string_view marker){
    std::vector<Tag> tags;std::array<std::deque<std::size_t>,2> openings;
    for(std::size_t at=0;at<input.size();){
        if(input[at]=='<'){auto found=tag(input,at);if(found&&!found->self_closing&&(found->name==a||(!b.empty()&&found->name==b))){
            const auto group=found->name==a?0u:1u;const auto index=tags.size();tags.push_back(*found);
            if(!found->closing)openings[group].push_back(index);
            else if(!openings[group].empty()){tags[openings[group].front()].replaced=true;tags.back().replaced=true;openings[group].pop_front();}
            at=found->last;continue;
        }}++at;
    }
    std::string output;output.reserve(input.size());std::size_t start=0;
    for(const auto& item:tags)if(item.replaced){append(output,input.substr(start,item.first-start));append(output,marker);start=item.last;}
    append(output,input.substr(start));return output;
}
std::string strip_tags(std::string_view input){
    std::string output;output.reserve(input.size());
    for(std::size_t at=0;at<input.size();){
        if(input[at]=='<'){auto end=input.find('>',at+1);if(end==input.npos){append(output,input.substr(at));break;}if(end>at+1){at=end+1;continue;}}
        output+=input[at++];
    }
    return output;
}
std::string normalized(std::string_view value){auto result=breaks(text_detail::newlines(value));result=paired(result,"strong","b","**");result=paired(result,"em","i","*");result=paired(result,"code",{},"`");return strip_tags(result);}
std::string escaped(std::string_view value){
    std::string output;output.reserve(value.size());
    for(char c:value){switch(c){case '&':append(output,"&amp;");break;case '<':append(output,"&lt;");break;case '>':append(output,"&gt;");break;case '"':append(output,"&quot;");break;
        case '\x1e':append(output,"&#30;");break;case '\x1f':append(output,"&#31;");break;default:append(output,std::string_view(&c,1));}}
    return output;
}
std::string protect_code(std::string_view input,std::vector<std::string>& code){
    std::string output;std::size_t at=0;
    while(at<input.size()){
        auto open=input.find('`',at);if(open==input.npos){append(output,input.substr(at));break;}
        auto close=input.find_first_of("`\n",open+1);
        if(close==input.npos||input[close]=='\n'||close==open+1){const auto end=close!=input.npos&&input[close]=='\n'?close+1:open+1;append(output,input.substr(at,end-at));at=end;continue;}
        append(output,input.substr(at,open-at));code.push_back("<code>"+std::string(input.substr(open+1,close-open-1))+"</code>");
        append(output,std::string(1,'\x1f')+std::to_string(code.size()-1)+std::string(1,'\x1e'));at=close+1;
    }
    return output;
}
std::string inline_line(std::string_view input,std::string_view delimiter,std::string_view start,std::string_view finish,bool strong){
    std::string output;std::size_t at=0;
    while(at<input.size()){
        const auto open=input.find(delimiter,at);if(open==input.npos){append(output,input.substr(at));break;}
        const auto first=open+delimiter.size();
        if(first==input.size()||input[first]==delimiter.front()||(!strong&&open&&input[open-1]==delimiter.front())){append(output,input.substr(at,open+1-at));at=open+1;continue;}
        const auto close=input.find(delimiter,first+1);
        if(close==input.npos){append(output,input.substr(at));break;}
        if(!strong&&close+1<input.size()&&input[close+1]==delimiter.front()){append(output,input.substr(at,open+1-at));at=open+1;continue;}
        append(output,input.substr(at,open-at));append(output,start);append(output,input.substr(first,close-first));append(output,finish);at=close+delimiter.size();
    }
    return output;
}
std::string inline_pattern(std::string_view input,std::string_view delimiter,std::string_view start,std::string_view finish,bool strong){
    std::string output;std::size_t at=0;
    while(at<input.size()){const auto newline=input.find('\n',at);const auto end=newline==input.npos?input.size():newline;
        append(output,inline_line(input.substr(at,end-at),delimiter,start,finish,strong));if(newline==input.npos)break;append(output,"\n");at=end+1;}
    return output;
}
template<class F>core::Result<std::string> convert(std::string_view input,F&& operation){
    if(input.size()>TextMarkup::maximum_input_bytes)return core::fail(core::ErrorCode::out_of_range,"Text markup input exceeds its byte budget");
    if(!core::valid_utf8(input))return core::fail(core::ErrorCode::invalid_argument,"Text markup requires UTF-8");
    try{return operation();}catch(const std::length_error& e){return core::fail(core::ErrorCode::out_of_range,e.what());}catch(...){return core::fail(core::ErrorCode::failed,"Text markup allocation failed",std::current_exception());}
}
}
core::Result<std::string> TextMarkup::normalize(std::string_view input){return convert(input,[&]{return normalized(input);});}
core::Result<std::string> TextMarkup::render_plain_text(std::string_view input){return normalize(input);}
core::Result<std::string> TextMarkup::render_html(std::string_view input){
    return convert(input,[&]{
        std::vector<std::string> code;auto value=protect_code(escaped(normalized(input)),code);
        value=inline_pattern(value,"**","<strong>","</strong>",true);value=inline_pattern(value,"__","<strong>","</strong>",true);
        value=inline_pattern(value,"*","<em>","</em>",false);value=inline_pattern(value,"_","<em>","</em>",false);
        std::string output;
        for(std::size_t at=0;at<value.size();){
            if(value[at]=='\x1f'){const auto end=value.find('\x1e',at+1);if(end==value.npos)throw std::logic_error("Invalid internal markup token");std::size_t index=0;const auto parsed=std::from_chars(value.data()+at+1,value.data()+end,index);
                if(parsed.ec!=std::errc{}||parsed.ptr!=value.data()+end||index>=code.size())throw std::logic_error("Invalid internal markup token");
                append(output,code[index]);at=end+1;
            }else if(value[at]=='\n'){append(output,"<br/>");++at;}else{append(output,std::string_view(value.data()+at,1));++at;}
        }
        return output;
    });
}
} // namespace lvrs
