#pragma once
#include <iiSDKCore/Core/Result/Result.h>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>

namespace lvrs {
struct RouteLimits final {
    static constexpr std::size_t path_bytes=65536,segments=4096,patterns=65536;
    static constexpr std::size_t pattern_bytes=8*1024*1024,pattern_segments=262144;
    static constexpr std::size_t cache_capacity=65536,cache_bytes=8*1024*1024;
};
using RouteParams=std::map<std::string,std::string,std::less<>>;
struct RouteMatch final {
    bool matched=false;
    RouteParams params;
    friend bool operator==(const RouteMatch&,const RouteMatch&)=default;
};
// Pure path matching, independent of URL parsing, filesystem paths and widgets.
class RouteMatcher final {
public:
    [[nodiscard]] static ii::core::Result<std::string> normalize_path(std::string_view);
    [[nodiscard]] static ii::core::Result<RouteMatch> match(std::string_view path,std::string_view pattern);
};
}
