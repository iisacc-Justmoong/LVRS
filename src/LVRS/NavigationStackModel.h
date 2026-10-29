#pragma once
#include "RouteMatcher.h"
#include <iiSDKCore/Core/Value/Value.h>
#include <iiSDKCore/Runtime/Query.h>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace lvrs {
struct NavigationLimits final {
    static constexpr std::size_t depth=65536,entry_bytes=1024*1024,total_bytes=16*1024*1024;
    static constexpr std::size_t value_depth=64,value_nodes=65536,view_id_bytes=RouteLimits::path_bytes;
};
enum class NavigationOperation {push,replace,set_root};
struct NavigationEntry final {
    std::string path;
    ii::core::Value params{ii::core::Value::Object{}};
    // Identity/lifetime of an already compiled C++ component; never source code.
    std::shared_ptr<const void> component;
    std::string view_id;
    bool enabled=true,disabled=false;
    friend bool operator==(const NavigationEntry&,const NavigationEntry&)=default;
};
struct NavigationViewEntry final {
    std::string view_id,path;
    bool enabled=true;
    friend bool operator==(const NavigationViewEntry&,const NavigationViewEntry&)=default;
};
struct NavigationState final {
    std::uint64_t revision=1;
    std::size_t depth=0;
    std::string current_path;
    ii::core::Value current_params{ii::core::Value::Object{}};
};
struct NavigationChange final {
    std::uint64_t revision=0;
    bool path=false,current=false,tracking=false,tracked_ids=false;
};
using NavigationChangePort=ii::runtime::QueryPort<NavigationChange>;
// Presentation history only. No document undo, persistence, source evaluation
// or widget ownership is implied. Borrowed spans expire on the next mutation.
class NavigationStackModel final {
public:
    NavigationStackModel();
    ~NavigationStackModel();
    NavigationStackModel(const NavigationStackModel&)=delete;
    NavigationStackModel& operator=(const NavigationStackModel&)=delete;
    [[nodiscard]] ii::core::Result<NavigationState> state()const;
    [[nodiscard]] ii::core::Result<std::vector<NavigationEntry>> path()const;
    [[nodiscard]] ii::core::Result<std::span<const NavigationEntry>> path_view()const;
    [[nodiscard]] ii::core::Result<bool> set_path(std::vector<NavigationEntry>);
    [[nodiscard]] ii::core::Result<std::string> normalize_path(std::string_view)const;
    [[nodiscard]] ii::core::Result<NavigationEntry> create_path_entry(std::string_view,ii::core::Value={})const;
    [[nodiscard]] ii::core::Result<NavigationEntry> create_component_path_entry(std::shared_ptr<const void>,ii::core::Value={})const;
    [[nodiscard]] ii::core::Result<std::vector<NavigationEntry>> stack_after_path_operation(std::string_view,ii::core::Value={},NavigationOperation=NavigationOperation::push)const;
    [[nodiscard]] ii::core::Result<std::vector<NavigationEntry>> stack_after_component_operation(std::shared_ptr<const void>,ii::core::Value={},NavigationOperation=NavigationOperation::push)const;
    [[nodiscard]] ii::core::Result<std::vector<NavigationEntry>> stack_after_pop()const;
    [[nodiscard]] ii::core::Result<std::vector<NavigationEntry>> stack_after_pop_to_root()const;
    // Preflight a single operation without copying the presentation history.
    [[nodiscard]] ii::core::Result<void> validate_entry_operation(NavigationEntry,NavigationOperation=NavigationOperation::push)const;
    [[nodiscard]] ii::core::Result<bool> apply_entry_operation(NavigationEntry,NavigationOperation=NavigationOperation::push);
    [[nodiscard]] ii::core::Result<void> validate_path_operation(std::string_view,ii::core::Value={},NavigationOperation=NavigationOperation::push)const;
    [[nodiscard]] ii::core::Result<void> validate_component_operation(std::shared_ptr<const void>,ii::core::Value={},NavigationOperation=NavigationOperation::push)const;
    [[nodiscard]] ii::core::Result<bool> apply_path_operation(std::string_view,ii::core::Value={},NavigationOperation=NavigationOperation::push);
    [[nodiscard]] ii::core::Result<bool> apply_component_operation(std::shared_ptr<const void>,ii::core::Value={},NavigationOperation=NavigationOperation::push);
    [[nodiscard]] ii::core::Result<bool> pop();
    [[nodiscard]] ii::core::Result<bool> pop_to_root();
    [[nodiscard]] ii::core::Result<NavigationEntry> current_entry_descriptor()const;
    [[nodiscard]] ii::core::Result<NavigationViewEntry> create_view_tracking_entry(const NavigationEntry&,std::size_t)const;
    [[nodiscard]] ii::core::Result<std::vector<NavigationViewEntry>> build_view_tracking_entries(std::span<const NavigationEntry>)const;
    [[nodiscard]] ii::core::Result<std::vector<NavigationViewEntry>> build_view_tracking_entries()const;
    [[nodiscard]] ii::core::Result<std::vector<NavigationViewEntry>> view_tracking_entries()const;
    [[nodiscard]] ii::core::Result<std::vector<std::string>> tracked_view_ids()const;
    [[nodiscard]] ii::core::Result<std::vector<std::string>> update_tracked_view_ids(std::span<const NavigationViewEntry>);
    [[nodiscard]] ii::core::Result<void> observe(std::weak_ptr<NavigationChangePort>);
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
