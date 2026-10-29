#include <LVRS/RoutingDetail.h>
#include <algorithm>

namespace lvrs {
using ii::core::Result;
namespace {
bool space(char32_t c){return (c>=9&&c<=13)||c==32||c==0x85||c==0xa0||c==0x1680||(c>=0x2000&&c<=0x200a)||c==0x2028||c==0x2029||c==0x202f||c==0x205f||c==0x3000;}
char32_t scalar(std::string_view value,std::size_t& i){
    const auto first=static_cast<unsigned char>(value[i++]);if(first<128)return first;
    unsigned remaining=first<0xe0?1:first<0xf0?2:3;char32_t code=first&((1u<<(6-remaining))-1);
    while(remaining--)code=(code<<6)|(static_cast<unsigned char>(value[i++])&0x3f);return code;
}
}
std::string_view routing_detail::trim(std::string_view text){
    std::size_t first=text.size(),last=0;
    for(std::size_t i=0;i<text.size();){const auto begin=i;const auto code=scalar(text,i);if(!space(code)){first=std::min(first,begin);last=i;}}
    return first==text.size()?std::string_view{}:text.substr(first,last-first);
}
Result<std::vector<std::string_view>> routing_detail::segments(std::string_view path){
    std::vector<std::string_view> result;if(path=="/")return result;
    std::size_t start=1;
    for(std::size_t i=1;i<=path.size();++i)if(i==path.size()||path[i]=='/'){
        if(result.size()==RouteLimits::segments)return ii::core::fail(ii::core::ErrorCode::full,"Route has too many segments");
        result.push_back(path.substr(start,i-start));start=i+1;
    }
    return result;
}
Result<routing_detail::Pattern> routing_detail::compile(std::string_view text){
    auto normalized=RouteMatcher::normalize_path(text);if(!normalized)return std::unexpected(normalized.error());
    Pattern pattern;pattern.path=std::move(*normalized);auto parts=segments(pattern.path);if(!parts)return std::unexpected(parts.error());
    pattern.tokens.reserve(parts->size());
    for(const auto segment:*parts){
        Token token{.offset=std::size_t(segment.data()-pattern.path.data()),.length=segment.size()};
        if(segment.starts_with('[')&&segment.ends_with(']')){
            token.kind=Token::Kind::parameter;++token.offset;token.length-=2;
            if(segment.size()>=5&&segment.substr(1).starts_with("...")){token.kind=Token::Kind::rest;token.offset+=3;token.length-=3;}
        }
        pattern.tokens.push_back(token);
    }
    return pattern;
}
RouteMatch routing_detail::match(std::span<const std::string_view> path,const Pattern& pattern){
    RouteMatch result;RouteParams params;std::size_t index=0;
    for(const auto& token:pattern.tokens){const auto value=std::string_view(pattern.path).substr(token.offset,token.length);
        if(token.kind==Token::Kind::rest){std::string rest;const auto start=index;for(;index<path.size();++index){if(index!=start)rest+='/';rest+=path[index];}
            params.insert_or_assign(std::string(value),std::move(rest));return {true,std::move(params)};
        }
        if(index==path.size())return result;
        if(token.kind==Token::Kind::literal){if(path[index]!=value)return result;}
        else params.insert_or_assign(std::string(value),std::string(path[index]));++index;
    }
    if(index!=path.size())return result;return {true,std::move(params)};
}
}
