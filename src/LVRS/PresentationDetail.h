#pragma once
#include <iiSDKCore/Core/Property/Observable.h>
#include <iiSDKCore/Core/Value/Value.h>
#include <iiSDKCore/Runtime/Query.h>
#include <algorithm>
#include <optional>
#include <span>
#include <thread>
namespace lvrs::presentation_detail {
std::string normalized_text(std::string_view);
std::vector<std::string> ordered_keys(const ii::core::Value::Object&);
ii::core::Value::Object normalized_values(const ii::core::Value::Object&);
double number(const ii::core::Value&,double fallback);
bool fuzzy_equal(double,double);
struct Domain {
    const std::thread::id owner=std::this_thread::get_id();
    bool changing=false,notifying=false,closed=false;
    ii::core::Result<void> writable()const{
        if(owner!=std::this_thread::get_id())return ii::core::fail(ii::core::ErrorCode::wrong_executor,"Presentation model requires its owner thread");
        if(closed)return ii::core::fail(ii::core::ErrorCode::closed,"Presentation model is closed");
        if(changing||notifying)return ii::core::fail(ii::core::ErrorCode::busy,"Presentation model is committing or notifying a command");
        return {};
    }
    void readable()const{
        if(owner!=std::this_thread::get_id()||closed)throw std::logic_error("Presentation model read requires its active owner thread");
    }
};
struct Flag final {bool& value;explicit Flag(bool& v):value(v){value=true;}~Flag(){value=false;}};
template<class Change>struct Signals:Domain {
    using Port=ii::runtime::QueryPort<Change>;
    std::vector<std::weak_ptr<const Port>> receivers;
    ii::core::Result<void> observe(std::shared_ptr<const Port> port){
        if(auto ready=writable();!ready)return ready;
        if(!port)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Presentation observation requires a receiver");
        std::erase_if(receivers,[](const auto& r){return r.expired();});
        for(const auto& r:receivers)if(auto live=r.lock();live&&live.get()==port.get())return {};
        if(receivers.size()==1024)return ii::core::fail(ii::core::ErrorCode::full,"Presentation observer capacity exhausted");
        receivers.push_back(std::move(port));return {};
    }
    static void tick(ii::core::Observable<std::uint64_t>& epoch,std::optional<ii::core::Error>& failure){
        try{epoch.set(epoch.get()+1);}catch(...){if(!failure)failure=ii::core::Error{ii::core::ErrorCode::failed,"Presentation observation failed after commit",std::current_exception()};}
    }
    void send(const Change& change,std::optional<ii::core::Error>& failure){
        std::vector<std::shared_ptr<const Port>> live;
        std::erase_if(receivers,[](const auto& r){return r.expired();});
        for(const auto& receiver:receivers)if(auto port=receiver.lock())live.push_back(std::move(port));
        for(const auto& port:live){if(closed)break;auto reply=port->send(change);if(!reply&&reply.error().code!=ii::runtime::QueryErrorCode::receiver_expired&&!failure)
            failure=ii::core::Error{ii::core::ErrorCode::failed,"Presentation receiver failed after commit",reply.error().cause};}
    }
};
template<class F>auto attempt(F action)->std::invoke_result_t<F&>{
    try{return action();}catch(...){return ii::core::fail(ii::core::ErrorCode::failed,"Presentation command failed",std::current_exception());}
}
}
