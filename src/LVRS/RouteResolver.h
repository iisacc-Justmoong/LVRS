#pragma once
#include "RouteMatcher.h"
#include <iiSDKCore/Runtime/Query.h>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace lvrs {
struct RouteResolution final {
    bool matched=false;
    std::ptrdiff_t index=-1;
    RouteParams params;
};
struct RouteResolverState final {
    std::uint64_t revision=1,cache_hits=0,cache_misses=0,pattern_matches=0;
    std::size_t patterns=0,cache_capacity=256,cached_entries=0,cache_bytes=0;
};
struct RouteResolverChange final {
    std::uint64_t revision=0;
    bool patterns=false,cache_capacity=false;
};
using RouteResolverChangePort=ii::runtime::QueryPort<RouteResolverChange>;
// Owner-thread resolver. A result remains owned and immutable after eviction,
// configuration changes or destruction. Cache hits share the same result.
class RouteResolver final {
public:
    RouteResolver();
    ~RouteResolver();
    RouteResolver(const RouteResolver&)=delete;
    RouteResolver& operator=(const RouteResolver&)=delete;
    [[nodiscard]] ii::core::Result<bool> set_route_patterns(std::vector<std::string>);
    [[nodiscard]] ii::core::Result<bool> set_cache_capacity(std::int64_t);
    [[nodiscard]] ii::core::Result<std::shared_ptr<const RouteResolution>> resolve(std::string_view);
    [[nodiscard]] ii::core::Result<void> clear_cache();
    [[nodiscard]] ii::core::Result<RouteResolverState> state()const;
    [[nodiscard]] ii::core::Result<std::span<const std::string>> route_patterns()const;
    [[nodiscard]] ii::core::Result<void> observe(std::weak_ptr<RouteResolverChangePort>);
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
