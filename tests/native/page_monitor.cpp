#include <LVRS/PageMonitor.h>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
void run(std::string name){
    lvrs::PageMonitor monitor;
    if(name=="empty"){
        const auto state=take(monitor.state());check(state.revision==1&&state.count==0&&state.current.empty()&&!state.can_undo,"Initial monitor state changed");
        check(take(monitor.history()).empty()&&take(monitor.undo()).empty()&&!take(monitor.clear())&&!take(monitor.record(" \t\n")),"Empty operations mutated history");
    }else if(name=="record"){
        check(take(monitor.record(" reports/// "))&&!take(monitor.record("reports///")),"Trim or consecutive duplicate suppression failed");
        take(monitor.record("/other"));take(monitor.record("reports///"));
        check(take(monitor.history())==std::vector<std::string>{"reports///","/other","reports///"}&&take(monitor.state()).revision==4,"Visit log unexpectedly normalized routes or removed nonconsecutive repeats");
    }else if(name=="unicode"){
        take(monitor.record("\xc2\xa0\xe3\x80\x80/보고서\xe2\x80\x89"));check(take(monitor.state()).current=="/보고서","Unicode White_Space trim failed");
        check(!monitor.record("\xc0\xaf")&&!monitor.record(std::string("/a\0b",4))&&take(monitor.state()).count==1,"Invalid UTF-8 or NUL modified the log");
    }else if(name=="undo"){
        take(monitor.record("/"));take(monitor.record("/one"));take(monitor.record("/two"));
        check(take(monitor.undo())=="/one"&&take(monitor.state()).can_undo&&take(monitor.undo())=="/"&&!take(monitor.state()).can_undo,"Undo did not retain the first visit");
        const auto revision=take(monitor.state()).revision;check(take(monitor.undo())=="/"&&take(monitor.state()).revision==revision,"Undo of a single visit published a change");
    }else if(name=="clear"){
        take(monitor.record("/"));check(take(monitor.clear())&&!take(monitor.clear())&&take(monitor.state()).count==0,"Clear was not idempotent");take(monitor.record("/new"));check(take(monitor.state()).current=="/new","Clear retained stale history");
    }else if(name=="growth"){
        std::uintptr_t previous=0;std::size_t growths=0;
        for(unsigned i=0;i<10000;++i){take(monitor.record("/"+std::to_string(i)));const auto address=reinterpret_cast<std::uintptr_t>(take(monitor.history_view()).data());if(address!=previous){previous=address;++growths;}}
        check(growths<32&&take(monitor.state()).count==10000,"Recording repeatedly copied the complete visit log");
    }else if(name=="limits"){
        take(monitor.record("/safe"));auto before=take(monitor.state());check(!monitor.record(std::string(lvrs::RouteLimits::path_bytes+1,'x'))&&take(monitor.state()).revision==before.revision,"Oversized record partially committed");
        bool rejected=false;for(unsigned i=0;i<300;++i){auto result=monitor.record(std::string(65500,'x')+std::to_string(i));if(!result){check(result.error().code==core::ErrorCode::full,"History budget returned the wrong error");rejected=true;break;}}
        check(rejected&&take(monitor.state()).count<300,"Visit-log byte budget was not enforced");
    }else if(name=="observation"){
        unsigned count=0;auto port=std::make_shared<lvrs::PageMonitorPort>([&](const auto& state){++count;check(take(monitor.state()).revision==state.revision&&take(monitor.history()).size()==state.count,"Monitor observer saw an uncommitted history");check(!monitor.record("/reenter")&&!monitor.clear(),"Monitor mutation reentered notification");});
        good(monitor.observe(port));good(monitor.observe(port));take(monitor.record("/"));take(monitor.record("/"));take(monitor.record("/next"));take(monitor.undo());take(monitor.clear());check(count==4,"Monitor notification coalescing changed");port.reset();take(monitor.record("/expired"));
    }else if(name=="failure"){
        unsigned called=0;auto failing=std::make_shared<lvrs::PageMonitorPort>([](const auto&){throw std::runtime_error("visit receiver failed");});auto later=std::make_shared<lvrs::PageMonitorPort>([&](const auto&){++called;});good(monitor.observe(failing));good(monitor.observe(later));
        auto result=monitor.record("/committed");check(!result&&result.error().cause&&take(monitor.state()).current=="/committed"&&called==1,"Receiver failure rolled back a visit or suppressed later receivers");
        check(bool(failing->close()),"Could not close failing monitor receiver");take(monitor.record("/closed-receiver"));check(called==2,"Closed receiver prevented remaining notification delivery");
    }else if(name=="ownership"){
        bool read=false,write=false;std::thread thread([&]{auto a=monitor.state();auto b=monitor.record("/bad");read=!a&&a.error().code==core::ErrorCode::wrong_executor;write=!b&&b.error().code==core::ErrorCode::wrong_executor;});thread.join();check(read&&write&&take(monitor.state()).count==0,"Monitor allowed foreign-thread access");
    }else if(name=="retirement"){
        auto model=std::make_shared<lvrs::PageMonitor>();auto raw=model.get();auto port=std::make_shared<lvrs::PageMonitorPort>([&](const auto&){model.reset();});good(raw->observe(port));check(take(raw->record("/retire"))&&!model,"Monitor could not retire safely from its receiver");
    }else if(name=="singleton"){
        auto first=lvrs::PageMonitor::current();check(first==lvrs::PageMonitor::current(),"Owner-thread default monitor changed identity");std::shared_ptr<lvrs::PageMonitor> other;std::thread thread([&]{other=lvrs::PageMonitor::current();});thread.join();check(first!=other,"Monitor default crossed owner threads");
    }else throw std::runtime_error("Unknown PageMonitor case");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"record");std::cout<<"LVRS_PAGE_MONITOR_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
