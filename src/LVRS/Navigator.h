#pragma once
#include "NavigationTarget.h"
#include "ViewStateTracker.h"
#include <functional>
#include <thread>

namespace lvrs {
using NavigatorPlacement=NavigationPlacement;
struct NavigatorState final {
    std::uint64_t revision=1;
    std::size_t router_count=0;
    bool has_router=false;
    NavigationTargetState current;
};
struct NavigatorChange final {NavigatorState state;bool routers=false,current=false;};
using NavigatorPort=ii::runtime::QueryPort<NavigatorChange>;
class Navigator;
class NavigatorRegistration final {
public:
    ~NavigatorRegistration();
    NavigatorRegistration(const NavigatorRegistration&)=delete;
    NavigatorRegistration& operator=(const NavigatorRegistration&)=delete;
    [[nodiscard]] ii::core::Result<void> close();
private:
    friend class Navigator;
    NavigatorRegistration()=default;
    std::shared_ptr<const void> live_;
    std::function<ii::core::Result<void>()> close_;
    std::thread::id owner_;
};
// Explicit instances isolate application/navigation domains. current() supplies
// the owner-thread default; no OS window activation or focus change is implied.
class Navigator final {
public:
    explicit Navigator(std::shared_ptr<ViewStateTracker> = {});
    ~Navigator();
    Navigator(const Navigator&)=delete;
    Navigator& operator=(const Navigator&)=delete;
    [[nodiscard]] static std::shared_ptr<Navigator> current();
    [[nodiscard]] ii::core::Result<bool> register_router(std::shared_ptr<NavigationTarget>,NavigatorPlacement={});
    // Assign the lease before notifications. A receiver error after commit does
    // not discard ownership of a successfully registered target.
    [[nodiscard]] ii::core::Result<void> track_router(std::shared_ptr<NavigationTarget>,std::shared_ptr<NavigatorRegistration>& lease,NavigatorPlacement={});
    [[nodiscard]] ii::core::Result<void> unregister_router(std::shared_ptr<NavigationTarget> = {});
    [[nodiscard]] ii::core::Result<NavigatorState> state();
    [[nodiscard]] ii::core::Result<std::shared_ptr<NavigationTarget>> router();
    [[nodiscard]] ii::core::Result<std::vector<std::shared_ptr<NavigationTarget>>> router_stack();
    [[nodiscard]] ii::core::Result<bool> can_navigate();
    [[nodiscard]] ii::core::Result<std::shared_ptr<ViewStateTracker>> view_state_tracker();
    // bool reports dispatch admission, matching the original Navigator. The
    // target may accept a no-op (for example back at depth one).
    [[nodiscard]] ii::core::Result<bool> go(std::string_view,ii::core::Value={},NavigationOperation=NavigationOperation::push);
    [[nodiscard]] ii::core::Result<bool> replace(std::string_view path,ii::core::Value params={}){return go(path,std::move(params),NavigationOperation::replace);}
    [[nodiscard]] ii::core::Result<bool> set_root(std::string_view path,ii::core::Value params={}){return go(path,std::move(params),NavigationOperation::set_root);}
    [[nodiscard]] ii::core::Result<bool> go_to(PageComponentRef,ii::core::Value={},NavigationOperation=NavigationOperation::push);
    [[nodiscard]] ii::core::Result<bool> replace_with(PageComponentRef page,ii::core::Value params={}){return go_to(std::move(page),std::move(params),NavigationOperation::replace);}
    [[nodiscard]] ii::core::Result<bool> set_root_component(PageComponentRef page,ii::core::Value params={}){return go_to(std::move(page),std::move(params),NavigationOperation::set_root);}
    [[nodiscard]] ii::core::Result<bool> back();
    [[nodiscard]] ii::core::Result<bool> pop_to_root();
    [[nodiscard]] ii::core::Result<void> observe(std::weak_ptr<NavigatorPort>);
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
