#pragma once
#include <LVRS/SvgSource.h>
#include <iiGUI/Image.h>
#include <iiSDKCore/Runtime/Query.h>
#include <cstdint>

namespace lvrs {
enum class SvgManagerEvent {last_error,vector_first,minimum_scale,maximum_scale,cache_size,revision};
using SvgManagerChangePort=ii::runtime::QueryPort<SvgManagerEvent>;
struct SvgCacheLimits final {
    std::size_t raster_bytes=64*1024*1024;
    std::size_t source_bytes=64*1024*1024;
    std::size_t pending_requests=64;
};
class SvgManager final {
    struct Impl;
public:
    SvgManager();
    explicit SvgManager(ii::runtime::EventLoop&,SvgSourceOptions={});
    explicit SvgManager(std::shared_ptr<SvgSource>,SvgCacheLimits={});
    ~SvgManager();
    SvgManager(const SvgManager&)=delete;
    SvgManager& operator=(const SvgManager&)=delete;
    [[nodiscard]] std::string lastError()const;
    [[nodiscard]] bool vectorFirst()const;
    void setVectorFirst(bool);
    [[nodiscard]] double minimumScale()const;
    void setMinimumScale(double);
    [[nodiscard]] double maximumScale()const;
    void setMaximumScale(double);
    [[nodiscard]] int cacheSize()const;
    void setCacheSize(int);
    [[nodiscard]] std::uint64_t revision()const;
    [[nodiscard]] std::string icon(std::string_view,int logical_size=18,double scale=0);
    [[nodiscard]] double deviceScale()const;
    void clearCache();
    void ensureMinimumScale(double);
    // Native widget input: shares the cached immutable raster instead of
    // decoding an encoded data URL for every view update.
    [[nodiscard]] std::shared_ptr<const ii::gui::RasterImage> image(std::string_view,int logical_size=18,double scale=0);
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const SvgManagerChangePort>);
private:
    std::shared_ptr<Impl> impl_;
};
}
