#include "Navigator.h"
#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>

namespace lvrs {
using namespace ii;
namespace {
struct Guard final {bool& value;bool before;explicit Guard(bool& flag):value(flag),before(flag){value=true;}~Guard(){value=before;}};
template<class F>auto safely(F&& operation)->decltype(operation()){
    try{return operation();}catch(...){return core::fail(core::ErrorCode::failed,"Navigation target callback failed",std::current_exception());}
}
bool same_group(const NavigatorPlacement& a,const NavigatorPlacement& b){
    return !a.sibling_group.expired()&&!b.sibling_group.expired()&&a.parent==b.parent&&!a.sibling_group.owner_before(b.sibling_group)&&!b.sibling_group.owner_before(a.sibling_group);
}
bool same_state(const NavigationTargetState& a,const NavigationTargetState& b){
    return a.mounted==b.mounted&&a.can_go_back==b.can_go_back&&a.content_generation==b.content_generation&&a.navigation.revision==b.navigation.revision&&a.navigation.depth==b.navigation.depth&&a.navigation.current_path==b.navigation.current_path&&a.navigation.current_params==b.navigation.current_params;
}
void raise(core::Error error){if(error.cause)std::rethrow_exception(error.cause);throw std::runtime_error(error.message);}
}
struct Navigator::Impl final : std::enable_shared_from_this<Impl> {
    struct Entry final {
        std::uint64_t token=0;
        std::weak_ptr<NavigationTarget> target;
        NavigatorPlacement placement;
        NavigationTargetState cached;
        std::shared_ptr<NavigationTargetPort> port;
        bool scoped=false,retired=false;
        std::weak_ptr<const void> lease;
    };
    std::thread::id owner=std::this_thread::get_id();
    std::vector<Entry> entries;
    std::vector<std::weak_ptr<NavigatorPort>> receivers;
    std::shared_ptr<ViewStateTracker> tracker;
    NavigatorState snapshot;
    std::weak_ptr<NavigationTarget> published_target;
    std::vector<std::weak_ptr<NavigationTarget>> published_stack;
    std::uint64_t next_token=1,active_token=0;
    bool changing=false,notifying=false,dirty=false,stack_dirty=false,tracking_dirty=true;
    core::Result<void> ready(bool mutate=false)const{
        if(owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Navigator requires its owner thread");
        if(mutate&&(changing||notifying))return core::fail(core::ErrorCode::busy,"Navigator is dispatching or notifying another command");return {};
    }
    static bool live(const Entry& entry){return !entry.retired&&!entry.target.expired()&&(!entry.scoped||!entry.lease.expired());}
    auto find(std::uint64_t token){return std::find_if(entries.begin(),entries.end(),[&](const auto& entry){return entry.token==token;});}
    std::pair<std::uint64_t,std::shared_ptr<NavigationTarget>> active(){
        for(auto i=entries.rbegin();i!=entries.rend();++i)if(live(*i))if(auto target=i->target.lock())return {i->token,std::move(target)};return {};
    }
    void prune(){const auto before=entries.size();std::erase_if(entries,[](const auto& entry){return !live(entry);});if(before!=entries.size()){dirty=true;stack_dirty=true;}}
    core::Result<void> notify(NavigatorChange change){
        std::vector<std::shared_ptr<NavigatorPort>> live_receivers;
        std::erase_if(receivers,[](const auto& receiver){return receiver.expired();});for(const auto& weak:receivers)if(auto receiver=weak.lock())live_receivers.push_back(std::move(receiver));
        Guard guard{notifying};std::optional<core::Error> failure;
        for(const auto& receiver:live_receivers){auto reply=receiver->send(change);if(!reply&&reply.error().code!=runtime::QueryErrorCode::receiver_expired&&!failure)failure=core::Error{core::ErrorCode::failed,"Navigator receiver failed after state was committed",reply.error().cause};}
        if(failure)return std::unexpected(*failure);return {};
    }
    core::Result<void> flush(){
        prune();auto selected=active();NavigationTargetState next;
        // A target is kept alive over its call; no borrowed vector entry crosses
        // foreign code. Internal lease retirement only marks entries while busy.
        while(selected.second){
            auto status=safely([&]{return selected.second->state();});auto found=find(selected.first);
            if(found==entries.end()||!live(*found)){prune();selected=active();continue;}
            if(!status&&status.error().code!=core::ErrorCode::busy&&status.error().code!=core::ErrorCode::closed)return std::unexpected(status.error());
            if((!status&&status.error().code==core::ErrorCode::closed)||(status&&!status->mounted)){found->retired=true;prune();selected=active();continue;}
            if(status)found->cached=std::move(*status);next=found->cached;break;
        }
        const bool current=active_token!=selected.first||!same_state(snapshot.current,next);
        const bool routers=stack_dirty||snapshot.router_count!=entries.size();
        if(!routers&&!current&&!tracking_dirty){dirty=false;return {};}
        if(routers||current){
            if(snapshot.revision==std::numeric_limits<std::uint64_t>::max())return core::fail(core::ErrorCode::full,"Navigator revision exhausted");
            active_token=selected.first;published_target=selected.second;
            if(routers){published_stack.clear();published_stack.reserve(entries.size());for(const auto& entry:entries)if(live(entry))published_stack.push_back(entry.target);}
            snapshot={.revision=snapshot.revision+1,.router_count=entries.size(),.has_router=bool(selected.second),.current=std::move(next)};
            dirty=false;stack_dirty=false;tracking_dirty|=current;
        }
        // Commit selection before delivering tracker callbacks. An inactive
        // router's direct navigation must not overwrite the selected stack.
        core::Result<void> tracked;
        if(tracking_dirty){
            auto descriptors=selected.second?safely([&]{return selected.second->view_tracking_entries();}):core::Result<std::vector<NavigationViewEntry>>{std::vector<NavigationViewEntry>{}};
            if(descriptors){auto synced=tracker->sync_navigation(*descriptors);if(synced||synced.error().code==core::ErrorCode::failed)tracking_dirty=false;if(!synced)tracked=std::unexpected(synced.error());}
            else if(descriptors.error().code!=core::ErrorCode::busy&&descriptors.error().code!=core::ErrorCode::closed)tracked=std::unexpected(descriptors.error());
        }
        auto sent=(routers||current)?notify({snapshot,routers,current}):core::Result<void>{};
        if(!tracked)return tracked;return sent;
    }
    core::Result<void> retire(std::uint64_t token){
        if(auto r=ready();!r)return r;auto found=find(token);if(found==entries.end()||found->retired)return {};
        found->retired=true;dirty=true;stack_dirty=true;if(changing||notifying)return {};
        Guard guard{changing};return flush();
    }
    void updated(std::uint64_t token,const NavigationTargetChange& event){
        if(auto r=ready();!r)raise(r.error());
        auto found=find(token);if(found==entries.end()||!live(*found))return;
        found->cached={.navigation=event.navigation,.mounted=true,.can_go_back=event.navigation.depth>1,.content_generation=event.content_generation};dirty=true;
        if(!changing&&!notifying){Guard guard{changing};if(auto result=flush();!result)raise(result.error());}
    }
    core::Result<void> insert(std::shared_ptr<NavigationTarget> target,NavigatorPlacement placement,bool scoped,std::weak_ptr<const void> lease,std::uint64_t& token){
        prune();const auto existing=std::find_if(entries.begin(),entries.end(),[&](const auto& entry){return entry.target.lock()==target;});const bool replacing=existing!=entries.end();
        if(entries.size()>=4096&&!replacing)return core::fail(core::ErrorCode::full,"Navigator registry limit is 4096");
        const auto preserved=!scoped&&replacing&&existing->scoped?existing->token:0;
        std::shared_ptr<NavigationTargetPort> port;
        if(preserved){scoped=true;lease=existing->lease;port=existing->port;}
        if(!preserved&&next_token==std::numeric_limits<std::uint64_t>::max())return core::fail(core::ErrorCode::full,"Navigator registration identity exhausted");
        auto status=safely([&]{return target->state();});if(!status)return std::unexpected(status.error());if(!status->mounted)return core::fail(core::ErrorCode::invalid_argument,"Register a mounted navigation target");
        if(placement.sibling_group.expired()){auto derived=safely([&]{return target->placement();});if(!derived)return std::unexpected(derived.error());placement=std::move(*derived);}
        const auto identity=preserved?preserved:next_token++;auto weak=weak_from_this();
        if(!port){port=std::make_shared<NavigationTargetPort>([weak,identity](const auto& event){if(auto p=weak.lock())p->updated(identity,event);});auto observed=safely([&]{return target->observe(port);});if(!observed)return observed;}
        // An observer implementation can deliver immediately. Re-read before
        // publishing to avoid caching its pre-subscription revision.
        status=safely([&]{return target->state();});if(!status)return std::unexpected(status.error());if(!status->mounted)return core::fail(core::ErrorCode::closed,"Target retired during registration");
        if(preserved&&(lease.expired()||find(preserved)==entries.end()||!live(*find(preserved))))return core::fail(core::ErrorCode::closed,"Scoped target retired during re-registration");
        std::erase_if(entries,[&](const auto& entry){return entry.target.lock()==target;});
        auto position=std::find_if(entries.begin(),entries.end(),[&](const auto& entry){return same_group(placement,entry.placement)&&entry.placement.sibling_order>placement.sibling_order;});
        entries.insert(position,Entry{identity,target,std::move(placement),std::move(*status),std::move(port),scoped,false,std::move(lease)});
        token=identity;dirty=true;stack_dirty=true;return {};
    }
    template<class Command>core::Result<bool> dispatch(Command&& command){
        if(auto r=ready(true);!r)return std::unexpected(r.error());Guard guard{changing};
        if(auto r=flush();!r)return std::unexpected(r.error());auto selected=active();if(!selected.second)return false;
        auto result=safely([&]{return command(*selected.second);});if(!result&&result.error().code==core::ErrorCode::closed)(void)retire(selected.first);
        auto notified=flush();if(!result)return std::unexpected(result.error());if(!notified)return std::unexpected(notified.error());return true;
    }
};
NavigatorRegistration::~NavigatorRegistration(){live_.reset();if(close_&&owner_==std::this_thread::get_id())try{(void)close_();}catch(...){} }
core::Result<void> NavigatorRegistration::close(){
    if(owner_!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Close Navigator registrations on their owner thread");
    if(!live_)return {};live_.reset();auto close=std::move(close_);return close?close():core::Result<void>{};
}
Navigator::Navigator(std::shared_ptr<ViewStateTracker> tracker):impl_(std::make_shared<Impl>()){impl_->tracker=tracker?std::move(tracker):std::make_shared<ViewStateTracker>();}
Navigator::~Navigator()=default;
std::shared_ptr<Navigator> Navigator::current(){thread_local auto instance=std::make_shared<Navigator>(ViewStateTracker::current());return instance;}
core::Result<bool> Navigator::register_router(std::shared_ptr<NavigationTarget> target,NavigatorPlacement placement){
    auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(!target)return false;Guard guard{p->changing};std::uint64_t token=0;
    if(auto r=p->insert(std::move(target),std::move(placement),false,{},token);!r)return std::unexpected(r.error());if(auto r=p->flush();!r)return std::unexpected(r.error());return true;
}
core::Result<void> Navigator::track_router(std::shared_ptr<NavigationTarget> target,std::shared_ptr<NavigatorRegistration>& lease,NavigatorPlacement placement){
    auto p=impl_;if(auto r=p->ready(true);!r)return r;if(!target)return core::fail(core::ErrorCode::invalid_argument,"Scoped registration requires a target");Guard guard{p->changing};
    auto next=std::shared_ptr<NavigatorRegistration>(new NavigatorRegistration);next->owner_=p->owner;next->live_=std::make_shared<const int>(0);std::uint64_t token=0;
    if(auto r=p->insert(std::move(target),std::move(placement),true,next->live_,token);!r)return r;
    next->close_=[weak=std::weak_ptr<Impl>{p},token]()->core::Result<void>{if(auto p=weak.lock())return p->retire(token);return {};};
    lease=std::move(next);return p->flush();
}
core::Result<void> Navigator::unregister_router(std::shared_ptr<NavigationTarget> target){
    auto p=impl_;if(auto r=p->ready(true);!r)return r;Guard guard{p->changing};
    for(auto& entry:p->entries)if(!target||entry.target.lock()==target){entry.retired=true;p->dirty=true;p->stack_dirty=true;}return p->flush();
}
core::Result<NavigatorState> Navigator::state(){auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());if(!p->changing&&!p->notifying){Guard guard{p->changing};if(auto r=p->flush();!r)return std::unexpected(r.error());}return p->snapshot;}
core::Result<std::shared_ptr<NavigationTarget>> Navigator::router(){auto p=impl_;auto status=state();if(!status)return std::unexpected(status.error());return p->published_target.lock();}
core::Result<std::vector<std::shared_ptr<NavigationTarget>>> Navigator::router_stack(){
    auto p=impl_;auto status=state();if(!status)return std::unexpected(status.error());std::vector<std::shared_ptr<NavigationTarget>> result;
    for(const auto& weak:p->published_stack)if(auto target=weak.lock())result.push_back(std::move(target));return result;
}
core::Result<bool> Navigator::can_navigate(){auto status=state();if(!status)return std::unexpected(status.error());return status->has_router;}
core::Result<std::shared_ptr<ViewStateTracker>> Navigator::view_state_tracker(){auto p=impl_;auto status=state();if(!status)return std::unexpected(status.error());return p->tracker;}
core::Result<bool> Navigator::go(std::string_view path,core::Value params,NavigationOperation operation){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(path.empty())return false;return p->dispatch([&](NavigationTarget& target){return target.go(path,std::move(params),operation);});}
core::Result<bool> Navigator::go_to(PageComponentRef page,core::Value params,NavigationOperation operation){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(!page)return false;return p->dispatch([&](NavigationTarget& target){return target.go_to(std::move(page),std::move(params),operation);});}
core::Result<bool> Navigator::back(){auto p=impl_;return p->dispatch([](NavigationTarget& target){return target.back();});}
core::Result<bool> Navigator::pop_to_root(){auto p=impl_;return p->dispatch([](NavigationTarget& target){return target.pop_to_root();});}
core::Result<void> Navigator::observe(std::weak_ptr<NavigatorPort> receiver){
    auto p=impl_;if(auto r=p->ready(true);!r)return r;auto live=receiver.lock();if(!live)return core::fail(core::ErrorCode::invalid_argument,"Navigator receiver already expired");
    std::erase_if(p->receivers,[](const auto& weak){return weak.expired();});for(const auto& weak:p->receivers)if(weak.lock()==live)return {};p->receivers.push_back(std::move(receiver));return {};
}
}
