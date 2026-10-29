#include "ModelProperties.h"
#include <iiSDKCore/Core/Property/Observable.h>
#include <thread>

namespace lvrs {
struct ModelProperties::Impl final {
    const std::thread::id owner=std::this_thread::get_id();
    ii::core::Observable<std::uint64_t> epoch{0};
    bool closed=false;
};
ModelProperties::ModelProperties():properties_(std::make_shared<Impl>()){}
ModelProperties::~ModelProperties(){
    auto state=properties_;state->closed=true;
    // A borrowed shared handle is already expired when its owner is destroyed.
    // Subscribers must test that handle before querying virtual model methods.
    try{state->epoch.set(state->epoch.get()+1);}catch(...) {}
}
ModelProperties::DescriptorSignal ModelProperties::descriptor_signal()const noexcept{
    DescriptorSignal result;result.state_=properties_;return result;
}
ii::core::Result<void> ModelProperties::DescriptorSignal::notify()const{
    auto state=state_.lock();
    if(!state||state->closed)return {};
    if(state->owner!=std::this_thread::get_id())return ii::core::fail(ii::core::ErrorCode::wrong_executor,"Model properties require their owner thread");
    try{state->epoch.set(state->epoch.get()+1);return {};}
    catch(...){return ii::core::fail(ii::core::ErrorCode::failed,"Model descriptor observation failed after commit",std::current_exception());}
}
ii::core::ObservableSubscription ModelProperties::observe_descriptor(std::function<void()> callback)const{
    auto state=properties_;
    if(state->closed||state->owner!=std::this_thread::get_id())throw std::logic_error("Model properties require their active owner thread");
    if(!callback)throw std::invalid_argument("Descriptor observation requires a callback");
    return state->epoch.observe([callback=std::move(callback)](auto,auto){callback();});
}
}
