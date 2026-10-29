#include <LVRS/RouteMatcher.h>
#include <LVRS/RoutingDetail.h>
#include <iiSDKCore/Core/Text/Utf8.h>

namespace lvrs {
using ii::core::Result;
Result<std::string> RouteMatcher::normalize_path(std::string_view text){
    if(text.size()>RouteLimits::path_bytes)return ii::core::fail(ii::core::ErrorCode::full,"Route exceeds the UTF-8 path budget");
    if(text.find('\0')!=text.npos||!ii::core::valid_utf8(text))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Routes require canonical UTF-8 without NUL");
    text=routing_detail::trim(text);if(text.empty())return std::string("/");
    while(text.size()>1&&text.back()=='/')text.remove_suffix(1);
    std::string normalized;if(text.front()!='/')normalized='/';normalized+=text;
    if(normalized.size()>RouteLimits::path_bytes)return ii::core::fail(ii::core::ErrorCode::full,"Normalized route exceeds the path budget");return normalized;
}
Result<RouteMatch> RouteMatcher::match(std::string_view path,std::string_view route){
    auto normalized=normalize_path(path);if(!normalized)return std::unexpected(normalized.error());
    auto parts=routing_detail::segments(*normalized);if(!parts)return std::unexpected(parts.error());
    auto pattern=routing_detail::compile(route);if(!pattern)return std::unexpected(pattern.error());
    return routing_detail::match(*parts,*pattern);
}
}
