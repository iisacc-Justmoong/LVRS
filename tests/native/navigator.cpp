#include <LVRS/Navigator.h>
#include <LVRS/PageRouter.h>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
struct Target final : lvrs::NavigationTarget {
    lvrs::NavigationStackModel history;
    std::vector<std::weak_ptr<lvrs::NavigationTargetPort>> receivers;
    std::function<void()> on_go;
    int commands=0;bool open=true,throws=false,tracking_busy=false,tracking_throws=false;
    Target(){take(history.apply_path_operation("/"));}
    core::Result<lvrs::NavigationTargetState> state()const override{
        if(!open)return core::fail(core::ErrorCode::closed,"Target closed");auto value=history.state();if(!value)return std::unexpected(value.error());return lvrs::NavigationTargetState{.navigation=*value,.mounted=true,.can_go_back=value->depth>1,.content_generation=value->revision};
    }
    core::Result<std::vector<lvrs::NavigationViewEntry>> view_tracking_entries()const override{
        if(tracking_throws)throw std::runtime_error("Target tracking getter failed");
        if(tracking_busy)return core::fail(core::ErrorCode::busy,"Target is constructing a page");return history.view_tracking_entries();
    }
    core::Result<bool> finish(core::Result<bool> result){
        if(!result)return result;auto current=take(history.state());
        for(const auto& weak:receivers)if(auto port=weak.lock()){auto reply=port->send({current,{},current.revision});if(!reply)return core::fail(core::ErrorCode::failed,"Target receiver failed",reply.error().cause);}
        return result;
    }
    core::Result<bool> go(std::string_view path,core::Value params,lvrs::NavigationOperation operation)override{
        ++commands;if(throws)throw std::runtime_error("Foreign target failure");if(on_go)on_go();return finish(history.apply_path_operation(path,std::move(params),operation));
    }
    core::Result<bool> go_to(lvrs::PageComponentRef page,core::Value params,lvrs::NavigationOperation operation)override{++commands;return finish(history.apply_component_operation(std::move(page),std::move(params),operation));}
    core::Result<bool> back()override{++commands;return finish(history.pop());}
    core::Result<bool> pop_to_root()override{++commands;return finish(history.pop_to_root());}
    core::Result<void> observe(std::weak_ptr<lvrs::NavigationTargetPort> port)override{receivers.push_back(std::move(port));return {};}
};
void run(std::string name){
    lvrs::Navigator navigator;auto first=std::make_shared<Target>(),second=std::make_shared<Target>();
    if(name=="registration"){
        check(!take(navigator.can_navigate())&&!take(navigator.go("/missing"))&&!take(navigator.register_router({})),"Empty registry admitted navigation");
        take(navigator.register_router(first));take(navigator.register_router(second));check(take(navigator.router())==second&&take(navigator.router_stack()).size()==2,"Last router did not become active");
        take(navigator.register_router(first));check(take(navigator.router())==first&&take(navigator.router_stack()).size()==2,"Re-registration duplicated a router");
        good(navigator.unregister_router(first));check(take(navigator.router())==second,"Unregister did not restore the previous router");good(navigator.unregister_router());check(!take(navigator.state()).has_router,"Clear retained an active router");
    }else if(name=="siblings"){
        auto group=std::make_shared<const int>(1);take(navigator.register_router(second,{group,7,20}));take(navigator.register_router(first,{group,7,10}));
        auto stack=take(navigator.router_stack());check(stack.size()==2&&stack.front()==first&&stack.back()==second,"Registration ignored sibling order");
        take(navigator.register_router(first,{group,7,10}));check(take(navigator.router())==second,"Re-registration moved an earlier sibling above a later sibling");
        take(navigator.register_router(first,{group,8,0}));check(take(navigator.router())==first,"Different parents were incorrectly sorted together");
    }else if(name=="dispatch"){
        take(navigator.register_router(first));check(!take(navigator.go(""))&&first->commands==0,"Empty href was dispatched");
        check(take(navigator.back())&&first->commands==1&&take(navigator.state()).current.navigation.depth==1,"Navigator bool lost no-op dispatch admission");
        take(navigator.go("/one",core::Value{core::Value::Object{{"typed",core::Value{42}}}}));take(navigator.replace("/two"));check(take(navigator.state()).current.navigation.current_path=="/two"&&take(navigator.state()).current.navigation.depth==2,"Replace did not reach the active target");
        take(navigator.go("/three"));take(navigator.pop_to_root());take(navigator.set_root("/root"));check(take(navigator.state()).current.navigation.current_path=="/root"&&take(navigator.state()).current.navigation.depth==1,"Root dispatch diverged from target history");
    }else if(name=="components"){
        take(navigator.register_router(first));auto page=std::make_shared<const lvrs::PageComponent>(lvrs::PageFactory{});
        check(!take(navigator.go_to({}))&&first->commands==0,"Null component was dispatched");take(navigator.go_to(page,core::Value{7}));check(take(first->history.current_entry_descriptor()).component==page,"Compiled component identity was lost");
        take(navigator.replace_with(page,core::Value{8}));take(navigator.set_root_component(page));check(take(navigator.state()).current.navigation.depth==1,"Component replace/root was not forwarded");
    }else if(name=="weak"){
        take(navigator.register_router(first));take(navigator.register_router(second));second.reset();check(take(navigator.router())==first&&take(navigator.state()).router_count==1,"Expired target did not restore its predecessor");
        first->open=false;check(!take(navigator.can_navigate())&&!take(navigator.go("/closed")),"Closed manual target remained routable");
    }else if(name=="leases"){
        std::shared_ptr<lvrs::NavigatorRegistration> lease,other;good(navigator.track_router(first,lease));good(navigator.track_router(first,other));good(lease->close());check(take(navigator.router())==first,"Old registration lease removed a newer registration");
        good(other->close());check(!take(navigator.can_navigate()),"Scoped close retained registration");
        good(navigator.track_router(first,lease));take(navigator.register_router(first));good(lease->close());check(!take(navigator.can_navigate()),"Manual re-registration detached the existing scope lifetime");
        good(navigator.track_router(first,lease));std::thread worker([lease=std::move(lease)]()mutable{lease.reset();});worker.join();check(!take(navigator.can_navigate()),"Cross-thread lease destruction left a routable target");
    }else if(name=="observation"){
        int called=0;auto receiver=std::make_shared<lvrs::NavigatorPort>([&](const auto& event){++called;check(take(navigator.state()).revision==event.state.revision,"Navigator observer saw inconsistent state");check(!navigator.unregister_router(),"Observer admitted mutation reentry");if(event.state.router_count==2)throw std::runtime_error("Registry receiver failed");});good(navigator.observe(receiver));take(navigator.register_router(first));
        auto result=navigator.register_router(second);check(!result&&result.error().cause&&take(navigator.router())==second&&called==2,"Receiver failure rolled back committed registration");
        std::shared_ptr<lvrs::NavigatorRegistration> lease;result=navigator.register_router(first);check(!result,"Expected registry receiver failure");
        auto tracked=navigator.track_router(second,lease);check(!tracked&&lease&&take(navigator.router())==second,"Committed scoped registration lost its lease on receiver error");receiver.reset();good(lease->close());
    }else if(name=="forwarding"){
        take(navigator.register_router(first));int changes=0;auto receiver=std::make_shared<lvrs::NavigatorPort>([&](const auto& event){if(event.current)++changes;});good(navigator.observe(receiver));
        take(first->go("/direct",{},lvrs::NavigationOperation::push));check(changes==1&&take(navigator.state()).current.navigation.current_path=="/direct","Direct target updates did not reach Navigator");
        take(navigator.go("/via-service"));check(changes==2,"Delegated target changes were not coalesced once");
    }else if(name=="retirement"){
        std::shared_ptr<lvrs::NavigatorRegistration> lease;good(navigator.track_router(first,lease));first->on_go=[&]{lease.reset();};take(navigator.go("/retire"));check(!take(navigator.can_navigate()),"Target callback could not retire its scoped registration");
        auto owned=std::make_shared<lvrs::Navigator>();auto raw=owned.get();take(raw->register_router(second));second->on_go=[&]{owned.reset();};check(take(raw->go("/retire-owner"))&&!owned,"Target callback could not retire the Navigator owner safely");
    }else if(name=="ownership"){
        bool rejected=false,distinct=false;auto local=lvrs::Navigator::current();check(local==lvrs::Navigator::current(),"Owner-thread default Navigator is unstable");
        std::thread worker([&]{auto status=navigator.state();rejected=!status&&status.error().code==core::ErrorCode::wrong_executor;distinct=lvrs::Navigator::current()!=local;});worker.join();check(rejected&&distinct,"Navigator domains or owner-thread admission changed");
    }else if(name=="limits"){
        std::vector<std::shared_ptr<Target>> targets;targets.reserve(4097);for(unsigned i=0;i<4096;++i){auto target=std::make_shared<Target>();take(navigator.register_router(target));targets.push_back(std::move(target));}
        auto result=navigator.register_router(first);check(!result&&result.error().code==core::ErrorCode::full&&take(navigator.state()).router_count==4096,"Registry limit partially changed the stack");take(navigator.register_router(targets.front()));
    }else if(name=="tracking"){
        auto tracker=take(navigator.view_state_tracker());take(navigator.register_router(first));take(navigator.go("/one"));
        check(take(tracker->loaded_views())==std::vector<std::string>{"/","/one"},"Selected history did not synchronize to the tracker");
        take(navigator.register_router(second));take(first->go("/inactive",{},lvrs::NavigationOperation::push));check(take(tracker->loaded_views())==std::vector<std::string>{"/"},"Inactive router overwrote the selected tracking domain");
        good(navigator.unregister_router(second));check(take(tracker->loaded_views())==std::vector<std::string>{"/","/one","/inactive"},"Selected router restoration did not restore tracking entries");
        good(navigator.unregister_router());check(take(tracker->state()).loaded_count==0,"Empty registry retained tracking entries");
        lvrs::Navigator isolated;check(take(isolated.view_state_tracker())!=tracker&&take(lvrs::Navigator::current()->view_state_tracker())==lvrs::ViewStateTracker::current(),"Explicit and owner-thread default tracking domains were mixed");
    }else if(name=="tracking_events"){
        auto tracker=take(navigator.view_state_tracker());std::string order;
        auto tracked=std::make_shared<lvrs::ViewStatePort>([&](const auto& event){order+='T';check(take(navigator.state()).current.navigation.current_path==event.current_active_view,"Tracker observer saw the previous Navigator selection");check(!navigator.go("/reenter"),"Tracker observer reentered Navigator mutation");});
        auto navigation=std::make_shared<lvrs::NavigatorPort>([&](const auto& event){order+='N';check(take(tracker->state()).current_active_view==event.state.current.navigation.current_path,"Navigator observer saw stale tracking state");});good(tracker->observe(tracked));good(navigator.observe(navigation));
        take(navigator.register_router(first));take(navigator.go("/next"));check(order=="TNTN","Tracker and Navigator notifications were not ordered after commit");
    }else if(name=="tracking_failure"){
        auto tracker=take(navigator.view_state_tracker());unsigned calls=0;auto failing=std::make_shared<lvrs::ViewStatePort>([](const auto&){throw std::runtime_error("Tracking receiver failed");});auto navigation=std::make_shared<lvrs::NavigatorPort>([&](const auto&){++calls;});good(tracker->observe(failing));good(navigator.observe(navigation));
        auto registered=navigator.register_router(first);check(!registered&&registered.error().cause&&take(navigator.router())==first&&take(tracker->state()).current_active_view=="/"&&calls==1,"Tracking receiver failure rolled back committed selection or suppressed Navigator publication");
        check(bool(failing->close()),"Could not close tracking receiver");second->tracking_throws=true;registered=navigator.register_router(second);check(!registered&&registered.error().cause&&calls==2,"Tracking getter exception escaped its Result boundary");second->tracking_throws=false;check(take(navigator.router())==second,"Tracking getter failure lost a committed registration");
    }else if(name=="tracking_busy"){
        auto tracker=take(navigator.view_state_tracker());first->tracking_busy=true;take(navigator.register_router(first));check(take(tracker->state()).loaded_count==0,"Busy tracking getter published incomplete descriptors");first->tracking_busy=false;take(navigator.state());check(take(tracker->state()).current_active_view=="/","Deferred tracker synchronization was not retried");
    }else if(name=="exceptions"){
        take(navigator.register_router(first));first->throws=true;auto result=navigator.go("/throw");check(!result&&result.error().cause&&take(navigator.state()).current.navigation.current_path=="/","Foreign exception escaped its Result boundary");
    }else throw std::runtime_error("Unknown Navigator case");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"registration");std::cout<<"LVRS_NAVIGATOR_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
