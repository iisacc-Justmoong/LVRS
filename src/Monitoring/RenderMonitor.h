#pragma once
#include <iiGUI/Presentation.h>
#include <iiSDKCore/Core/Value/Value.h>
#include <iiSDKCore/Runtime/Query.h>
#include <functional>

namespace ii::gui { class Host; }
namespace lvrs {
enum class RenderMonitorEvent { active, stats, dropped_threshold, sample_capacity, last_error };
using RenderMonitorChangePort = ii::runtime::QueryPort<RenderMonitorEvent>;
class RenderMonitor final {
public:
    RenderMonitor();
    explicit RenderMonitor(std::shared_ptr<ii::gui::PresentationSource>);
    explicit RenderMonitor(std::function<std::int64_t()> epoch_milliseconds);
    ~RenderMonitor();
    RenderMonitor(const RenderMonitor&) = delete;
    RenderMonitor& operator=(const RenderMonitor&) = delete;
    void attachWindow(std::shared_ptr<ii::gui::PresentationSource>);
    void attachWindow(ii::gui::Host&);
    void start();
    void stop();
    void reset();
    [[nodiscard]] ii::core::Value::Object performanceSnapshot() const;
    [[nodiscard]] bool active() const;
    [[nodiscard]] double fps() const;
    [[nodiscard]] double lastFrameMs() const;
    [[nodiscard]] double avgFrameMs() const;
    [[nodiscard]] double p95FrameMs() const;
    [[nodiscard]] double p99FrameMs() const;
    [[nodiscard]] std::uint64_t droppedFrameCount() const;
    [[nodiscard]] double droppedFrameThresholdMs() const;
    void setDroppedFrameThresholdMs(double);
    [[nodiscard]] int frameSampleCapacity() const;
    void setFrameSampleCapacity(int);
    [[nodiscard]] int recentSampleCount() const;
    [[nodiscard]] std::uint64_t frameCount() const;
    [[nodiscard]] std::string lastError() const;
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const RenderMonitorChangePort>);
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
