#include <LVRS/RenderMonitor.h>
#include <iiSDKCore/Core/Property/Observable.h>
#include <iiSDKCore/Runtime/EventLoop.h>
#include <array>
#include <cmath>
#include <iostream>
#include <thread>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
auto at(std::int64_t value){return runtime::Clock::time_point{}+std::chrono::milliseconds(value);}
struct Fixture final {
    std::shared_ptr<gui::PresentationStream> source=std::make_shared<gui::PresentationStream>();lvrs::RenderMonitor monitor{[]{return std::int64_t(123);}};std::int64_t time=0;
    Fixture(){monitor.attachWindow(source);}
    void frame(int elapsed){time+=elapsed;good(source->queued(at(time),gui::GraphicsApi::metal));}
};
struct Fake final:gui::PresentationSource {
    core::Observable<gui::PresentationEvent> value;bool stopped=false;
    bool closed()const override{return stopped;}
    core::Result<core::ObservableSubscription> observe(std::function<void(const gui::PresentationEvent&)> f)override{return value.observe([f=std::move(f)](const auto& e,std::uint64_t){f(e);});}
    void send(std::uint64_t sequence,int timestamp,gui::PresentationKind kind=gui::PresentationKind::queued,std::string error={}){value.set({kind,at(timestamp),sequence,gui::GraphicsApi::metal,std::move(error)});}
};
void defaults(){lvrs::RenderMonitor m;check(!m.active()&&m.fps()==0&&m.frameCount()==0&&m.recentSampleCount()==0&&m.frameSampleCapacity()==240&&m.droppedFrameThresholdMs()==20&&m.lastError().empty(),"Render defaults changed");}
void first(){Fixture f;f.frame(20);check(f.monitor.frameCount()==1&&f.monitor.recentSampleCount()==0&&f.monitor.lastFrameMs()==0,"First frame created synthetic elapsed time");}
void submillisecond(){Fixture f;good(f.source->queued(runtime::Clock::time_point{},gui::GraphicsApi::metal));good(f.source->queued(runtime::Clock::time_point{}+std::chrono::microseconds(500),gui::GraphicsApi::metal));good(f.source->queued(at(2),gui::GraphicsApi::metal));check(f.monitor.frameCount()==3&&f.monitor.recentSampleCount()==1&&f.monitor.lastFrameMs()==1,"Whole-millisecond restart semantics changed");}
void cadence(){Fixture f;f.frame(0);for(int i=1;i<8;++i)f.frame(i*10);check(f.monitor.p95FrameMs()==0,"Percentiles updated before eight positive samples");f.frame(80);check(f.monitor.p95FrameMs()==80&&f.monitor.p99FrameMs()==80&&f.monitor.avgFrameMs()==45&&f.monitor.fps()==12.5,"Percentile rank, cadence or average changed");f.frame(90);check(f.monitor.p95FrameMs()==80,"Unforced percentile cadence changed");}
void ring(){Fixture f;f.monitor.setFrameSampleCapacity(16);f.frame(0);for(int i=1;i<=20;++i)f.frame(i);check(f.monitor.recentSampleCount()==16&&f.monitor.frameCount()==21&&f.monitor.avgFrameMs()==12.5,"Ring eviction average or lifetime frames wrong");}
void resize(){Fixture f;f.frame(0);for(int i=1;i<=30;++i)f.frame(i);f.monitor.setFrameSampleCapacity(16);check(f.monitor.avgFrameMs()==22.5&&f.monitor.p95FrameMs()==30&&f.monitor.p99FrameMs()==30,"Resize did not retain latest ordered values and force percentiles");f.monitor.setFrameSampleCapacity(100);f.frame(31);check(f.monitor.recentSampleCount()==17&&f.monitor.avgFrameMs()==23,"Ring resize lost append position");}
void threshold(){Fixture f;f.frame(0);f.frame(20);f.frame(30);f.frame(10);check(f.monitor.droppedFrameCount()==2,"Threshold equality was not counted");f.monitor.setDroppedFrameThresholdMs(35);check(f.monitor.droppedFrameCount()==0,"Threshold did not recompute retained samples");}
void lifetime_drops(){Fixture f;f.monitor.setFrameSampleCapacity(16);f.frame(0);for(int i=0;i<100;++i)f.frame(30);check(f.monitor.droppedFrameCount()==100,"Eviction decremented lifetime drops");f.monitor.setDroppedFrameThresholdMs(21);check(f.monitor.droppedFrameCount()==16,"Threshold did not reset drops to retained window");f.frame(30);f.monitor.setFrameSampleCapacity(200);check(f.monitor.droppedFrameCount()==17,"Capacity change recomputed lifetime drops");}
void bounds(){Fixture f;f.monitor.setFrameSampleCapacity(-1);f.monitor.setDroppedFrameThresholdMs(-1);check(f.monitor.frameSampleCapacity()==16&&f.monitor.droppedFrameThresholdMs()==5,"Lower monitor bounds lost");f.monitor.setFrameSampleCapacity(3000);f.monitor.setDroppedFrameThresholdMs(1000);check(f.monitor.frameSampleCapacity()==2000&&f.monitor.droppedFrameThresholdMs()==100,"Upper monitor bounds lost");}
void nonfinite(){Fixture f;bool failed=false;try{f.monitor.setDroppedFrameThresholdMs(INFINITY);}catch(const std::invalid_argument&){failed=true;}check(failed&&f.monitor.droppedFrameThresholdMs()==20,"Nonfinite threshold committed");}
void stop_resume(){Fixture f;f.frame(0);f.frame(20);f.monitor.stop();f.frame(30);f.monitor.start();f.frame(10);check(f.monitor.frameCount()==3&&f.monitor.lastFrameMs()==40&&f.monitor.avgFrameMs()==30&&f.monitor.lastError().empty(),"Stopped delivery or pause-inclusive restart changed");}
void reset(){Fixture f;f.frame(0);f.frame(20);f.monitor.reset();check(f.monitor.active()&&f.monitor.frameCount()==0&&f.monitor.recentSampleCount()==0,"Reset detached monitor or retained metrics");f.frame(10);check(f.monitor.frameCount()==1&&f.monitor.recentSampleCount()==0,"Reset retained time baseline");}
void reattach(){Fixture f;f.frame(0);f.frame(20);auto old=f.source;f.source=std::make_shared<gui::PresentationStream>();f.monitor.attachWindow(f.source);good(old->queued(at(100),gui::GraphicsApi::metal));check(f.monitor.active()&&f.monitor.frameCount()==0&&!old->has_observers(),"Old source changed replacement metrics");f.time=0;f.frame(0);check(f.monitor.frameCount()==1,"Replacement source did not start");}
void null_source(){Fixture f;f.frame(0);f.monitor.attachWindow(std::shared_ptr<gui::PresentationSource>{});check(!f.monitor.active()&&f.monitor.frameCount()==0&&!f.source->has_observers(),"Null attachment retained capture");f.monitor.start();check(f.monitor.active(),"Explicit start without source differs from original");}
void closed_source(){auto source=std::make_shared<gui::PresentationStream>();source->close();lvrs::RenderMonitor m;m.attachWindow(source);check(!m.active()&&m.frameCount()==0,"Closed source attached active");}
void closure(){Fixture f;f.frame(0);f.frame(20);f.source->close();check(!f.monitor.active()&&f.monitor.frameCount()==2&&f.monitor.lastFrameMs()==20,"Window closure reset completed metrics");}
void source_failure(){auto source=std::make_shared<Fake>();lvrs::RenderMonitor m;m.attachWindow(source);source->send(1,0);source->send(1,0,gui::PresentationKind::closed,"queue full");check(!m.active()&&m.lastError()=="queue full"&&m.frameCount()==1,"Source failure lost diagnostic or metrics");}
void gap(){auto source=std::make_shared<Fake>();lvrs::RenderMonitor m;m.attachWindow(source);source->send(50,0);source->send(52,20);check(!m.active()&&!m.lastError().empty()&&m.frameCount()==1,"Missing source events silently entered statistics");}
void backwards(){auto source=std::make_shared<Fake>();lvrs::RenderMonitor m;m.attachWindow(source);source->send(1,20);source->send(2,10);check(!m.active()&&!m.lastError().empty()&&m.frameCount()==1,"Backward source clock committed");}
void duplicate(){auto source=std::make_shared<Fake>();lvrs::RenderMonitor m;m.attachWindow(source);source->send(1,0);source->send(1,20);check(!m.active()&&m.frameCount()==1,"Repeated source sequence counted");}
void owner(){Fixture f;bool read=false,write=false;std::thread t([&]{try{(void)f.monitor.fps();}catch(const std::logic_error&){read=true;}try{f.monitor.start();}catch(const std::logic_error&){write=true;}});t.join();check(read&&write&&f.monitor.active(),"Foreign monitor access admitted");}
void events(){Fixture f;std::vector<lvrs::RenderMonitorEvent> events;auto p=std::make_shared<lvrs::RenderMonitorChangePort>([&](auto e){events.push_back(e);});good(f.monitor.observe(p));f.monitor.setFrameSampleCapacity(16);check(events==std::vector{lvrs::RenderMonitorEvent::sample_capacity,lvrs::RenderMonitorEvent::stats},"Capacity notification order changed");events.clear();f.monitor.setDroppedFrameThresholdMs(25);check(events==std::vector{lvrs::RenderMonitorEvent::dropped_threshold,lvrs::RenderMonitorEvent::stats},"Threshold notification order changed");}
void observer_failure(){Fixture f;int stats=0;auto a=std::make_shared<lvrs::RenderMonitorChangePort>([&](auto e){if(e==lvrs::RenderMonitorEvent::sample_capacity)throw std::runtime_error("observer");});auto b=std::make_shared<lvrs::RenderMonitorChangePort>([&](auto e){stats+=e==lvrs::RenderMonitorEvent::stats;});good(f.monitor.observe(a));good(f.monitor.observe(b));bool failed=false;try{f.monitor.setFrameSampleCapacity(16);}catch(...){failed=true;}check(failed&&stats==1&&f.monitor.frameSampleCapacity()==16,"Observer failure rolled back or suppressed later events");}
void retirement(){auto source=std::make_shared<gui::PresentationStream>();auto m=std::make_unique<lvrs::RenderMonitor>();m->attachWindow(source);auto port=std::make_shared<lvrs::RenderMonitorChangePort>([&](auto){m.reset();});good(m->observe(port));good(source->queued(at(0),gui::GraphicsApi::metal));check(!m&&!source->has_observers(),"Retired monitor retained source subscription");}
void dependencies(){Fixture f;std::vector<core::ObservableRead> reads;core::ObservableReadScope::collect([&](auto read){reads.push_back(std::move(read));},[&]{(void)f.monitor.fps();(void)f.monitor.frameCount();});check(reads.size()==2&&reads[0].identity==reads[1].identity,"Stats properties do not share native dependency group");int changes=0;auto lease=reads[0].subscribe([&]{++changes;});f.frame(0);check(changes==1,"Hidden C++ metrics did not invalidate projection");}
void snapshot(){Fixture f;f.frame(0);f.frame(20);auto values=f.monitor.performanceSnapshot();check(values.size()==14&&values.at("schema")==core::Value("lvrs.performance.v1")&&values.at("epochMs")==core::Value(123)&&values.at("frameCount")==core::Value(std::uint64_t(2)),"Snapshot fields, integer kinds or injected epoch changed");}
void deferred_source(){runtime::EventLoop loop(8);auto source=std::make_shared<gui::PresentationStream>([&](runtime::Task task){return loop.post(std::move(task));});lvrs::RenderMonitor m;m.attachWindow(source);good(source->queued(at(1),gui::GraphicsApi::metal));check(m.frameCount()==0,"Renderer entered projection synchronously");loop.poll();check(m.frameCount()==1,"Core delivery not projected");}
}
int main(int argc,char** argv){try{const std::string name=argc>1?argv[1]:"defaults";
#define CASE(value) if(name==#value)value();else
CASE(defaults) CASE(first) CASE(submillisecond) CASE(cadence) CASE(ring) CASE(resize) CASE(threshold) CASE(lifetime_drops) CASE(bounds) CASE(nonfinite) CASE(stop_resume) CASE(reset) CASE(reattach) CASE(null_source) CASE(closed_source) CASE(closure) CASE(source_failure) CASE(gap) CASE(backwards) CASE(duplicate) CASE(owner) CASE(events) CASE(observer_failure) CASE(retirement) CASE(dependencies) CASE(snapshot) CASE(deferred_source)
throw std::runtime_error("Unknown render case");
#undef CASE
std::cout<<"Render monitor "<<name<<" passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
