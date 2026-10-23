#include "Icons.h"
#include <algorithm>
#include <list>
#include <mutex>
#include <utility>

namespace lvrs {
namespace detail {
std::span<const IconSource> embedded_icons();
std::span<const std::pair<std::string_view,std::string_view>> icon_aliases();
}
namespace {
struct Cache final {
    struct Entry final { std::string name;std::uint32_t size;std::shared_ptr<const ii::gui::RasterImage> image; };
    std::mutex mutex;
    std::list<Entry> entries;
    std::size_t bytes=0;
    auto find(std::string_view name,std::uint32_t size) { return std::ranges::find_if(entries,[&](const auto& e){return e.name==name && e.size==size;}); }
};
Cache& cache() { static Cache value;return value; }
}
std::span<const IconSource> Icons::sources() { return detail::embedded_icons(); }
std::string Icons::normalize(std::string_view raw) {
    constexpr std::string_view space=" \t\r\n\f\v";
    const auto begin=raw.find_first_not_of(space);if(begin==raw.npos) return {};
    raw=raw.substr(begin,raw.find_last_not_of(space)-begin+1);
    constexpr std::string_view prefix="qrc:/qt/qml/LVRS/resources/iconset/";
    if(raw.starts_with(prefix)) raw.remove_prefix(prefix.size());
    std::string result(raw),lower(result);
    auto fold=[](char c){return c>='A' && c<='Z'?char(c-'A'+'a'):c;};
    std::ranges::transform(lower,lower.begin(),fold);
    if(lower.ends_with(".svg")) result.resize(result.size()-4);
    std::erase(result,'/');lower=result;std::ranges::transform(lower,lower.begin(),fold);
    for(const auto& [alias,target]:detail::icon_aliases()) if(alias==lower) return std::string(target);
    return result;
}
ii::core::Result<std::string_view> Icons::data(std::string_view name) {
    const auto key=normalize(name);const auto all=sources();
    auto it=std::ranges::lower_bound(all,key,{},&IconSource::name);
    if(it==all.end() || it->name!=key) return ii::core::fail(ii::core::ErrorCode::not_found,"Unknown embedded LVRS icon: "+key);
    return it->svg;
}
ii::core::Result<std::shared_ptr<const ii::gui::RasterImage>> Icons::image(std::string_view name,std::uint32_t pixels) {
    if(!pixels || pixels>512) return ii::core::fail(ii::core::ErrorCode::invalid_argument,"LVRS icon raster size must be 1..512 pixels");
    const auto key=normalize(name);auto& c=cache();
    { std::lock_guard lock(c.mutex);auto it=c.find(key,pixels);if(it!=c.entries.end()) { auto result=it->image;c.entries.splice(c.entries.begin(),c.entries,it);return result; } }
    auto svg=data(key);if(!svg) return std::unexpected(svg.error());
    auto decoded=ii::gui::svg_image(*svg,pixels,pixels);if(!decoded) return std::unexpected(decoded.error());
    std::lock_guard lock(c.mutex);
    // Another worker may have decoded the same immutable source while unlocked.
    if(auto it=c.find(key,pixels);it!=c.entries.end()) { auto result=it->image;c.entries.splice(c.entries.begin(),c.entries,it);return result; }
    c.bytes+=(*decoded)->rgba().size();c.entries.push_front({key,pixels,*decoded});
    while(c.entries.size()>128 || c.bytes>8*1024*1024) { c.bytes-=c.entries.back().image->rgba().size();c.entries.pop_back(); }
    return *decoded;
}
} // namespace lvrs
