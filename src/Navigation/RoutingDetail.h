#pragma once
#include <LVRS/RouteMatcher.h>
#include <span>
#include <vector>

namespace lvrs::routing_detail {
struct Token final {enum class Kind {literal,parameter,rest};Kind kind=Kind::literal;std::size_t offset=0,length=0;};
struct Pattern final {std::string path;std::vector<Token> tokens;};
std::string_view trim(std::string_view);
ii::core::Result<Pattern> compile(std::string_view);
ii::core::Result<std::vector<std::string_view>> segments(std::string_view);
RouteMatch match(std::span<const std::string_view>,const Pattern&);
}
