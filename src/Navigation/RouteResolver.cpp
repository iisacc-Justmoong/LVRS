#include <LVRS/RouteResolver.h>
#include <LVRS/RoutingDetail.h>
#include <algorithm>
#include <list>
#include <thread>
#include <unordered_map>

namespace lvrs {
using namespace ii;
struct RouteResolver::Impl final {
    struct Cached final {std::string path;std::shared_ptr<const RouteResolution> resolution;std::size_t bytes=0;};
    std::thread::id owner=std::this_thread::get_id();
    std::vector<std::string> patterns;
    std::vector<std::optional<routing_detail::Pattern>> compiled;
    std::list<Cached> lru;
    std::unordered_map<std::string,decltype(lru)::iterator> cache;
    RouteResolverState state;
    std::vector<std::weak_ptr<RouteResolverChangePort>> receivers;
    bool notifying=false;
    core::Result<void> ready(bool mutate=false)const{
        if(owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Route resolver requires its owner thread");
        if(mutate&&notifying)return core::fail(core::ErrorCode::busy,"Route resolver mutation during notification");return {};
    }
    void clear(){cache.clear();lru.clear();state.cache_bytes=0;state.cached_entries=0;}
    void evict(){while(!lru.empty()&&(lru.size()>state.cache_capacity||state.cache_bytes>RouteLimits::cache_bytes)){state.cache_bytes-=lru.front().bytes;cache.erase(lru.front().path);lru.pop_front();}state.cached_entries=lru.size();}
    core::Result<void> notify(RouteResolverChange change){
        std::vector<std::shared_ptr<RouteResolverChangePort>> live;std::erase_if(receivers,[](const auto& receiver){return receiver.expired();});for(const auto& receiver:receivers)if(auto port=receiver.lock())live.push_back(std::move(port));
        notifying=true;struct Reset final {bool& flag;~Reset(){flag=false;}}reset{notifying};std::optional<core::Error> failed;
        for(const auto& port:live){auto reply=port->send(change);if(!reply&&reply.error().code!=runtime::QueryErrorCode::receiver_expired&&!failed)failed=core::Error{core::ErrorCode::failed,"Resolver receiver failed after configuration was committed",reply.error().cause};}
        if(failed)return std::unexpected(*failed);return {};
    }
};
RouteResolver::RouteResolver():impl_(std::make_shared<Impl>()){}
RouteResolver::~RouteResolver()=default;
core::Result<bool> RouteResolver::set_route_patterns(std::vector<std::string> patterns){
    auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(patterns==p->patterns)return false;
    if(patterns.size()>RouteLimits::patterns)return core::fail(core::ErrorCode::full,"Too many route patterns");
    std::vector<std::optional<routing_detail::Pattern>> compiled;compiled.reserve(patterns.size());std::size_t bytes=0,tokens=0;
    for(const auto& path:patterns){
        if(path.size()>RouteLimits::pattern_bytes-bytes)return core::fail(core::ErrorCode::full,"Route pattern byte budget exceeded");bytes+=path.size();
        if(path.empty()){compiled.emplace_back();continue;}
        auto pattern=routing_detail::compile(path);if(!pattern)return std::unexpected(pattern.error());
        if(pattern->tokens.size()>RouteLimits::pattern_segments-tokens)return core::fail(core::ErrorCode::full,"Route pattern token budget exceeded");tokens+=pattern->tokens.size();compiled.emplace_back(std::move(*pattern));
    }
    p->patterns=std::move(patterns);p->compiled=std::move(compiled);p->state.patterns=p->patterns.size();++p->state.revision;p->clear();
    if(auto r=p->notify({p->state.revision,true,false});!r)return std::unexpected(r.error());return true;
}
core::Result<bool> RouteResolver::set_cache_capacity(std::int64_t value){
    auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());value=std::max<std::int64_t>(16,value);
    if(std::uint64_t(value)>RouteLimits::cache_capacity)return core::fail(core::ErrorCode::full,"Resolver cache capacity exceeds its budget");
    if(p->state.cache_capacity==std::size_t(value))return false;p->state.cache_capacity=std::size_t(value);++p->state.revision;p->evict();
    if(auto r=p->notify({p->state.revision,false,true});!r)return std::unexpected(r.error());return true;
}
core::Result<std::shared_ptr<const RouteResolution>> RouteResolver::resolve(std::string_view path){
    auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());auto normalized=RouteMatcher::normalize_path(path);if(!normalized)return std::unexpected(normalized.error());
    if(auto hit=p->cache.find(*normalized);hit!=p->cache.end()){++p->state.cache_hits;p->lru.splice(p->lru.end(),p->lru,hit->second);return hit->second->resolution;}
    auto segments=routing_detail::segments(*normalized);if(!segments)return std::unexpected(segments.error());++p->state.cache_misses;
    auto result=std::make_shared<RouteResolution>();
    for(std::size_t i=0;i<p->compiled.size();++i){if(!p->compiled[i])continue;++p->state.pattern_matches;
        auto matched=routing_detail::match(*segments,*p->compiled[i]);if(matched.matched){result->matched=true;result->index=std::ptrdiff_t(i);result->params=std::move(matched.params);break;}
    }
    std::shared_ptr<const RouteResolution> immutable=result;
    std::size_t bytes=128+normalized->size()*2;for(const auto& [key,value]:result->params)bytes+=64+key.size()+value.size();
    if(bytes<=RouteLimits::cache_bytes){
        p->lru.push_back({*normalized,immutable,bytes});auto item=std::prev(p->lru.end());
        try{p->cache.emplace(item->path,item);}catch(...){p->lru.pop_back();throw;}
        p->state.cache_bytes+=bytes;p->evict();
    }
    return immutable;
}
core::Result<void> RouteResolver::clear_cache(){auto p=impl_;if(auto r=p->ready(true);!r)return r;p->clear();return {};}
core::Result<RouteResolverState> RouteResolver::state()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->state;}
core::Result<std::span<const std::string>> RouteResolver::route_patterns()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return std::span<const std::string>{p->patterns};}
core::Result<void> RouteResolver::observe(std::weak_ptr<RouteResolverChangePort> receiver){
    auto p=impl_;if(auto r=p->ready(true);!r)return r;auto incoming=receiver.lock();if(!incoming)return core::fail(core::ErrorCode::invalid_argument,"Resolver receiver already expired");
    std::erase_if(p->receivers,[](const auto& port){return port.expired();});for(const auto& weak:p->receivers)if(weak.lock()==incoming)return {};p->receivers.push_back(std::move(receiver));return {};
}
}
