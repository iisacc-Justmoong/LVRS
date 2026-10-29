#include <LVRS/PageRouter.h>
#include <LVRS/Navigator.h>
#include <LVRS/ViewModelRegistry.h>
#include <algorithm>
#include <map>
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
    struct Target final {
        NavigationEntry entry;PageComponentRef component;
        const PageRoute* route=nullptr;
        bool bind_model=true;
        RouteModelBinding binding;
        Target(NavigationEntry value,PageComponentRef page,const PageRoute* declared=nullptr,bool bind=true):entry(std::move(value)),component(std::move(page)),route(declared),bind_model(bind){}
    };
    using References=std::map<std::string,std::size_t,std::less<>>;
    struct TrackingPlan final {
        std::size_t keep=0;
        bool enabled=false,append=false;
        std::string id;
        References::node_type addition;
        std::vector<std::string> removed;
    };
    std::thread::id owner=std::this_thread::get_id();
    PageRouterOptions options;
    std::weak_ptr<PageRouter> self;
    NavigationStackModel history;
    RouteResolver resolver;
    gui::Widget host;
    PageComponentRef active_component;
    bool mounted=false,changing=false,notifying=false,retired=false;
    std::vector<std::string> tracked_ids;
    References references;
    std::vector<std::weak_ptr<PageNavigationPort>> receivers;
    std::vector<std::weak_ptr<PageNavigationFailurePort>> failures;
    core::Result<void> ready(bool mutate=false,bool require_mount=false)const{
        if(owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Page router requires its owner thread");
        if(retired)return core::fail(core::ErrorCode::closed,"Page router host retired");
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
        const auto* route=(*resolution)->matched?&options.routes[std::size_t((*resolution)->index)]:nullptr;
        auto component=route?route->component:options.not_found_component;
        if(!component||!*component)return core::fail(core::ErrorCode::not_found,"No compiled page matches this route");
        return Target{{.path=std::move(*normalized),.params=core::Value{std::move(merged)}},std::move(component),route,bool(route)};
    }
    void identify(Target& target,std::size_t index){
        static const core::Value::Object empty;
        const auto* params=std::get_if<core::Value::Object>(&target.entry.params.storage());
        target.binding=ViewModelRegistry::route_binding(target.entry.path,target.route?target.route->metadata:empty,params?*params:empty,std::ptrdiff_t(index));
        target.entry.view_id=target.binding.view_id;
    }
    core::Result<TrackingPlan> plan_tracking(std::size_t keep,const std::string* id){
        TrackingPlan plan{.keep=keep,.enabled=bool(options.view_models),.append=id!=nullptr};
        if(!plan.enabled)return plan;
        if(keep>tracked_ids.size())return core::fail(core::ErrorCode::failed,"Page model tracking does not match navigation history");
        plan.removed.assign(tracked_ids.begin()+std::ptrdiff_t(keep),tracked_ids.end());
        if(id){
            plan.id=*id;
            if(!references.contains(*id)){References node;node.emplace(*id,0);plan.addition=node.extract(node.begin());}
            const auto needed=keep+1;
            if(tracked_ids.capacity()<needed)tracked_ids.reserve(std::min(NavigationLimits::depth,std::max(needed,tracked_ids.capacity()?tracked_ids.capacity()*2:std::size_t(8))));
        }
        return plan;
    }
    void apply_tracking(TrackingPlan& plan){
        if(!plan.enabled)return;
        // All nodes, strings and vector capacity were prepared before building
        // the page. No history or registry-wide snapshot is copied for a push.
        if(plan.append){
            auto found=references.find(plan.id);
            if(found==references.end())found=references.insert(std::move(plan.addition)).position;
            ++found->second;
        }
        for(const auto& id:plan.removed){auto found=references.find(id);if(--found->second==0)references.erase(found);}
        tracked_ids.resize(plan.keep);if(plan.append)tracked_ids.push_back(std::move(plan.id));
        std::erase_if(plan.removed,[&](const auto& id){return references.contains(id);});
        std::sort(plan.removed.begin(),plan.removed.end());plan.removed.erase(std::unique(plan.removed.begin(),plan.removed.end()),plan.removed.end());
    }
    void retire()noexcept{
        if(retired)return;retired=true;
        auto removed=std::move(tracked_ids);references.clear();
        if(!options.view_models)return;
        try{std::sort(removed.begin(),removed.end());removed.erase(std::unique(removed.begin(),removed.end()),removed.end());(void)options.view_models->retire_views(removed);}catch(...){/* Closing a widget scope cannot throw. */}
    }
    core::Result<bool> reject(std::string path,core::Error error){
        Guard guard{notifying};(void)notify_ports(failures,PageNavigationFailure{std::move(path),error});return std::unexpected(std::move(error));
    }
    core::Result<bool> publish(Target next,std::function<core::Result<bool>()> commit,TrackingPlan plan){
        // Keep both component constructors alive across old-scope destruction.
        auto previous=active_component;bool changed=false;
        auto built=host.set_content([&](gui::WidgetBuilder& widgets){return next.component->build(widgets,{},next.entry);},[&]()->core::Result<void>{auto result=commit();if(!result)return std::unexpected(result.error());changed=*result;return {};});
        if(!built)return reject(next.entry.path,built.error());
        active_component=std::move(next.component);
        apply_tracking(plan);
        Guard guard{notifying};std::optional<core::Error> binding_failure;
        if(options.view_models){
            auto released=options.view_models->retire_views(plan.removed);if(!released)binding_failure=released.error();
            if(!retired&&next.bind_model&&!next.binding.model_key.empty()){
                auto bound=options.view_models->bind_view(next.binding.view_id,next.binding.model_key,next.binding.writable);
                if(!bound&&!binding_failure)binding_failure=bound.error();
                // A rejected model key or ownership claim is a registry business
                // error, as in QML. It does not roll back a published page.
                if(retired){
                    // Clearing last_error may retire the host before bind_view
                    // publishes this new ID. Repeat cleanup after that command.
                    auto late=options.view_models->retire_views(std::span(&next.binding.view_id,1));if(!late&&!binding_failure)binding_failure=late.error();
                }
            }
        }
        auto current=history.state();if(!current)return std::unexpected(current.error());
        auto snapshot=host.snapshot();if(!snapshot)return std::unexpected(snapshot.error());
        auto sent=notify_ports(receivers,PageNavigationChange{std::move(*current),active_component,snapshot->content_generation});
        if(binding_failure)return std::unexpected(*binding_failure);if(!sent)return std::unexpected(sent.error());return changed;
    }
    core::Result<bool> navigate(Target next,NavigationOperation operation){
        auto path=history.path_view();if(!path)return std::unexpected(path.error());
        const bool root=path->empty()||operation==NavigationOperation::set_root;
        const auto keep=root?0:operation==NavigationOperation::replace?path->size()-1:path->size();
        identify(next,keep);
        const auto& entry=next.entry;
        if(options.view_models){auto admitted=options.view_models->validate_view_admission(entry.view_id);if(!admitted)return reject(entry.path,admitted.error());}
        auto preflight=history.validate_entry_operation(entry,operation);
        if(!preflight)return reject(entry.path,preflight.error());
        if(!path->empty()&&(operation==NavigationOperation::replace||(operation==NavigationOperation::set_root&&path->size()==1))&&path->back()==entry)return false;
        auto tracking=plan_tracking(keep,&entry.view_id);if(!tracking)return reject(entry.path,tracking.error());
        // Only the single candidate is copied. Push never clones the old stack.
        const auto descriptor=next.entry;
        return publish(std::move(next),[this,descriptor,operation]{return history.apply_entry_operation(descriptor,operation);},std::move(*tracking));
    }
    core::Result<bool> remove(bool root){
        auto path=history.path_view();if(!path)return std::unexpected(path.error());if(path->size()<=1)return false;
        const auto entry=(*path)[root?0:path->size()-2];
        core::Result<Target> next=entry.component?core::Result<Target>{Target{entry,std::static_pointer_cast<const PageComponent>(entry.component)}}:resolve(entry.path,entry.params);
        if(!next)return reject(entry.path,next.error());
        const auto keep=root?1:path->size()-1;identify(*next,keep-1);
        if(options.view_models){auto admitted=options.view_models->validate_view_admission(next->entry.view_id);if(!admitted)return reject(entry.path,admitted.error());}
        auto tracking=plan_tracking(keep,nullptr);if(!tracking)return reject(entry.path,tracking.error());
        return publish(std::move(*next),[this,root]{return root?history.pop_to_root():history.pop();},std::move(*tracking));
    }
};
core::Result<void> PageComponent::build(gui::WidgetBuilder& widgets,gui::Widget parent,const NavigationEntry& context)const{
    if(!factory_)return core::fail(core::ErrorCode::invalid_argument,"Compiled page has no constructor");return factory_(widgets,parent,context);
}
PageRouter::PageRouter():impl_(std::make_shared<Impl>()){}
PageRouter::~PageRouter(){impl_->retire();}
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
    if(auto hook=widgets.on_retire([weak=std::weak_ptr(p)]{if(auto state=weak.lock())state->retire();});!hook)return std::unexpected(hook.error());
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
