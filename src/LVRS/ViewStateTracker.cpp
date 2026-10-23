#include "ViewStateTracker.h"
#include "TrackingDetail.h"
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace lvrs {
using namespace ii;
namespace {
constexpr auto absent=std::size_t(-1);
core::Value record_value(const ViewRecord& record){
    return core::Value{core::Value::Object{
        {"viewId",core::Value{record.view_id}},{"path",core::Value{record.path}},
        {"state",core::Value{view_state_name(record.state)}},{"stateCode",core::Value{std::int64_t(record.state)}},
        {"index",core::Value{std::int64_t(record.index)}},{"loaded",core::Value{true}},
        {"enabled",core::Value{record.state!=ViewState::disabled}},{"active",core::Value{record.state==ViewState::active}}
    }};
}
core::Value strings(std::vector<std::string> values){core::Value::Array result;result.reserve(values.size());for(auto& value:values)result.emplace_back(std::move(value));return core::Value{std::move(result)};}
}
std::string_view view_state_name(ViewState state)noexcept{
    switch(state){case ViewState::active:return "Active";case ViewState::inactive:return "Inactive";case ViewState::disabled:return "Disabled";}return "Disabled";
}
struct ViewStateTracker::Impl final {
    tracking_detail::Owner owner;
    tracking_detail::Receivers<ViewStatePort> receivers;
    std::vector<ViewRecord> records;
    std::unordered_map<std::string,std::vector<std::size_t>> indices;
    std::unordered_set<std::string> overrides;
    std::set<std::size_t> enabled;
    std::size_t disabled_count=0,override_bytes=0;
    std::size_t active()const{return enabled.empty()?absent:*enabled.rbegin();}
    ViewStateSummary summary()const{
        const auto index=active();const auto count=index==absent?std::size_t(0):std::size_t(1);
        return {.revision=owner.revision,.loaded_count=records.size(),.active_count=count,
            .inactive_count=records.size()-disabled_count-count,.disabled_count=disabled_count,
            .current_active_view=index==absent?std::string{}:records[index].view_id};
    }
    core::Result<void> publish(){++owner.revision;return receivers.notify(owner,summary());}
    template<class Input>core::Result<bool> sync(std::span<const Input> inputs){
        if(auto result=owner.ready(true);!result)return std::unexpected(result.error());
        if(inputs.size()>NavigationLimits::depth)return core::fail(core::ErrorCode::full,"View tracking stack depth exceeded");
        std::vector<ViewRecord> next;next.reserve(inputs.size());std::size_t anonymous=0,bytes=0,disabled=0,last=absent;
        for(const auto& input:inputs){
            auto id=tracking_detail::text(input.view_id),path=tracking_detail::text(input.path);
            if(!id)return std::unexpected(id.error());if(!path)return std::unexpected(path.error());
            if(id->empty())*id=path->empty()?"_component_"+std::to_string(anonymous++):*path;
            const auto cost=id->size()+path->size()+sizeof(ViewRecord);
            if(cost>NavigationLimits::total_bytes-bytes)return core::fail(core::ErrorCode::full,"View tracking stack byte budget exceeded");bytes+=cost;
            bool base_enabled=input.enabled;if constexpr(std::same_as<Input,ViewStackInput>)base_enabled&=!input.disabled;
            const bool available=base_enabled&&!overrides.contains(*id);
            const auto index=next.size();next.push_back({std::move(*id),std::move(*path),index,base_enabled,available?ViewState::inactive:ViewState::disabled});
            if(available)last=index;else ++disabled;
        }
        if(last!=absent)next[last].state=ViewState::active;
        if(next==records)return false;if(auto result=owner.can_publish();!result)return std::unexpected(result.error());
        std::unordered_map<std::string,std::vector<std::size_t>> next_indices;std::set<std::size_t> next_enabled;
        next_indices.reserve(next.size());for(const auto& record:next){next_indices[record.view_id].push_back(record.index);if(record.state!=ViewState::disabled)next_enabled.insert(record.index);}
        records.swap(next);indices.swap(next_indices);enabled.swap(next_enabled);disabled_count=disabled;
        if(auto result=publish();!result)return std::unexpected(result.error());return true;
    }
    std::vector<std::string> partition(std::optional<ViewState> state)const{
        std::vector<std::string> result;if(!state)result.reserve(records.size());
        for(const auto& record:records)if(!state||record.state==*state)result.push_back(record.view_id);return result;
    }
    core::Result<const ViewRecord*> find_top(std::string_view raw)const{
        if(auto result=owner.ready();!result)return std::unexpected(result.error());
        auto id=tracking_detail::text(raw);if(!id)return std::unexpected(id.error());
        auto found=indices.find(*id);return found==indices.end()?nullptr:&records[found->second.back()];
    }
};
ViewStateTracker::ViewStateTracker():impl_(std::make_shared<Impl>()){}
ViewStateTracker::~ViewStateTracker()=default;
std::shared_ptr<ViewStateTracker> ViewStateTracker::current(){thread_local auto model=std::make_shared<ViewStateTracker>();return model;}
core::Result<bool> ViewStateTracker::sync_stack(std::span<const ViewStackInput> inputs){auto p=impl_;return p->sync(inputs);}
core::Result<bool> ViewStateTracker::sync_navigation(std::span<const NavigationViewEntry> inputs){auto p=impl_;return p->sync(inputs);}
core::Result<bool> ViewStateTracker::sync_value(const core::Value& value){
    auto p=impl_;if(auto result=p->owner.ready(true);!result)return std::unexpected(result.error());
    const auto* array=std::get_if<core::Value::Array>(&value.storage());if(!array)return core::fail(core::ErrorCode::invalid_argument,"View tracking input requires a Core Value array");
    if(array->size()>NavigationLimits::depth)return core::fail(core::ErrorCode::full,"View tracking input depth exceeded");
    std::vector<ViewStackInput> inputs;inputs.reserve(array->size());std::size_t bytes=0;
    for(const auto& entry:*array){
        const auto* object=std::get_if<core::Value::Object>(&entry.storage());if(!object)return core::fail(core::ErrorCode::invalid_argument,"Each view tracking entry requires an object");
        ViewStackInput input;
        for(auto [key,target]:{std::pair{"viewId",&input.view_id},std::pair{"path",&input.path}}){
            auto found=object->find(key);if(found==object->end())continue;
            const auto* text=std::get_if<std::string>(&found->second.storage());if(!text)return core::fail(core::ErrorCode::invalid_argument,"View id and path require typed strings");
            auto normalized=tracking_detail::text(*text);if(!normalized)return std::unexpected(normalized.error());
            if(normalized->size()>NavigationLimits::total_bytes-bytes)return core::fail(core::ErrorCode::full,"View tracking input byte budget exceeded");bytes+=normalized->size();*target=std::move(*normalized);
        }
        for(auto [key,target]:{std::pair{"enabled",&input.enabled},std::pair{"disabled",&input.disabled}}){
            auto found=object->find(key);if(found==object->end())continue;
            const auto* boolean=std::get_if<bool>(&found->second.storage());if(!boolean)return core::fail(core::ErrorCode::invalid_argument,"View enable policy requires typed booleans");*target=*boolean;
        }
        inputs.push_back(std::move(input));
    }
    return p->sync(std::span<const ViewStackInput>{inputs});
}
core::Result<bool> ViewStateTracker::set_view_disabled(std::string_view raw,bool disabled){
    auto p=impl_;if(auto result=p->owner.ready(true);!result)return std::unexpected(result.error());
    auto id=tracking_detail::text(raw);if(!id)return std::unexpected(id.error());if(id->empty()||p->overrides.contains(*id)==disabled)return false;
    const auto found=p->indices.find(*id);const auto previous=p->active();std::set<std::size_t> admitted;
    bool changed=false;
    if(found!=p->indices.end())for(auto index:found->second)if(p->records[index].base_enabled){changed=true;if(!disabled)admitted.insert(index);}
    if(changed)if(auto result=p->owner.can_publish();!result)return std::unexpected(result.error());
    if(disabled){
        if(p->overrides.size()==NavigationLimits::depth||id->size()>NavigationLimits::total_bytes-p->override_bytes)return core::fail(core::ErrorCode::full,"View disabled override budget exceeded");
        p->overrides.insert(*id);p->override_bytes+=id->size();
        if(found!=p->indices.end())for(auto index:found->second)if(p->records[index].base_enabled){p->enabled.erase(index);p->records[index].state=ViewState::disabled;++p->disabled_count;}
    }else{
        p->overrides.erase(*id);p->override_bytes-=id->size();p->enabled.merge(admitted);
        if(found!=p->indices.end())for(auto index:found->second)if(p->records[index].base_enabled){p->records[index].state=ViewState::inactive;--p->disabled_count;}
    }
    if(!changed)return false;
    const auto current=p->active();if(previous!=absent&&p->records[previous].state!=ViewState::disabled)p->records[previous].state=ViewState::inactive;
    if(current!=absent)p->records[current].state=ViewState::active;
    if(auto result=p->publish();!result)return std::unexpected(result.error());return true;
}
core::Result<ViewStateSummary> ViewStateTracker::state()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return p->summary();}
core::Result<std::vector<ViewRecord>> ViewStateTracker::stack()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return p->records;}
core::Result<std::span<const ViewRecord>> ViewStateTracker::stack_view()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return std::span<const ViewRecord>{p->records};}
core::Result<std::vector<std::string>> ViewStateTracker::loaded_views()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return p->partition({});}
core::Result<std::vector<std::string>> ViewStateTracker::active_views()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return p->partition(ViewState::active);}
core::Result<std::vector<std::string>> ViewStateTracker::inactive_views()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return p->partition(ViewState::inactive);}
core::Result<std::vector<std::string>> ViewStateTracker::disabled_views()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return p->partition(ViewState::disabled);}
core::Result<std::optional<ViewRecord>> ViewStateTracker::view(std::string_view raw)const{
    auto p=impl_;auto record=p->find_top(raw);if(!record)return std::unexpected(record.error());return *record?std::optional<ViewRecord>{**record}:std::optional<ViewRecord>{};
}
core::Result<bool> ViewStateTracker::is_loaded(std::string_view id)const{auto p=impl_;auto record=p->find_top(id);if(!record)return std::unexpected(record.error());return *record!=nullptr;}
core::Result<std::string> ViewStateTracker::state_of(std::string_view id)const{auto p=impl_;auto record=p->find_top(id);if(!record)return std::unexpected(record.error());return *record?std::string(view_state_name((*record)->state)):std::string{};}
core::Result<ViewStateSnapshot> ViewStateTracker::snapshot()const{
    auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());ViewStateSnapshot result{.state=p->summary(),.stack=p->records};
    result.loaded_views.reserve(p->records.size());result.active_views.reserve(result.state.active_count);result.inactive_views.reserve(result.state.inactive_count);result.disabled_views.reserve(result.state.disabled_count);
    for(const auto& record:p->records){result.loaded_views.push_back(record.view_id);(record.state==ViewState::active?result.active_views:record.state==ViewState::inactive?result.inactive_views:result.disabled_views).push_back(record.view_id);}return result;
}
core::Result<core::Value> ViewStateTracker::snapshot_value()const{
    auto state=snapshot();if(!state)return std::unexpected(state.error());core::Value::Array stack;stack.reserve(state->stack.size());for(const auto& record:state->stack)stack.push_back(record_value(record));
    return core::Value{core::Value::Object{
        {"currentActiveView",core::Value{state->state.current_active_view}},
        {"loadedViews",strings(std::move(state->loaded_views))},{"activeViews",strings(std::move(state->active_views))},
        {"inactiveViews",strings(std::move(state->inactive_views))},{"disabledViews",strings(std::move(state->disabled_views))},{"stack",core::Value{std::move(stack)}}
    }};
}
core::Result<bool> ViewStateTracker::clear(){
    auto p=impl_;if(auto result=p->owner.ready(true);!result)return std::unexpected(result.error());if(p->records.empty()&&p->overrides.empty())return false;
    if(auto result=p->owner.can_publish();!result)return std::unexpected(result.error());p->records.clear();p->indices.clear();p->enabled.clear();p->overrides.clear();p->disabled_count=p->override_bytes=0;
    if(auto result=p->publish();!result)return std::unexpected(result.error());return true;
}
core::Result<void> ViewStateTracker::observe(std::weak_ptr<ViewStatePort> receiver){auto p=impl_;if(auto result=p->owner.ready(true);!result)return result;return p->receivers.observe(std::move(receiver));}
}
