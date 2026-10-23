#include <LVRS/ViewStateTracker.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
bool sync(lvrs::ViewStateTracker& tracker,std::initializer_list<lvrs::ViewStackInput> entries){return take(tracker.sync_stack(std::span<const lvrs::ViewStackInput>{entries.begin(),entries.size()}));}
void run(std::string name){
    lvrs::ViewStateTracker tracker;using State=lvrs::ViewState;
    if(name=="states"){
        sync(tracker,{{"home","/"},{"report","/reports"},{"blocked","/blocked",false}});const auto snapshot=take(tracker.snapshot());
        check(snapshot.state.loaded_count==3&&snapshot.state.active_count==1&&snapshot.state.inactive_count==1&&snapshot.state.disabled_count==1&&snapshot.state.current_active_view=="report","Tracker partitions did not select the last enabled view");
        check(snapshot.loaded_views==std::vector<std::string>{"home","report","blocked"}&&snapshot.active_views==std::vector<std::string>{"report"}&&snapshot.inactive_views==std::vector<std::string>{"home"}&&snapshot.disabled_views==std::vector<std::string>{"blocked"},"Tracker partition order changed");
        check(take(tracker.state_of("absent")).empty()&&!take(tracker.view("absent"))&&!take(tracker.is_loaded("absent")),"Missing view acquired an invented record");
    }else if(name=="normalization"){
        sync(tracker,{{"\xc2\xa0 report \xe3\x80\x80"," /reports/// "}});const auto record=take(tracker.view(" report "));check(record&&record->view_id=="report"&&record->path=="/reports///","Tracker normalization changed route syntax");
    }else if(name=="anonymous"){
        sync(tracker,{{""," /path "},{},{"explicit",""},{}});const auto records=take(tracker.stack());check(records[0].view_id=="/path"&&records[1].view_id=="_component_0"&&records[2].view_id=="explicit"&&records[3].view_id=="_component_1","Anonymous ids no longer count only anonymous entries");
    }else if(name=="duplicates"){
        sync(tracker,{{"same","/one"},{"other","/other"},{"same","/two",false}});auto record=take(tracker.view("same"));check(record&&record->index==2&&record->path=="/two"&&record->state==State::disabled,"Lookup did not select the topmost duplicate");
        take(tracker.set_view_disabled("same",true));check(take(tracker.state_of("same"))=="Disabled"&&take(tracker.disabled_views())==std::vector<std::string>{"same","same"},"Override did not apply to all duplicate ids");
        take(tracker.set_view_enabled("same",true));check(take(tracker.view("same"))->state==State::disabled&&take(tracker.active_views())==std::vector<std::string>{"other"},"Enable override incorrectly forced a base-disabled duplicate on");
    }else if(name=="overrides"){
        sync(tracker,{{"a","/a"},{"b","/b"},{"c","/c"}});take(tracker.set_view_disabled("c",true));check(take(tracker.state()).current_active_view=="b","Disabling top view did not promote its predecessor");
        take(tracker.set_view_disabled("a",true));check(take(tracker.state()).current_active_view=="b"&&take(tracker.state()).disabled_count==2,"Disabling an inactive view changed the active view");
        take(tracker.set_view_enabled("c",true));check(take(tracker.state()).current_active_view=="c"&&!take(tracker.set_view_enabled("c",true)),"Restoring top view or duplicate override changed semantics");
    }else if(name=="dormant"){
        const auto revision=take(tracker.state()).revision;check(!take(tracker.set_view_disabled("future",true))&&take(tracker.state()).revision==revision,"Unloaded override emitted a stack publication");
        sync(tracker,{{"future","/future"}});check(take(tracker.state_of("future"))=="Disabled","Unloaded override was not retained across sync");
        sync(tracker,{});sync(tracker,{{"future","/future"}});check(take(tracker.state_of("future"))=="Disabled","Sync cleared persistent overrides");
    }else if(name=="base_enabled"){
        sync(tracker,{{"first","/first",false},{"second","/second",true,true}});check(take(tracker.state()).active_count==0&&take(tracker.state()).disabled_count==2,"Base disabled policy was ignored");
        take(tracker.set_view_enabled("first",true));check(take(tracker.state_of("first"))=="Disabled","set_view_enabled bypassed base-enabled policy");
    }else if(name=="clear"){
        check(!take(tracker.clear()),"Empty tracker clear published");take(tracker.set_view_disabled("future",true));check(take(tracker.clear()),"Clear did not remove an override-only state");sync(tracker,{{"future",""}});check(take(tracker.state_of("future"))=="Active","Clear retained a disabled override");take(tracker.clear());check(take(tracker.stack()).empty(),"Clear retained records");
    }else if(name=="value"){
        core::Value value{core::Value::Array{core::Value{core::Value::Object{{"viewId",core::Value{"home"}},{"path",core::Value{" / "}}}},core::Value{core::Value::Object{{"path",core::Value{"/report"}},{"disabled",core::Value{true}}}}}};
        check(take(tracker.sync_value(value)),"Typed Core input did not sync");const auto snapshot=take(tracker.snapshot_value());const auto& map=snapshot.get<core::Value::Object>()->get();check(map.at("currentActiveView")==core::Value{"home"},"Snapshot active id changed");
        const auto& entries=map.at("stack").get<core::Value::Array>()->get();const auto& report=entries[1].get<core::Value::Object>()->get();check(report.at("viewId")==core::Value{"/report"}&&report.at("stateCode")==core::Value{std::int64_t(2)}&&report.at("loaded")==core::Value{true}&&report.at("enabled")==core::Value{false},"Typed snapshot lost original fields");
    }else if(name=="malformed"){
        sync(tracker,{{"safe","/safe"}});const auto revision=take(tracker.state()).revision;
        check(!tracker.sync_value(core::Value{1})&&!tracker.sync_value(core::Value{core::Value::Array{core::Value{"invalid"}}})&&!tracker.sync_value(core::Value{core::Value::Array{core::Value{core::Value::Object{{"enabled",core::Value{"false"}}}}}}),"Malformed Core tracking input was coerced");
        check(!tracker.set_view_disabled(std::string("a\0b",3),true)&&!tracker.set_view_disabled("\xc0\xaf",true)&&take(tracker.state()).revision==revision,"Invalid ids partially modified tracker state");
    }else if(name=="observation"){
        unsigned count=0;auto port=std::make_shared<lvrs::ViewStatePort>([&](const auto& state){++count;check(take(tracker.state()).revision==state.revision&&take(tracker.stack()).size()==state.loaded_count,"Tracker observer saw inconsistent state");check(!tracker.clear()&&!tracker.set_view_disabled("a",true),"Tracker mutation reentered notification");});good(tracker.observe(port));good(tracker.observe(port));
        sync(tracker,{{"a","/a"},{"b","/b"}});check(!sync(tracker,{{"a","/a"},{"b","/b"}}),"Identical stack published another revision");take(tracker.set_view_disabled("b",true));take(tracker.clear());check(count==3,"Stack events were duplicated or omitted");
    }else if(name=="failure"){
        auto failing=std::make_shared<lvrs::ViewStatePort>([](const auto&){throw std::runtime_error("view receiver failed");});unsigned count=0;auto other=std::make_shared<lvrs::ViewStatePort>([&](const auto&){++count;});good(tracker.observe(failing));good(tracker.observe(other));
        std::array input{lvrs::ViewStackInput{"committed","/committed"}};auto result=tracker.sync_stack(input);check(!result&&result.error().cause&&take(tracker.state()).current_active_view=="committed"&&count==1,"Receiver failure rolled back stack or skipped remaining observers");check(bool(failing->close()),"Could not close failing view receiver");take(tracker.clear());check(count==2,"Closed receiver blocked delivery");
    }else if(name=="limits"){
        sync(tracker,{{"safe","/safe"}});const auto revision=take(tracker.state()).revision;std::vector<lvrs::ViewStackInput> many(lvrs::NavigationLimits::depth+1);check(!tracker.sync_stack(many),"Tracker depth budget ignored");std::array large{lvrs::ViewStackInput{std::string(lvrs::RouteLimits::path_bytes+1,'x'),""}};check(!tracker.sync_stack(large)&&take(tracker.state()).revision==revision,"Invalid candidate replaced the old tracker");
        std::vector<std::shared_ptr<lvrs::ViewStatePort>> ports;for(unsigned i=0;i<256;++i){ports.push_back(std::make_shared<lvrs::ViewStatePort>([](const auto&){}));good(tracker.observe(ports.back()));}auto overflow=std::make_shared<lvrs::ViewStatePort>([](const auto&){});check(!tracker.observe(overflow),"Receiver budget ignored");check(bool(ports.front()->close()),"Could not close a view receiver");take(tracker.clear());good(tracker.observe(overflow));
    }else if(name=="ownership"){
        bool read=false,write=false;std::thread thread([&]{auto a=tracker.snapshot();auto b=tracker.clear();read=!a&&a.error().code==core::ErrorCode::wrong_executor;write=!b&&b.error().code==core::ErrorCode::wrong_executor;});thread.join();check(read&&write,"Tracker allowed foreign-thread access");
    }else if(name=="retirement"){
        auto model=std::make_shared<lvrs::ViewStateTracker>();auto raw=model.get();auto port=std::make_shared<lvrs::ViewStatePort>([&](const auto&){model.reset();});good(raw->observe(port));std::array input{lvrs::ViewStackInput{"retire",""}};check(take(raw->sync_stack(input))&&!model,"Receiver retirement invalidated the in-flight tracker");
    }else if(name=="incremental"){
        std::vector<lvrs::ViewStackInput> inputs;for(unsigned i=0;i<10000;++i)inputs.push_back({"row"+std::to_string(i),"/"+std::to_string(i)});take(tracker.sync_stack(inputs));const auto data=take(tracker.stack_view()).data();const auto first_text=data[0].path.data();
        for(unsigned i=9990;i<10000;++i)take(tracker.set_view_disabled("row"+std::to_string(i),true));check(take(tracker.stack_view()).data()==data&&take(tracker.stack_view())[0].path.data()==first_text&&take(tracker.state()).current_active_view=="row9989","Override rebuilt the whole stack or failed incremental active selection");
        check(take(tracker.is_loaded("row0"))&&take(tracker.state_of("row0"))=="Inactive"&&take(tracker.state_of("row9989"))=="Active"&&take(tracker.state_of("row9999"))=="Disabled","Indexed selective queries lost the updated duplicate/active policy");
    }else if(name=="singleton"){
        auto first=lvrs::ViewStateTracker::current();check(first==lvrs::ViewStateTracker::current(),"Owner-thread tracker default changed identity");std::shared_ptr<lvrs::ViewStateTracker> other;std::thread thread([&]{other=lvrs::ViewStateTracker::current();});thread.join();check(first!=other,"Tracker default crossed owner threads");
    }else throw std::runtime_error("Unknown ViewStateTracker case");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"states");std::cout<<"LVRS_VIEW_STATE_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
