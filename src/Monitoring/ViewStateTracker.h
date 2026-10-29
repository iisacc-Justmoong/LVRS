#pragma once
#include <LVRS/NavigationStackModel.h>
#include <optional>

namespace lvrs {
enum class ViewState {active=0,inactive=1,disabled=2};
[[nodiscard]] std::string_view view_state_name(ViewState)noexcept;
struct ViewStackInput final {
    std::string view_id,path;
    bool enabled=true,disabled=false;
};
struct ViewRecord final {
    std::string view_id,path;
    std::size_t index=0;
    bool base_enabled=true;
    ViewState state=ViewState::inactive;
    friend bool operator==(const ViewRecord&,const ViewRecord&)=default;
};
struct ViewStateSummary final {
    std::uint64_t revision=1;
    std::size_t loaded_count=0,active_count=0,inactive_count=0,disabled_count=0;
    std::string current_active_view;
};
struct ViewStateSnapshot final {
    ViewStateSummary state;
    std::vector<ViewRecord> stack;
    std::vector<std::string> loaded_views,active_views,inactive_views,disabled_views;
};
using ViewStatePort=ii::runtime::QueryPort<ViewStateSummary>;
// Tracks the supplied logical stack. A record does not own or prove the
// existence of a live widget/page generation. Borrowed spans expire on mutation.
class ViewStateTracker final {
public:
    ViewStateTracker();
    ~ViewStateTracker();
    ViewStateTracker(const ViewStateTracker&)=delete;
    ViewStateTracker& operator=(const ViewStateTracker&)=delete;
    [[nodiscard]] static std::shared_ptr<ViewStateTracker> current();
    [[nodiscard]] ii::core::Result<bool> sync_stack(std::span<const ViewStackInput>);
    [[nodiscard]] ii::core::Result<bool> sync_navigation(std::span<const NavigationViewEntry>);
    [[nodiscard]] ii::core::Result<bool> sync_value(const ii::core::Value&);
    [[nodiscard]] ii::core::Result<bool> set_view_disabled(std::string_view,bool);
    [[nodiscard]] ii::core::Result<bool> set_view_enabled(std::string_view id,bool enabled){return set_view_disabled(id,!enabled);}
    [[nodiscard]] ii::core::Result<ViewStateSummary> state()const;
    [[nodiscard]] ii::core::Result<std::vector<ViewRecord>> stack()const;
    [[nodiscard]] ii::core::Result<std::span<const ViewRecord>> stack_view()const;
    [[nodiscard]] ii::core::Result<std::vector<std::string>> loaded_views()const;
    [[nodiscard]] ii::core::Result<std::vector<std::string>> active_views()const;
    [[nodiscard]] ii::core::Result<std::vector<std::string>> inactive_views()const;
    [[nodiscard]] ii::core::Result<std::vector<std::string>> disabled_views()const;
    [[nodiscard]] ii::core::Result<std::optional<ViewRecord>> view(std::string_view)const;
    [[nodiscard]] ii::core::Result<bool> is_loaded(std::string_view)const;
    [[nodiscard]] ii::core::Result<std::string> state_of(std::string_view)const;
    [[nodiscard]] ii::core::Result<ViewStateSnapshot> snapshot()const;
    [[nodiscard]] ii::core::Result<ii::core::Value> snapshot_value()const;
    [[nodiscard]] ii::core::Result<bool> clear();
    [[nodiscard]] ii::core::Result<void> observe(std::weak_ptr<ViewStatePort>);
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
