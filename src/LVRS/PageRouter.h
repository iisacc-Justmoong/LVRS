#pragma once
#include "NavigationTarget.h"
#include "RouteResolver.h"
#include <iiGUI/NativeWidgets.h>
#include <functional>

namespace lvrs {
class Navigator;
class ViewModelRegistry;
using PageFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget,const NavigationEntry&)>;
// An already compiled C++ page constructor. No source parsing or evaluation.
class PageComponent final {
public:
    explicit PageComponent(PageFactory factory):factory_(std::move(factory)){}
    [[nodiscard]] explicit operator bool()const noexcept{return bool(factory_);}
    [[nodiscard]] ii::core::Result<void> build(ii::gui::WidgetBuilder&,ii::gui::Widget,const NavigationEntry&)const;
private:
    PageFactory factory_;
};
struct PageRoute final {std::string path;PageComponentRef component;ii::core::Value::Object metadata;};
struct PageRouterOptions final {
    std::vector<PageRoute> routes;
    std::string initial_path="/";
    ii::core::Value initial_params;
    PageComponentRef not_found_component;
    std::int64_t route_resolve_cache_capacity=256;
    std::shared_ptr<Navigator> navigator;
    bool register_as_global_navigator=true;
    // Explicit model domain, shared with the owning backend. No hidden global
    // registry or document storage is created by a page transition.
    std::shared_ptr<ViewModelRegistry> view_models;
};
using PageRouterState=NavigationTargetState;
using PageNavigationChange=NavigationTargetChange;
struct PageNavigationFailure final {std::string path;ii::core::Error error;};
using PageNavigationPort=NavigationTargetPort;
using PageNavigationFailurePort=ii::runtime::QueryPort<PageNavigationFailure>;

// Owner-thread controller. One live page generation, one existing iiGUI surface.
// History survives page retirement; back reconstructs an independent page scope.
class PageRouter final : public NavigationTarget {
public:
    [[nodiscard]] static ii::core::Result<std::shared_ptr<PageRouter>> create(PageRouterOptions);
    ~PageRouter() override;
    PageRouter(const PageRouter&)=delete;
    PageRouter& operator=(const PageRouter&)=delete;
    [[nodiscard]] ii::core::Result<ii::gui::Widget> mount(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,ii::gui::Size={640,400});
    [[nodiscard]] ii::core::Result<bool> go(std::string_view,ii::core::Value={},NavigationOperation=NavigationOperation::push)override;
    [[nodiscard]] ii::core::Result<bool> push(std::string_view path,ii::core::Value params={}){return go(path,std::move(params));}
    [[nodiscard]] ii::core::Result<bool> replace(std::string_view path,ii::core::Value params={}){return go(path,std::move(params),NavigationOperation::replace);}
    [[nodiscard]] ii::core::Result<bool> set_root(std::string_view path,ii::core::Value params={}){return go(path,std::move(params),NavigationOperation::set_root);}
    [[nodiscard]] ii::core::Result<bool> go_to(PageComponentRef,ii::core::Value={},NavigationOperation=NavigationOperation::push)override;
    [[nodiscard]] ii::core::Result<bool> replace_with(PageComponentRef page,ii::core::Value params={}){return go_to(std::move(page),std::move(params),NavigationOperation::replace);}
    [[nodiscard]] ii::core::Result<bool> set_root_component(PageComponentRef page,ii::core::Value params={}){return go_to(std::move(page),std::move(params),NavigationOperation::set_root);}
    [[nodiscard]] ii::core::Result<bool> back()override;
    [[nodiscard]] ii::core::Result<bool> pop(){return back();}
    [[nodiscard]] ii::core::Result<bool> pop_to_root()override;
    [[nodiscard]] ii::core::Result<PageRouterState> state()const override;
    [[nodiscard]] ii::core::Result<NavigationPlacement> placement()const override;
    [[nodiscard]] ii::core::Result<std::vector<NavigationViewEntry>> view_tracking_entries()const override;
    [[nodiscard]] ii::core::Result<std::vector<NavigationEntry>> path()const;
    [[nodiscard]] ii::core::Result<ii::gui::Widget> current_page_item(std::string_view local_id={})const;
    [[nodiscard]] ii::core::Result<RouteResolverState> resolver_state()const;
    [[nodiscard]] ii::core::Result<void> observe(std::weak_ptr<PageNavigationPort>)override;
    [[nodiscard]] ii::core::Result<void> observe_failures(std::weak_ptr<PageNavigationFailurePort>);
private:
    PageRouter();
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
