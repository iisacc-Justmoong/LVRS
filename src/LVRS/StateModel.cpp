#include "StateModel.h"
#include "ViewModelDetail.h"
#include <set>
namespace lvrs {
using ii::core::Result;using ii::core::Value;namespace pd=presentation_detail;
struct StateModel::Impl final:pd::Signals<StateChange> {
    Value::Object values;
    std::uint64_t revision=0;
    ii::core::Observable<std::uint64_t> revision_epoch{0},values_epoch{0},keys_epoch{0};
    std::shared_ptr<ViewModel::Impl> base;
    explicit Impl(std::shared_ptr<ViewModel::Impl> b):base(std::move(b)){}
    Result<void> ready()const{if(auto r=writable();!r)return r;return base->writable();}
    Result<bool> commit(Value::Object next){
        if(values==next)return false;
        if(revision==UINT64_MAX)return ii::core::fail(ii::core::ErrorCode::full,"State revision exhausted");
        const auto before_keys=pd::ordered_keys(values),after_keys=pd::ordered_keys(next);
        std::vector<StateChange> changes;changes.reserve(values.size()+next.size()+3);
        Value::Object combined=values;for(const auto& [key,value]:next)combined.insert_or_assign(key,value);
        const auto next_revision=revision+1;
        changes.push_back({.event=StateEvent::revision,.revision=next_revision});
        changes.push_back({.event=StateEvent::values,.revision=next_revision});
        if(before_keys!=after_keys)changes.push_back({.event=StateEvent::keys,.revision=next_revision});
        for(const auto& key:pd::ordered_keys(combined)){
            const auto old=values.find(key),current=next.find(key);
            const bool was=old!=values.end(),present=current!=next.end();
            if(was==present&&(!was||old->second==current->second))continue;
            changes.push_back({.event=StateEvent::value,.revision=next_revision,.key=key,.value=present?current->second:Value{},.previous=was?old->second:Value{},.present=present,.previously_present=was});
        }
        values=std::move(next);revision=next_revision;
        pd::Flag base_guard{base->changing},notify{notifying};std::optional<ii::core::Error> failed;
        for(const auto& change:changes){
            if(closed)break;
            if(change.event==StateEvent::revision)tick(revision_epoch,failed);
            else if(change.event==StateEvent::values)tick(values_epoch,failed);
            else if(change.event==StateEvent::keys)tick(keys_epoch,failed);
            send(change,failed);
        }
        if(auto result=base->descriptor_signal.notify();!result&&!failed)failed=std::move(result.error());
        if(failed)return std::unexpected(*failed);return true;
    }
    void close()noexcept{closed=true;receivers.clear();try{revision_epoch.set(revision_epoch.get()+1);}catch(...) {}}
};
StateModel::StateModel():state_(std::make_shared<Impl>(impl_)){}
StateModel::~StateModel(){auto m=state_;m->close();}
Value::Object StateModel::values()const{auto m=state_;m->readable();(void)m->values_epoch.get();return m->values;}
std::vector<std::string> StateModel::state_keys()const{auto m=state_;m->readable();(void)m->keys_epoch.get();return pd::ordered_keys(m->values);}
std::uint64_t StateModel::revision()const{auto m=state_;m->readable();(void)m->revision_epoch.get();return m->revision;}
bool StateModel::empty()const{auto m=state_;m->readable();(void)m->values_epoch.get();return m->values.empty();}
Value StateModel::value(std::string_view key,Value fallback)const{return value_or(key,std::move(fallback));}
Value StateModel::value_or(std::string_view key,Value fallback)const{auto m=state_;m->readable();(void)m->values_epoch.get();const auto normalized=pd::normalized_text(key);auto found=m->values.find(normalized);return normalized.empty()||found==m->values.end()?std::move(fallback):found->second;}
bool StateModel::has_value(std::string_view key)const{auto m=state_;m->readable();(void)m->values_epoch.get();auto normalized=pd::normalized_text(key);return !normalized.empty()&&m->values.contains(normalized);}
Value::Object StateModel::state_snapshot()const{auto m=state_;auto base=m->base;m->readable();auto result=base->snapshot();(void)m->values_epoch.get();(void)m->keys_epoch.get();(void)m->revision_epoch.get();Value::Array keys;for(const auto& key:pd::ordered_keys(m->values))keys.emplace_back(key);
    result.insert_or_assign("values",Value(m->values));result.insert_or_assign("stateKeys",Value(std::move(keys)));result.insert_or_assign("revision",Value(m->revision));result.insert_or_assign("empty",Value(m->values.empty()));return result;}
Result<bool> StateModel::set_values(Value::Object values){auto m=state_;if(auto r=m->ready();!r)return std::unexpected(r.error());return pd::attempt([&]()->Result<bool>{auto next=pd::normalized_values(values);pd::Flag guard{m->changing};return m->commit(std::move(next));});}
Result<bool> StateModel::set_value(std::string key,Value value){auto m=state_;if(auto r=m->ready();!r)return std::unexpected(r.error());return pd::attempt([&]()->Result<bool>{auto normalized=pd::normalized_text(key);pd::Flag guard{m->changing};
    const std::string error=normalized.empty()?"Empty state key":"";std::optional<ii::core::Error> failed;
    if(auto r=m->base->assign_error(error);!r){if(m->base->error.get()!=error)return std::unexpected(r.error());failed=r.error();}
    if(m->closed)return ii::core::fail(ii::core::ErrorCode::closed,"State owner retired during error notification");
    if(normalized.empty()){if(failed)return std::unexpected(*failed);return false;}auto next=m->values;next.insert_or_assign(std::move(normalized),std::move(value));auto changed=m->commit(std::move(next));if(!changed)return std::unexpected(changed.error());if(failed)return std::unexpected(*failed);return true;});}
Result<bool> StateModel::remove_value(std::string key){auto m=state_;if(auto r=m->ready();!r)return std::unexpected(r.error());return pd::attempt([&]()->Result<bool>{auto normalized=pd::normalized_text(key);pd::Flag guard{m->changing};
    const std::string error=normalized.empty()?"Empty state key":"";std::optional<ii::core::Error> failed;
    if(auto r=m->base->assign_error(error);!r){if(m->base->error.get()!=error)return std::unexpected(r.error());failed=r.error();}
    if(m->closed)return ii::core::fail(ii::core::ErrorCode::closed,"State owner retired during error notification");
    if(normalized.empty()||!m->values.contains(normalized)){if(failed)return std::unexpected(*failed);return false;}auto next=m->values;next.erase(normalized);auto changed=m->commit(std::move(next));if(!changed)return std::unexpected(changed.error());if(failed)return std::unexpected(*failed);return *changed;});}
Result<bool> StateModel::apply_patch(Value::Object patch){auto m=state_;if(auto r=m->ready();!r)return std::unexpected(r.error());return pd::attempt([&]()->Result<bool>{auto normalized=pd::normalized_values(patch);if(normalized.empty())return false;auto next=m->values;for(auto& [key,value]:normalized)next.insert_or_assign(key,std::move(value));pd::Flag guard{m->changing};auto changed=m->commit(std::move(next));if(!changed)return std::unexpected(changed.error());return true;});}
Result<bool> StateModel::clear_values(){auto m=state_;if(auto r=m->ready();!r)return std::unexpected(r.error());pd::Flag guard{m->changing};return pd::attempt([&]{return m->commit({});});}
Result<void> StateModel::observe(std::shared_ptr<const StateChangePort> port){auto m=state_;return m->observe(std::move(port));}
std::optional<Value> StateModel::read_property(std::string_view name)const{
    if(name=="values")return Value(values());
    if(name=="stateKeys"){Value::Array keys;for(auto& key:state_keys())keys.emplace_back(std::move(key));return Value(std::move(keys));}
    if(name=="revision")return Value(revision());
    if(name=="empty")return Value(empty());
    return ViewModel::read_property(name);
}
Result<bool> StateModel::write_property(std::string_view name,Value value){
    auto m=state_;if(auto ready=m->ready();!ready)return std::unexpected(ready.error());
    if(name=="values"){
        auto object=value.get<Value::Object>();if(!object)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"StateModel values require Object");
        auto result=set_values(object->get());if(!result)return std::unexpected(result.error());return true;
    }
    return ViewModel::write_property(name,std::move(value));
}
Value::Object StateModel::model_descriptor()const{
    auto result=ViewModel::model_descriptor();result.insert_or_assign("className",Value("StateModel"));result.insert_or_assign("stateModel",Value(true));
    result.emplace("values",Value(values()));result.emplace("stateKeys",*read_property("stateKeys"));result.emplace("revision",Value(revision()));result.emplace("empty",Value(empty()));return result;
}
ii::core::ObservableSubscription StateModel::observe_revision(std::function<void()> changed)const{auto m=state_;m->readable();if(!changed)throw std::invalid_argument("State revision observation requires a callback");return m->revision_epoch.observe([changed=std::move(changed)](auto,auto){changed();});}
}
