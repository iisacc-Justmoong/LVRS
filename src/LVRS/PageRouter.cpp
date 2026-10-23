#include "PageRouter.h"
#include "Navigator.h"
#include <algorithm>
#include <optional>
#include <thread>

namespace lvrs {
using namespace ii;
namespace {
struct Guard final {bool& flag;explicit Guard(bool& value):flag(value){flag=true;}~Guard(){flag=false;}};
template<class Port>core::Result<void> observe_port(std::vector<std::weak_ptr<Port>>& ports,std::weak_ptr<Port> receiver){
    auto incoming=receiver.lock();if(!incoming)return core::fail(core::ErrorCode::invalid_argument,"Page router receiver already expired");
    std::erase_if(ports,[](const auto& port){return port.expired();});
    for(const auto& port:ports)if(port.lock()==incoming)return {};ports.push_back(std::move(receiver));return {};
}
template<class Port,class Event>core::Result<void> notify_ports(std::vector<std::weak_ptr<Port>>& ports,const Event& event){
    std::vector<std::shared_ptr<Port>> live;std::erase_if(ports,[](const auto& port){return port.expired();});
    for(const auto& port:ports)if(auto locked=port.lock())live.push_back(std::move(locked));
    std::optional<core::Error> failed;
    for(const auto& port:live){auto reply=port->send(event);if(!reply&&reply.error().code!=runtime::QueryErrorCode::receiver_expired&&!failed)failed=core::Error{core::ErrorCode::failed,"Page router receiver failed after navigation was committed",reply.error().cause};}
    if(failed)return std::unexpected(*failed);return {};
}
}
struct PageRouter::Impl final {
    struct Target final {NavigationEntry entry;PageComponentRef component;};
    std::thread::id owner=std::this_thread::get_id();
    PageRouterOptions options;
    std::weak_ptr<PageRouter> self;
    NavigationStackModel history;
    RouteResolver resolver;
    gui::Widget host;
    PageComponentRef active_component;
    bool mounted=false,changing=false,notifying=false;
    std::vector<std::weak_ptr<PageNavigationPort>> receivers;
    std::vector<std::weak_ptr<PageNavigationFailurePort>> failures;
    core::Result<void> ready(bool mutate=false,bool require_mount=false)const{
        if(owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Page router requires its owner thread");
        if((changing&&!notifying)||(mutate&&(changing||notifying)))return core::fail(core::ErrorCode::busy,"Page router is constructing or notifying another navigation");
        if(require_mount&&!mounted)return core::fail(core::ErrorCode::closed,"Page router is not mounted");
        if(mounted){auto snapshot=host.snapshot();if(!snapshot)return std::unexpected(snapshot.error());}
        return {};
    }
    core::Result<Target> resolve(std::string_view path,core::Value params){
        auto normalized=RouteMatcher::normalize_path(path);if(!normalized)return std::unexpected(normalized.error());
        auto supplied=history.create_path_entry(*normalized,std::move(params));if(!supplied)return std::unexpected(supplied.error());
        auto explicit_params=supplied->params.get<core::Value::Object>();if(!explicit_params)return core::fail(core::ErrorCode::invalid_argument,"Route parameters require a typed object");
        auto resolution=resolver.resolve(*normalized);if(!resolution)return std::unexpected(resolution.error());
        core::Value::Object merged;
        for(const auto& [key,value]:(*resolution)->params)merged.emplace(key,core::Value{value});
        for(const auto& [key,value]:explicit_params->get())merged.insert_or_assign(key,value);
        auto component=(*resolution)->matched?options.routes[std::size_t((*resolution)->index)].component:options.not_found_component;
        if(!component||!*component)return core::fail(core::ErrorCode::not_found,"No compiled page matches this route");
        return Target{{.path=std::move(*normalized),.params=core::Value{std::move(merged)}},std::move(component)};
    }
    core::Result<bool> reject(std::string path,core::Error error){
        Guard guard{notifying};(void)notify_ports(failures,PageNavigationFailure{std::move(path),error});return std::unexpected(std::move(error));
    }
    core::Result<bool> publish(Target next,std::function<core::Result<bool>()> commit){
        // Keep both component constructors alive across old-scope destruction.
        auto previous=active_component;bool changed=false;
        auto built=host.set_content([&](gui::WidgetBuilder& widgets){return next.component->build(widgets,{},next.entry);},[&]()->core::Result<void>{auto result=commit();if(!result)return std::unexpected(result.error());changed=*result;return {};});
        if(!built)return reject(next.entry.path,built.error());
        active_component=std::move(next.component);
        auto current=history.state();if(!current)return std::unexpected(current.error());
        auto snapshot=host.snapshot();if(!snapshot)return std::unexpected(snapshot.error());
        Guard guard{notifying};auto sent=notify_ports(receivers,PageNavigationChange{std::move(*current),active_component,snapshot->content_generation});
        if(!sent)return std::unexpected(sent.error());return changed;
    }
    core::Result<bool> navigate(Target next,NavigationOperation operation){
        const auto& entry=next.entry;
        auto preflight=entry.component?history.validate_component_operation(entry.component,entry.params,operation):history.validate_path_operation(entry.path,entry.params,operation);
        if(!preflight)return reject(entry.path,preflight.error());
        auto path=history.path_view();if(!path)return std::unexpected(path.error());
        if(!path->empty()&&(operation==NavigationOperation::replace||(operation==NavigationOperation::set_root&&path->size()==1))&&path->back()==entry)return false;
        // Only the single candidate is copied. Push never clones the old stack.
        const auto descriptor=next.entry;
        return publish(std::move(next),[this,descriptor,operation]{return descriptor.component?history.apply_component_operation(descriptor.component,descriptor.params,operation):history.apply_path_operation(descriptor.path,descriptor.params,operation);});
    }
    core::Result<bool> remove(bool root){
        auto path=history.path_view();if(!path)return std::unexpected(path.error());if(path->size()<=1)return false;
        const auto entry=(*path)[root?0:path->size()-2];
        core::Result<Target> next=entry.component?core::Result<Target>{Target{entry,std::static_pointer_cast<const PageComponent>(entry.component)}}:resolve(entry.path,entry.params);
        if(!next)return reject(entry.path,next.error());
        return publish(std::move(*next),[this,root]{return root?history.pop_to_root():history.pop();});
    }
};
core::Result<void> PageComponent::build(gui::WidgetBuilder& widgets,gui::Widget parent,const NavigationEntry& context)const{
    if(!factory_)return core::fail(core::ErrorCode::invalid_argument,"Compiled page has no constructor");return factory_(widgets,parent,context);
}
PageRouter::PageRouter():impl_(std::make_shared<Impl>()){}
PageRouter::~PageRouter()=default;
core::Result<std::shared_ptr<PageRouter>> PageRouter::create(PageRouterOptions options){
    auto result=std::shared_ptr<PageRouter>(new PageRouter);auto p=result->impl_;
    p->self=result;
    if(options.routes.size()>RouteLimits::patterns)return core::fail(core::ErrorCode::full,"Too many page route declarations");
    std::vector<std::string> patterns;patterns.reserve(options.routes.size());
    for(const auto& route:options.routes){if(!route.component||!*route.component)return core::fail(core::ErrorCode::invalid_argument,"Every route requires a compiled page constructor");patterns.push_back(route.path);}
    if(options.not_found_component&&!*options.not_found_component)return core::fail(core::ErrorCode::invalid_argument,"Fallback page requires a constructor");
    if(auto r=p->resolver.set_route_patterns(std::move(patterns));!r)return std::unexpected(r.error());
    if(auto r=p->resolver.set_cache_capacity(options.route_resolve_cache_capacity);!r)return std::unexpected(r.error());
    auto initial=RouteMatcher::normalize_path(options.initial_path);if(!initial)return std::unexpected(initial.error());options.initial_path=std::move(*initial);p->options=std::move(options);return result;
}
core::Result<gui::Widget> PageRouter::mount(gui::WidgetBuilder& widgets,gui::Widget parent,std::string_view id,gui::Size size){
    auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->mounted)return core::fail(core::ErrorCode::already_exists,"Page router is already mounted");Guard guard{p->changing};
    auto target=p->resolve(p->options.initial_path,p->options.initial_params);if(!target)return std::unexpected(target.error());
    auto host=widgets.content_host(parent,id,size);if(!host)return std::unexpected(host.error());auto owner=p->self.lock();if(!owner)return core::fail(core::ErrorCode::closed,"Page router owner retired");if(auto retained=widgets.retain(owner);!retained)return std::unexpected(retained.error());
    p->host=*host;p->mounted=true;
    if(p->options.register_as_global_navigator){
        if(!p->options.navigator)p->options.navigator=Navigator::current();
        std::shared_ptr<NavigatorRegistration> lease;core::Result<void> registered;
        {Guard exposed{p->notifying};registered=p->options.navigator->track_router(owner,lease);}
        if(lease)if(auto retained=widgets.retain(lease);!retained)return std::unexpected(retained.error());
        if(!registered)return std::unexpected(registered.error());
    }
    auto result=p->navigate(std::move(*target),NavigationOperation::set_root);if(!result)return std::unexpected(result.error());return *host;
}
core::Result<bool> PageRouter::go(std::string_view path,core::Value params,NavigationOperation operation){
    auto p=impl_;if(auto r=p->ready(true,true);!r)return std::unexpected(r.error());Guard guard{p->changing};
    auto target=p->resolve(path,std::move(params));if(!target)return p->reject(std::string(path.substr(0,RouteLimits::path_bytes)),target.error());return p->navigate(std::move(*target),operation);
}
core::Result<bool> PageRouter::go_to(PageComponentRef component,core::Value params,NavigationOperation operation){
    auto p=impl_;if(auto r=p->ready(true,true);!r)return std::unexpected(r.error());Guard guard{p->changing};
    if(!component||!*component)return p->reject({},core::Error{core::ErrorCode::invalid_argument,"Navigation requires a compiled page constructor"});
    auto entry=p->history.create_component_path_entry(component,std::move(params));if(!entry)return p->reject({},entry.error());return p->navigate({std::move(*entry),std::move(component)},operation);
}
core::Result<bool> PageRouter::back(){auto p=impl_;if(auto r=p->ready(true,true);!r)return std::unexpected(r.error());Guard guard{p->changing};return p->remove(false);}
core::Result<bool> PageRouter::pop_to_root(){auto p=impl_;if(auto r=p->ready(true,true);!r)return std::unexpected(r.error());Guard guard{p->changing};return p->remove(true);}
core::Result<PageRouterState> PageRouter::state()const{
    auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());auto history=p->history.state();if(!history)return std::unexpected(history.error());PageRouterState result{.navigation=std::move(*history),.mounted=p->mounted,.can_go_back=history->depth>1};
    if(p->mounted){auto snapshot=p->host.snapshot();if(!snapshot)return std::unexpected(snapshot.error());result.content_generation=snapshot->content_generation;}return result;
}
core::Result<std::vector<NavigationEntry>> PageRouter::path()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->history.path();}
core::Result<std::vector<NavigationViewEntry>> PageRouter::view_tracking_entries()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->history.view_tracking_entries();}
core::Result<NavigationPlacement> PageRouter::placement()const{auto p=impl_;if(auto r=p->ready(false,true);!r)return std::unexpected(r.error());auto place=p->host.placement();if(!place)return std::unexpected(place.error());return NavigationPlacement{place->scope,place->parent,place->sibling};}
core::Result<gui::Widget> PageRouter::current_page_item(std::string_view id)const{auto p=impl_;if(auto r=p->ready(false,true);!r)return std::unexpected(r.error());return p->host.content_item(id);}
core::Result<RouteResolverState> PageRouter::resolver_state()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->resolver.state();}
core::Result<void> PageRouter::observe(std::weak_ptr<PageNavigationPort> receiver){auto p=impl_;if(auto r=p->ready();!r)return r;return observe_port(p->receivers,std::move(receiver));}
core::Result<void> PageRouter::observe_failures(std::weak_ptr<PageNavigationFailurePort> receiver){auto p=impl_;if(auto r=p->ready(true);!r)return r;return observe_port(p->failures,std::move(receiver));}
}
