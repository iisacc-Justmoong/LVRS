#pragma once
#include <LVRS/NavigationStackModel.h>

namespace lvrs {
struct PageMonitorState final {
    std::uint64_t revision=1;
    std::size_t count=0;
    std::string current;
    bool can_undo=false;
};
using PageMonitorPort=ii::runtime::QueryPort<PageMonitorState>;
// A visit log, independent of page lifetime and NavigationStackModel. Recording
// and undo never navigate or change application documents.
class PageMonitor final {
public:
    PageMonitor();
    ~PageMonitor();
    PageMonitor(const PageMonitor&)=delete;
    PageMonitor& operator=(const PageMonitor&)=delete;
    [[nodiscard]] static std::shared_ptr<PageMonitor> current();
    [[nodiscard]] ii::core::Result<PageMonitorState> state()const;
    [[nodiscard]] ii::core::Result<std::vector<std::string>> history()const;
    [[nodiscard]] ii::core::Result<std::span<const std::string>> history_view()const;
    [[nodiscard]] ii::core::Result<bool> record(std::string_view);
    [[nodiscard]] ii::core::Result<std::string> undo();
    [[nodiscard]] ii::core::Result<bool> clear();
    [[nodiscard]] ii::core::Result<void> observe(std::weak_ptr<PageMonitorPort>);
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
