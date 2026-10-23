#include "NavigationStackModel.h"
#include "RoutingDetail.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <optional>
#include <thread>
#include <unordered_set>

namespace lvrs {
using namespace ii;
namespace {
using Value=core::Value;
struct Prepared final {NavigationEntry entry;std::size_t bytes=0;};
struct Guard final {bool& flag;explicit Guard(bool& value):flag(value){flag=true;}~Guard(){flag=false;}};
core::Result<Prepared> prepare(NavigationEntry entry){
    if(!entry.path.empty()){auto normalized=RouteMatcher::normalize_path(entry.path);if(!normalized)return std::unexpected(normalized.error());entry.path=std::move(*normalized);}
    if(entry.view_id.size()>NavigationLimits::view_id_bytes)return core::fail(core::ErrorCode::full,"Navigation view id exceeds its budget");
    if(entry.view_id.find('\0')!=entry.view_id.npos||!core::valid_utf8(entry.view_id))return core::fail(core::ErrorCode::invalid_argument,"Navigation view id requires UTF-8 without NUL");
    if(entry.params.kind()==core::ValueKind::null)entry.params=Value{Value::Object{}};
    std::size_t bytes=64+entry.path.size()+entry.view_id.size(),nodes=0;
    const auto charge=[&](std::size_t n){if(n>NavigationLimits::entry_bytes-bytes)return false;bytes+=n;return true;};
    struct Pending final {const Value* value;std::size_t depth;};std::vector<Pending> pending{{&entry.params,0}};
    while(!pending.empty()){
        const auto [value,depth]=pending.back();pending.pop_back();
        if(depth>NavigationLimits::value_depth||++nodes>NavigationLimits::value_nodes||!charge(32))return core::fail(core::ErrorCode::full,"Navigation parameter structure exceeds its budget");
        if(auto text=std::get_if<std::string>(&value->storage());text&&!charge(text->size()))return core::fail(core::ErrorCode::full,"Navigation text parameters exceed their budget");
        if(auto data=std::get_if<core::Bytes>(&value->storage());data&&!charge(data->size()))return core::fail(core::ErrorCode::full,"Navigation binary parameters exceed their budget");
        if(auto array=std::get_if<Value::Array>(&value->storage())){
            if(array->size()>NavigationLimits::value_nodes-nodes-pending.size())return core::fail(core::ErrorCode::full,"Navigation array exceeds its node budget");
            for(const auto& child:*array)pending.push_back({&child,depth+1});
        }
        if(auto object=std::get_if<Value::Object>(&value->storage())){
            if(object->size()>NavigationLimits::value_nodes-nodes-pending.size())return core::fail(core::ErrorCode::full,"Navigation object exceeds its node budget");
            for(const auto& [key,child]:*object){if(!charge(key.size()))return core::fail(core::ErrorCode::full,"Navigation object keys exceed their budget");pending.push_back({&child,depth+1});}
        }
    }
    return Prepared{std::move(entry),bytes};
}
NavigationViewEntry track(const NavigationEntry& entry,std::size_t index){
    auto id=std::string(routing_detail::trim(entry.view_id));if(id.empty())id=entry.path.empty()?"_component_"+std::to_string(index):entry.path;
    bool enabled=entry.enabled&&!entry.disabled;
    if(auto params=entry.params.get<Value::Object>();params){auto found=params->get().find("disabled");if(found!=params->get().end())if(auto disabled=found->second.get<bool>();disabled&&disabled->get())enabled=false;}
    return {std::move(id),entry.path,enabled};
}
bool current_differs(const NavigationEntry* a,const NavigationEntry* b){
    if(!a||!b){const auto* present=a?a:b;return present&&(!present->path.empty()||present->params!=Value{Value::Object{}});}
    return a->path!=b->path||a->params!=b->params;
}
bool valid_operation(NavigationOperation value){return unsigned(value)<=unsigned(NavigationOperation::set_root);}
}
struct NavigationStackModel::Impl final {
    std::thread::id owner=std::this_thread::get_id();
    std::vector<NavigationEntry> entries;
    std::vector<NavigationViewEntry> tracking;
    std::vector<std::size_t> costs;
    std::vector<std::string> tracked_ids;
    std::vector<std::weak_ptr<NavigationChangePort>> receivers;
    std::uint64_t revision=1;
    std::size_t bytes=0;
    bool changing=false,notifying=false;
    core::Result<void> ready(bool mutate=false)const{
        if(owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Navigation history requires its owner thread");
        if((changing&&!notifying)||(mutate&&(changing||notifying)))return core::fail(core::ErrorCode::busy,"Navigation history is committing or notifying another command");return {};
    }
    const NavigationEntry* current()const{return entries.empty()?nullptr:&entries.back();}
    core::Result<void> notify(NavigationChange change){
        change.revision=++revision;std::vector<std::shared_ptr<NavigationChangePort>> live;
        std::erase_if(receivers,[](const auto& port){return port.expired();});for(const auto& receiver:receivers)if(auto port=receiver.lock())live.push_back(std::move(port));
        Guard guard{notifying};std::optional<core::Error> failed;
        for(const auto& port:live){auto reply=port->send(change);if(!reply&&reply.error().code!=runtime::QueryErrorCode::receiver_expired&&!failed)failed=core::Error{core::ErrorCode::failed,"Navigation receiver failed after history was committed",reply.error().cause};}
        if(failed)return std::unexpected(*failed);return {};
    }
    core::Result<std::size_t> operation_bytes(std::size_t cost,NavigationOperation operation)const{
        if(!valid_operation(operation))return core::fail(core::ErrorCode::invalid_argument,"Unknown navigation operation");
        const bool root=entries.empty()||operation==NavigationOperation::set_root;
        if(!root&&operation==NavigationOperation::push&&entries.size()==NavigationLimits::depth)return core::fail(core::ErrorCode::full,"Navigation history depth budget exceeded");
        const auto retained=root?0:bytes-(operation==NavigationOperation::replace?costs.back():0);
        if(cost>NavigationLimits::total_bytes-retained)return core::fail(core::ErrorCode::full,"Navigation history byte budget exceeded");return retained+cost;
    }
    core::Result<std::vector<NavigationEntry>> preview(NavigationEntry entry,NavigationOperation operation)const{
        auto next=prepare(std::move(entry));if(!next)return std::unexpected(next.error());auto total=operation_bytes(next->bytes,operation);if(!total)return std::unexpected(total.error());
        if(entries.empty()||operation==NavigationOperation::set_root)return std::vector<NavigationEntry>{std::move(next->entry)};
        auto result=entries;if(operation==NavigationOperation::replace)result.back()=std::move(next->entry);else result.push_back(std::move(next->entry));return result;
    }
    core::Result<bool> apply(NavigationEntry entry,NavigationOperation operation){
        auto next=prepare(std::move(entry));if(!next)return std::unexpected(next.error());auto total=operation_bytes(next->bytes,operation);if(!total)return std::unexpected(total.error());
        const bool root=entries.empty()||operation==NavigationOperation::set_root,replace=!root&&operation==NavigationOperation::replace;
        if((replace||(root&&entries.size()==1))&&entries.back()==next->entry)return false;
        const auto index=root?0:replace?entries.size()-1:entries.size();auto next_tracking=track(next->entry,index);
        NavigationChange change{.path=true,.current=current_differs(current(),&next->entry),.tracking=root?tracking!=std::vector<NavigationViewEntry>{next_tracking}:!replace||tracking.back()!=next_tracking};
        Guard guard{changing};
        if(root){
            std::vector<NavigationEntry> new_entries;new_entries.push_back(std::move(next->entry));std::vector<NavigationViewEntry> new_tracking;new_tracking.push_back(std::move(next_tracking));std::vector<std::size_t> new_costs{next->bytes};
            entries.swap(new_entries);tracking.swap(new_tracking);costs.swap(new_costs);bytes=*total;
            if(auto r=notify(change);!r)return std::unexpected(r.error());return true;
        }
        if(replace){entries.back()=std::move(next->entry);tracking.back()=std::move(next_tracking);costs.back()=next->bytes;}
        else{
            const auto needed=entries.size()+1;
            const auto reserve=[&](auto& values){if(values.capacity()<needed)values.reserve(std::min(NavigationLimits::depth,std::max(needed,values.capacity()?values.capacity()*2:std::size_t(8))));};
            reserve(entries);reserve(tracking);reserve(costs);entries.push_back(std::move(next->entry));tracking.push_back(std::move(next_tracking));costs.push_back(next->bytes);
        }
        bytes=*total;if(auto r=notify(change);!r)return std::unexpected(r.error());return true;
    }
    core::Result<bool> remove_to(std::size_t count){
        if(entries.size()<=count)return false;
        const bool changed=current_differs(current(),count?&entries[count-1]:nullptr);Guard guard{changing};
        for(std::size_t i=count;i<costs.size();++i)bytes-=costs[i];entries.resize(count);tracking.resize(count);costs.resize(count);
        if(auto r=notify({.path=true,.current=changed,.tracking=true});!r)return std::unexpected(r.error());return true;
    }
};
NavigationStackModel::NavigationStackModel():impl_(std::make_shared<Impl>()){}
NavigationStackModel::~NavigationStackModel()=default;
core::Result<NavigationState> NavigationStackModel::state()const{
    auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());NavigationState result{.revision=p->revision,.depth=p->entries.size()};if(auto current=p->current()){result.current_path=current->path;result.current_params=current->params;}return result;
}
core::Result<std::vector<NavigationEntry>> NavigationStackModel::path()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->entries;}
core::Result<std::span<const NavigationEntry>> NavigationStackModel::path_view()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return std::span<const NavigationEntry>{p->entries};}
core::Result<bool> NavigationStackModel::set_path(std::vector<NavigationEntry> next){
    auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(next.size()>NavigationLimits::depth)return core::fail(core::ErrorCode::full,"Navigation history depth budget exceeded");
    std::vector<NavigationViewEntry> tracking;std::vector<std::size_t> costs;tracking.reserve(next.size());costs.reserve(next.size());std::size_t bytes=0;
    for(std::size_t i=0;i<next.size();++i){auto entry=prepare(std::move(next[i]));if(!entry)return std::unexpected(entry.error());if(entry->bytes>NavigationLimits::total_bytes-bytes)return core::fail(core::ErrorCode::full,"Navigation history byte budget exceeded");bytes+=entry->bytes;costs.push_back(entry->bytes);tracking.push_back(track(entry->entry,i));next[i]=std::move(entry->entry);}
    if(next==p->entries)return false;NavigationChange change{.path=true,.current=current_differs(p->current(),next.empty()?nullptr:&next.back()),.tracking=tracking!=p->tracking};Guard guard{p->changing};
    p->entries.swap(next);p->tracking.swap(tracking);p->costs.swap(costs);p->bytes=bytes;
    if(auto r=p->notify(change);!r)return std::unexpected(r.error());return true;
}
core::Result<NavigationEntry> NavigationStackModel::create_path_entry(std::string_view path,Value params)const{
    auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());if(path.size()>RouteLimits::path_bytes)return core::fail(core::ErrorCode::full,"Navigation path exceeds its budget");auto entry=prepare({.path=std::string(path),.params=std::move(params)});if(!entry)return std::unexpected(entry.error());return std::move(entry->entry);
}
core::Result<std::string> NavigationStackModel::normalize_path(std::string_view path)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return RouteMatcher::normalize_path(path);}
core::Result<NavigationEntry> NavigationStackModel::create_component_path_entry(std::shared_ptr<const void> component,Value params)const{
    auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());auto entry=prepare({.params=std::move(params),.component=std::move(component)});if(!entry)return std::unexpected(entry.error());return std::move(entry->entry);
}
core::Result<std::vector<NavigationEntry>> NavigationStackModel::stack_after_path_operation(std::string_view path,Value params,NavigationOperation operation)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());if(path.size()>RouteLimits::path_bytes)return core::fail(core::ErrorCode::full,"Navigation path exceeds its budget");return p->preview({.path=std::string(path),.params=std::move(params)},operation);}
core::Result<std::vector<NavigationEntry>> NavigationStackModel::stack_after_component_operation(std::shared_ptr<const void> component,Value params,NavigationOperation operation)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->preview({.params=std::move(params),.component=std::move(component)},operation);}
core::Result<std::vector<NavigationEntry>> NavigationStackModel::stack_after_pop()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());auto result=p->entries;if(result.size()>1)result.pop_back();return result;}
core::Result<std::vector<NavigationEntry>> NavigationStackModel::stack_after_pop_to_root()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->entries.empty()?std::vector<NavigationEntry>{}:std::vector<NavigationEntry>{p->entries.front()};}
core::Result<void> NavigationStackModel::validate_path_operation(std::string_view path,Value params,NavigationOperation operation)const{
    auto p=impl_;if(auto r=p->ready();!r)return r;if(path.size()>RouteLimits::path_bytes)return core::fail(core::ErrorCode::full,"Navigation path exceeds its budget");
    auto next=prepare({.path=std::string(path),.params=std::move(params)});if(!next)return std::unexpected(next.error());auto total=p->operation_bytes(next->bytes,operation);if(!total)return std::unexpected(total.error());return {};
}
core::Result<void> NavigationStackModel::validate_component_operation(std::shared_ptr<const void> component,Value params,NavigationOperation operation)const{
    auto p=impl_;if(auto r=p->ready();!r)return r;auto next=prepare({.params=std::move(params),.component=std::move(component)});if(!next)return std::unexpected(next.error());auto total=p->operation_bytes(next->bytes,operation);if(!total)return std::unexpected(total.error());return {};
}
core::Result<bool> NavigationStackModel::apply_path_operation(std::string_view path,Value params,NavigationOperation operation){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(path.size()>RouteLimits::path_bytes)return core::fail(core::ErrorCode::full,"Navigation path exceeds its budget");return p->apply({.path=std::string(path),.params=std::move(params)},operation);}
core::Result<bool> NavigationStackModel::apply_component_operation(std::shared_ptr<const void> component,Value params,NavigationOperation operation){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());return p->apply({.params=std::move(params),.component=std::move(component)},operation);}
core::Result<bool> NavigationStackModel::pop(){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->entries.size()<=1)return false;return p->remove_to(p->entries.size()-1);}
core::Result<bool> NavigationStackModel::pop_to_root(){auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());return p->remove_to(p->entries.empty()?0:1);}
core::Result<NavigationEntry> NavigationStackModel::current_entry_descriptor()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->current()?*p->current():NavigationEntry{};}
core::Result<NavigationViewEntry> NavigationStackModel::create_view_tracking_entry(const NavigationEntry& entry,std::size_t index)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());auto normalized=prepare(entry);if(!normalized)return std::unexpected(normalized.error());return track(normalized->entry,index);}
core::Result<std::vector<NavigationViewEntry>> NavigationStackModel::build_view_tracking_entries(std::span<const NavigationEntry> entries)const{
    auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());if(entries.size()>NavigationLimits::depth)return core::fail(core::ErrorCode::full,"Too many navigation tracking entries");
    std::vector<NavigationViewEntry> result;result.reserve(entries.size());std::size_t bytes=0;for(std::size_t i=0;i<entries.size();++i){auto normalized=prepare(entries[i]);if(!normalized)return std::unexpected(normalized.error());if(normalized->bytes>NavigationLimits::total_bytes-bytes)return core::fail(core::ErrorCode::full,"Tracking entry byte budget exceeded");bytes+=normalized->bytes;result.push_back(track(normalized->entry,i));}return result;
}
core::Result<std::vector<NavigationViewEntry>> NavigationStackModel::view_tracking_entries()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->tracking;}
core::Result<std::vector<NavigationViewEntry>> NavigationStackModel::build_view_tracking_entries()const{return view_tracking_entries();}
core::Result<std::vector<std::string>> NavigationStackModel::tracked_view_ids()const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->tracked_ids;}
core::Result<std::vector<std::string>> NavigationStackModel::update_tracked_view_ids(std::span<const NavigationViewEntry> entries){
    auto p=impl_;if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(entries.size()>NavigationLimits::depth)return core::fail(core::ErrorCode::full,"Too many tracked view entries");
    std::unordered_set<std::string> seen;std::vector<std::string> next;std::size_t bytes=0;
    for(const auto& entry:entries){const auto& id=entry.view_id;if(id.empty())continue;if(id.size()>NavigationLimits::view_id_bytes||id.find('\0')!=id.npos||!core::valid_utf8(id))return core::fail(core::ErrorCode::invalid_argument,"Invalid tracked view id");
        if(seen.insert(id).second){if(id.size()>NavigationLimits::total_bytes-bytes)return core::fail(core::ErrorCode::full,"Tracked id byte budget exceeded");bytes+=id.size();next.push_back(id);}}
    std::vector<std::string> removed;for(const auto& id:p->tracked_ids)if(!seen.contains(id))removed.push_back(id);
    if(next==p->tracked_ids)return removed;Guard guard{p->changing};p->tracked_ids.swap(next);if(auto r=p->notify({.tracked_ids=true});!r)return std::unexpected(r.error());return removed;
}
core::Result<void> NavigationStackModel::observe(std::weak_ptr<NavigationChangePort> receiver){
    auto p=impl_;if(auto r=p->ready(true);!r)return r;auto incoming=receiver.lock();if(!incoming)return core::fail(core::ErrorCode::invalid_argument,"Navigation receiver already expired");
    std::erase_if(p->receivers,[](const auto& port){return port.expired();});for(const auto& port:p->receivers)if(port.lock()==incoming)return {};p->receivers.push_back(std::move(receiver));return {};
}
}
