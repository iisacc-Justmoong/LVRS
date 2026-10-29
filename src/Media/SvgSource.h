#pragma once
#include <iiSDKCore/Core/Result/Result.h>
#include <functional>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace ii::runtime { class EventLoop; }
namespace lvrs {
// A missing payload denotes a remote resource. Its canonical key coalesces
// requests independently of the original spelling used by the raster cache.
struct SvgResource final {
    std::string key;
    std::optional<std::string> payload;
};
using SvgCompletion=std::function<void(ii::core::Result<std::string>)>;
class SvgRequest {
public:
    virtual ~SvgRequest()=default;
    virtual void cancel()noexcept=0;
};
class SvgSource {
public:
    virtual ~SvgSource()=default;
    [[nodiscard]] virtual double device_scale()const=0;
    [[nodiscard]] virtual ii::core::Result<SvgResource> read(std::string_view)=0;
    // Completion is deferred and delivered on the source's owner EventLoop.
    [[nodiscard]] virtual ii::core::Result<std::shared_ptr<SvgRequest>> fetch(std::string_view,SvgCompletion)=0;
};
struct SvgSourceOptions final {
    std::function<double()> device_scale;
    // Application resource namespaces remain owned by the application. LVRS
    // icon resources are embedded and need no source-directory access.
    std::function<ii::core::Result<std::string>(std::string_view)> resource;
    std::size_t maximum_source_bytes=1024*1024;
};
// The borrowed loop outlives this service and all requests. A null loop supports
// local/data/embedded sources and reports remote access as unavailable.
[[nodiscard]] std::shared_ptr<SvgSource> native_svg_source(ii::runtime::EventLoop* =nullptr,SvgSourceOptions={});
[[nodiscard]] bool svg_vector_source(std::string_view);
}
