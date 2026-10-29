#include "ProgressModel.h"
#include "PresentationDetail.h"
#include <array>
#include <cmath>
namespace lvrs {
using ii::core::Result;namespace pd=presentation_detail;
namespace {
double normalized(double value,const ProgressSnapshot& frame){
    if(std::abs(frame.value_range)<.000001)return value>=frame.effective.maximum?1.:0.;
    const double result=(value-frame.effective.minimum)/frame.value_range;
    return std::isfinite(result)?std::clamp(result,0.,1.):0.;
}
double state_number(const std::shared_ptr<StateSource>& state,std::string_view key,double fallback){
    if(!std::isfinite(fallback))fallback=0;
    return state?pd::number(state->value_or(key,ii::core::Value(fallback)),fallback):fallback;
}
ProgressSnapshot compute(const ProgressValues& raw,const ProgressStateKeys& keys,const std::shared_ptr<StateSource>& state){
    ProgressSnapshot result;result.effective={state_number(state,keys.minimum,raw.minimum),state_number(state,keys.maximum,raw.maximum),state_number(state,keys.start,raw.start),state_number(state,keys.current,raw.current)};
    result.value_range=result.effective.maximum-result.effective.minimum;
    result.normalized_start=normalized(result.effective.start,result);result.normalized_current=normalized(result.effective.current,result);
    result.fill_start=std::min(result.normalized_start,result.normalized_current);result.fill_progress=std::abs(result.normalized_current-result.normalized_start);return result;
}
ProgressStateKeys normalize(ProgressStateKeys keys){
    const ProgressStateKeys defaults;
    auto key=[](std::string value,const std::string& fallback){auto normalized=pd::normalized_text(value);return normalized.empty()?fallback:normalized;};
    return {key(std::move(keys.minimum),defaults.minimum),key(std::move(keys.maximum),defaults.maximum),key(std::move(keys.start),defaults.start),key(std::move(keys.current),defaults.current)};
}
}
struct ProgressModel::Impl final:pd::Signals<ProgressChange>,std::enable_shared_from_this<Impl> {
    ProgressValues values;ProgressStateKeys keys;ProgressSnapshot computed;
    std::weak_ptr<StateSource> source;
    ii::core::ObservableSubscription source_watch;
    ii::core::Observable<std::uint64_t> values_epoch{0},keys_epoch{0},source_epoch{0},revision_epoch{0},effective_epoch{0};
    std::uint64_t state_revision=0;
    bool source_alive=false,source_pending=false,draining_source=false;
    Result<void> publish(std::span<const ProgressEvent> events){
        std::optional<ii::core::Error> failed;
        {pd::Flag guard{notifying};for(auto event:events){
            if(closed)break;
            auto& epoch=event==ProgressEvent::values?values_epoch:event==ProgressEvent::keys?keys_epoch:event==ProgressEvent::state_model?source_epoch:event==ProgressEvent::revision?revision_epoch:effective_epoch;
            tick(epoch,failed);send({event,state_revision},failed);
        }}
        if(!draining_source){pd::Flag drain_guard{draining_source};for(std::size_t turns=0;source_pending&&!closed;++turns){
            source_pending=false;if(turns==256){if(!failed)failed=ii::core::Error{ii::core::ErrorCode::full,"Progress source notification cycle exceeded its bound"};break;}
            auto result=refresh_source();if(!result&&!failed)failed=result.error();
        }}
        if(failed)return std::unexpected(*failed);return {};
    }
    Result<void> refresh_source(){
        if(closed)return {};
        return pd::attempt([&]()->Result<void>{
            if(state_revision==UINT64_MAX)return ii::core::fail(ii::core::ErrorCode::full,"Progress state revision exhausted");
            auto state=source.lock();auto next=compute(values,keys,state);const bool changed=source_alive!=bool(state);
            computed=next;source_alive=bool(state);++state_revision;
            if(changed){const std::array events{ProgressEvent::state_model,ProgressEvent::revision,ProgressEvent::effective};return publish(events);}
            const std::array events{ProgressEvent::revision,ProgressEvent::effective};return publish(events);
        });
    }
    void source_changed(){
        if(closed)return;
        if(changing||notifying){source_pending=true;return;}
        if(auto result=refresh_source();!result)throw std::runtime_error(result.error().message);
    }
    Result<bool> change_values(ProgressValues next){
        if(auto ready=writable();!ready)return std::unexpected(ready.error());
        if(pd::fuzzy_equal(values.minimum,next.minimum)&&pd::fuzzy_equal(values.maximum,next.maximum)&&pd::fuzzy_equal(values.start,next.start)&&pd::fuzzy_equal(values.current,next.current))return false;
        return pd::attempt([&]()->Result<bool>{auto candidate=compute(next,keys,source.lock());values=next;computed=candidate;
            pd::Flag guard{changing};const std::array events{ProgressEvent::values,ProgressEvent::effective};auto result=publish(events);if(!result)return std::unexpected(result.error());return true;});
    }
    Result<bool> change_keys(ProgressStateKeys next){
        if(auto ready=writable();!ready)return std::unexpected(ready.error());
        return pd::attempt([&]()->Result<bool>{next=normalize(std::move(next));if(keys==next)return false;auto candidate=compute(values,next,source.lock());keys=std::move(next);computed=candidate;
            pd::Flag guard{changing};const std::array events{ProgressEvent::keys,ProgressEvent::effective};auto result=publish(events);if(!result)return std::unexpected(result.error());return true;});
    }
};
ProgressModel::ProgressModel():impl_(std::make_shared<Impl>()){}
ProgressModel::~ProgressModel(){auto m=impl_;m->closed=true;m->source_watch={};m->receivers.clear();}
double ProgressModel::minimum_value()const{auto m=impl_;m->readable();(void)m->values_epoch.get();return m->values.minimum;}
double ProgressModel::maximum_value()const{auto m=impl_;m->readable();(void)m->values_epoch.get();return m->values.maximum;}
double ProgressModel::start_value()const{auto m=impl_;m->readable();(void)m->values_epoch.get();return m->values.start;}
double ProgressModel::current_value()const{auto m=impl_;m->readable();(void)m->values_epoch.get();return m->values.current;}
Result<bool> ProgressModel::set_values(ProgressValues value){auto m=impl_;return m->change_values(value);}
Result<bool> ProgressModel::set_configuration(ProgressValues values,ProgressStateKeys keys,std::shared_ptr<StateSource> source){
    auto m=impl_;if(auto ready=m->writable();!ready)return std::unexpected(ready.error());
    return pd::attempt([&]()->Result<bool>{
        keys=normalize(std::move(keys));
        auto keep=[](double old,double next){return pd::fuzzy_equal(old,next)?old:next;};
        const bool value_change=!(pd::fuzzy_equal(m->values.minimum,values.minimum)&&pd::fuzzy_equal(m->values.maximum,values.maximum)&&pd::fuzzy_equal(m->values.start,values.start)&&pd::fuzzy_equal(m->values.current,values.current));
        values={keep(m->values.minimum,values.minimum),keep(m->values.maximum,values.maximum),keep(m->values.start,values.start),keep(m->values.current,values.current)};
        const bool key_change=m->keys!=keys,source_change=m->source.lock()!=source;
        if(!value_change&&!key_change&&!source_change)return false;
        if(source_change&&m->state_revision==UINT64_MAX)return ii::core::fail(ii::core::ErrorCode::full,"Progress state revision exhausted");
        auto candidate=compute(values,keys,source);ii::core::ObservableSubscription watch;
        if(source_change&&source)watch=source->observe_revision([weak=std::weak_ptr(m)]{if(auto model=weak.lock())model->source_changed();});
        std::vector<ProgressEvent> events;events.reserve(5);
        if(value_change)events.push_back(ProgressEvent::values);if(key_change)events.push_back(ProgressEvent::keys);
        if(source_change){events.push_back(ProgressEvent::state_model);events.push_back(ProgressEvent::revision);}
        events.push_back(ProgressEvent::effective);
        m->values=values;m->keys=std::move(keys);m->computed=candidate;
        if(source_change){m->source=source;m->source_watch=std::move(watch);m->source_alive=bool(source);++m->state_revision;}
        pd::Flag guard{m->changing};auto result=m->publish(events);if(!result)return std::unexpected(result.error());return true;
    });
}
Result<bool> ProgressModel::set_minimum_value(double value){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());auto next=m->values;next.minimum=value;return m->change_values(next);}
Result<bool> ProgressModel::set_maximum_value(double value){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());auto next=m->values;next.maximum=value;return m->change_values(next);}
Result<bool> ProgressModel::set_start_value(double value){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());auto next=m->values;next.start=value;return m->change_values(next);}
Result<bool> ProgressModel::set_current_value(double value){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());auto next=m->values;next.current=value;return m->change_values(next);}
std::shared_ptr<StateSource> ProgressModel::state_model()const{auto m=impl_;m->readable();(void)m->source_epoch.get();return m->source.lock();}
Result<bool> ProgressModel::set_state_model(std::shared_ptr<StateSource> source){auto m=impl_;if(auto ready=m->writable();!ready)return std::unexpected(ready.error());
    if(m->source.lock()==source)return false;
    return pd::attempt([&]()->Result<bool>{
        if(m->state_revision==UINT64_MAX)return ii::core::fail(ii::core::ErrorCode::full,"Progress state revision exhausted");
        auto candidate=compute(m->values,m->keys,source);ii::core::ObservableSubscription watch;
        if(source)watch=source->observe_revision([weak=std::weak_ptr(m)]{if(auto model=weak.lock())model->source_changed();});
        m->source=source;m->source_watch=std::move(watch);m->source_alive=bool(source);m->computed=candidate;++m->state_revision;
        pd::Flag guard{m->changing};const std::array events{ProgressEvent::state_model,ProgressEvent::revision,ProgressEvent::effective};auto result=m->publish(events);if(!result)return std::unexpected(result.error());return true;
    });
}
ProgressStateKeys ProgressModel::state_keys()const{auto m=impl_;m->readable();(void)m->keys_epoch.get();return m->keys;}
std::string ProgressModel::minimum_value_state_key()const{return state_keys().minimum;}
std::string ProgressModel::maximum_value_state_key()const{return state_keys().maximum;}
std::string ProgressModel::start_value_state_key()const{return state_keys().start;}
std::string ProgressModel::current_value_state_key()const{return state_keys().current;}
Result<bool> ProgressModel::set_state_keys(ProgressStateKeys keys){auto m=impl_;return m->change_keys(std::move(keys));}
Result<bool> ProgressModel::set_minimum_value_state_key(std::string key){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());auto next=m->keys;next.minimum=std::move(key);return m->change_keys(std::move(next));}
Result<bool> ProgressModel::set_maximum_value_state_key(std::string key){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());auto next=m->keys;next.maximum=std::move(key);return m->change_keys(std::move(next));}
Result<bool> ProgressModel::set_start_value_state_key(std::string key){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());auto next=m->keys;next.start=std::move(key);return m->change_keys(std::move(next));}
Result<bool> ProgressModel::set_current_value_state_key(std::string key){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());auto next=m->keys;next.current=std::move(key);return m->change_keys(std::move(next));}
bool ProgressModel::using_state_model()const{auto m=impl_;m->readable();(void)m->source_epoch.get();return !m->source.expired();}
std::uint64_t ProgressModel::state_revision()const{auto m=impl_;m->readable();(void)m->revision_epoch.get();return m->state_revision;}
ProgressSnapshot ProgressModel::snapshot()const{auto m=impl_;m->readable();(void)m->effective_epoch.get();return m->computed;}
double ProgressModel::effective_minimum_value()const{return snapshot().effective.minimum;}
double ProgressModel::effective_maximum_value()const{return snapshot().effective.maximum;}
double ProgressModel::effective_start_value()const{return snapshot().effective.start;}
double ProgressModel::effective_current_value()const{return snapshot().effective.current;}
double ProgressModel::value_range()const{return snapshot().value_range;}
double ProgressModel::normalized_start()const{return snapshot().normalized_start;}
double ProgressModel::normalized_current()const{return snapshot().normalized_current;}
double ProgressModel::fill_start()const{return snapshot().fill_start;}
double ProgressModel::fill_progress()const{return snapshot().fill_progress;}
double ProgressModel::progress()const{return snapshot().normalized_current;}
double ProgressModel::state_number(std::string_view key,double fallback)const{auto m=impl_;m->readable();(void)m->revision_epoch.get();(void)m->source_epoch.get();return lvrs::state_number(m->source.lock(),key,fallback);}
double ProgressModel::normalized_value(double value)const{return normalized(value,snapshot());}
double ProgressModel::radius_for(int style,double radius,double width,double height)const{auto m=impl_;m->readable();return style==1?std::max(0.,std::min(width,height)/2.):radius;}
Result<void> ProgressModel::observe(std::shared_ptr<const ProgressChangePort> receiver){auto m=impl_;return m->observe(std::move(receiver));}
}
