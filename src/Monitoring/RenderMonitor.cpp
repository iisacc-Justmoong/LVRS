#include <LVRS/RenderMonitor.h>
#include <LVRS/PresentationDetail.h>
#include <iiGUI/Host.h>
#include <array>
#include <chrono>
#include <cmath>

namespace lvrs {
namespace pd = presentation_detail;
struct RenderMonitor::Impl final : pd::Signals<RenderMonitorEvent>, std::enable_shared_from_this<Impl> {
    std::shared_ptr<ii::gui::PresentationSource> source;
    ii::core::ObservableSubscription subscription;
    std::function<std::int64_t()> epoch_milliseconds;
    std::vector<double> samples;
    std::optional<ii::runtime::Clock::time_point> previous;
    std::optional<std::uint64_t> last_sequence;
    std::uint64_t generation=0, frames=0, dropped=0;
    std::size_t next=0;
    int capacity=240, percentile_frames=0;
    double threshold=20, last=0, average=0, rate=0, p95=0, p99=0, sum=0;
    bool active=false;
    std::string error;
    std::array<std::uint64_t,5> versions{};
    ii::core::Observable<std::uint64_t> active_epoch{0}, stats_epoch{0}, threshold_epoch{0}, capacity_epoch{0}, error_epoch{0};
    auto& epoch(RenderMonitorEvent event) {
        switch(event){case RenderMonitorEvent::active:return active_epoch;case RenderMonitorEvent::stats:return stats_epoch;
        case RenderMonitorEvent::dropped_threshold:return threshold_epoch;case RenderMonitorEvent::sample_capacity:return capacity_epoch;
        case RenderMonitorEvent::last_error:return error_epoch;}
        throw std::logic_error("Unknown render monitor event");
    }
    void write(){if(auto ready=writable();!ready)throw std::logic_error(ready.error().message);}
    void publish(std::initializer_list<RenderMonitorEvent> events){
        std::optional<ii::core::Error> failure;
        const bool saved=notifying;notifying=true;
        struct Restore{bool& flag;bool value;~Restore(){flag=value;}}restore{notifying,saved};
        ii::core::ObservableReadScope::pause([&]{
            for(auto event:events){
                if(closed)break;
                try{epoch(event).set(++versions[std::size_t(event)]);}catch(...){if(!failure)failure=ii::core::Error{ii::core::ErrorCode::failed,"Render observer failed after commit",std::current_exception()};}
                if(!closed)send(event,failure);
            }
        });
        if(failure){if(failure->cause)std::rethrow_exception(failure->cause);throw std::runtime_error(failure->message);}
    }
    void clear(){samples.clear();next=0;sum=last=average=rate=p95=p99=0;frames=dropped=0;percentile_frames=0;previous.reset();last_sequence.reset();}
    std::vector<double> ordered()const{
        if(samples.size()<std::size_t(capacity)||next==0)return samples;
        std::vector<double> values;values.reserve(samples.size());values.insert(values.end(),samples.begin()+std::ptrdiff_t(next),samples.end());values.insert(values.end(),samples.begin(),samples.begin()+std::ptrdiff_t(next));return values;
    }
    void percentiles(){
        auto sorted=samples;std::ranges::sort(sorted);
        auto value=[&](double q){return sorted.empty()?0:sorted[std::min(sorted.size()-1,std::size_t(std::ceil(q*double(sorted.size())))-1)];};
        p95=value(.95);p99=value(.99);percentile_frames=0;
    }
    void append(double value){
        if(samples.size()<std::size_t(capacity)){samples.push_back(value);sum+=value;next=samples.size()<std::size_t(capacity)?samples.size():0;}
        else{sum+=value-samples[next];samples[next]=value;next=(next+1)%samples.size();}
        average=sum/double(samples.size());if(++percentile_frames>=8)percentiles();
    }
    void detach(){++generation;subscription.reset();source.reset();}
    void fail_source(std::string message){
        detach();const bool changed=active;active=false;error=std::move(message);
        if(changed)publish({RenderMonitorEvent::last_error,RenderMonitorEvent::active});else publish({RenderMonitorEvent::last_error});
    }
    void event(const ii::gui::PresentationEvent& event,std::uint64_t expected){
        if(owner!=std::this_thread::get_id())return;
        if(closed||generation!=expected)return;
        if(event.kind==ii::gui::PresentationKind::closed){
            detach();const bool changed=active;active=false;
            const bool error_changed=error!=event.error;error=event.error;
            if(changed&&error_changed)publish({RenderMonitorEvent::last_error,RenderMonitorEvent::active});
            else if(changed)publish({RenderMonitorEvent::active});else if(error_changed)publish({RenderMonitorEvent::last_error});
            return;
        }
        if(!active){last_sequence.reset();return;}
        if(!event.sequence||(last_sequence&&event.sequence!=*last_sequence+1)){fail_source("Presentation delivery sequence is discontinuous");return;}
        if(previous&&event.timestamp<*previous){fail_source("Presentation clock moved backwards");return;}
        last_sequence=event.sequence;
        const auto elapsed=previous?std::chrono::duration_cast<std::chrono::milliseconds>(event.timestamp-*previous).count():0;
        previous=event.timestamp;
        if(elapsed>0){last=double(elapsed);rate=1000./last;append(last);if(last>=threshold)++dropped;}
        ++frames;publish({RenderMonitorEvent::stats});
    }
};
RenderMonitor::RenderMonitor():RenderMonitor([]{return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}){}
RenderMonitor::RenderMonitor(std::shared_ptr<ii::gui::PresentationSource> source):RenderMonitor(){attachWindow(std::move(source));}
RenderMonitor::RenderMonitor(std::function<std::int64_t()> clock):impl_(std::make_shared<Impl>()){
    if(!clock)throw std::invalid_argument("Render monitor requires an epoch clock");impl_->epoch_milliseconds=std::move(clock);impl_->samples.reserve(240);
}
RenderMonitor::~RenderMonitor(){auto state=impl_;if(state->owner!=std::this_thread::get_id())std::terminate();state->closed=true;state->detach();state->receivers.clear();}
void RenderMonitor::attachWindow(std::shared_ptr<ii::gui::PresentationSource> source){
    auto state=impl_;state->write();const bool was_active=state->active;const auto old_error=state->error;
    state->detach();state->clear();state->active=false;state->error.clear();
    state->changing=true;
    if(source){
        try{
            if(!source->closed()){
                const auto expected=state->generation;auto admitting=std::make_shared<bool>(true),inline_event=std::make_shared<bool>(false);
                auto lease=source->observe([weak=std::weak_ptr<Impl>(state),expected,admitting,inline_event](const auto& event){
                    if(auto live=weak.lock()){if(live->owner!=std::this_thread::get_id())return;if(*admitting){*inline_event=true;return;}live->event(event,expected);}
                });*admitting=false;
                if(state->closed){state->changing=false;return;}
                if(lease&&lease->active()&&!*inline_event){state->subscription=std::move(*lease);state->source=std::move(source);state->active=true;}
                else state->error=lease?"Presentation source emitted during observation admission":lease.error().message;
            }
        }catch(const std::exception& e){state->error=e.what();}
        catch(...){state->error="Presentation source attachment failed";}
    }
    state->changing=false;if(state->closed)return;
    const bool changed=was_active!=state->active,failed=old_error!=state->error;
    if(changed&&failed)state->publish({RenderMonitorEvent::stats,RenderMonitorEvent::active,RenderMonitorEvent::last_error});
    else if(changed)state->publish({RenderMonitorEvent::stats,RenderMonitorEvent::active});
    else if(failed)state->publish({RenderMonitorEvent::stats,RenderMonitorEvent::last_error});else state->publish({RenderMonitorEvent::stats});
}
void RenderMonitor::attachWindow(ii::gui::Host& host){auto source=host.presentation_source();if(!source)throw std::logic_error(source.error().message);attachWindow(std::move(*source));}
void RenderMonitor::start(){auto state=impl_;state->write();if(!state->active){state->active=true;state->publish({RenderMonitorEvent::active});}}
void RenderMonitor::stop(){auto state=impl_;state->write();if(state->active){state->active=false;state->publish({RenderMonitorEvent::active});}}
void RenderMonitor::reset(){auto state=impl_;state->write();state->clear();state->publish({RenderMonitorEvent::stats});}
bool RenderMonitor::active()const{auto state=impl_;state->readable();(void)state->active_epoch.get();return state->active;}
#define LVRS_RENDER_STAT(type,name,member) type RenderMonitor::name()const{auto s=impl_;s->readable();(void)s->stats_epoch.get();return s->member;}
LVRS_RENDER_STAT(double,fps,rate)
LVRS_RENDER_STAT(double,lastFrameMs,last)
LVRS_RENDER_STAT(double,avgFrameMs,average)
LVRS_RENDER_STAT(double,p95FrameMs,p95)
LVRS_RENDER_STAT(double,p99FrameMs,p99)
LVRS_RENDER_STAT(std::uint64_t,droppedFrameCount,dropped)
LVRS_RENDER_STAT(std::uint64_t,frameCount,frames)
#undef LVRS_RENDER_STAT
int RenderMonitor::recentSampleCount()const{auto s=impl_;s->readable();(void)s->stats_epoch.get();return int(s->samples.size());}
double RenderMonitor::droppedFrameThresholdMs()const{auto s=impl_;s->readable();(void)s->threshold_epoch.get();return s->threshold;}
int RenderMonitor::frameSampleCapacity()const{auto s=impl_;s->readable();(void)s->capacity_epoch.get();return s->capacity;}
std::string RenderMonitor::lastError()const{auto s=impl_;s->readable();(void)s->error_epoch.get();return s->error;}
void RenderMonitor::setDroppedFrameThresholdMs(double value){
    auto s=impl_;s->write();if(!std::isfinite(value))throw std::invalid_argument("Frame threshold must be finite");value=std::clamp(value,5.,100.);
    if(std::abs(s->threshold-value)*1.e12<=std::min(std::abs(s->threshold),std::abs(value)))return;
    s->threshold=value;s->dropped=std::ranges::count_if(s->samples,[&](double frame){return frame>=value;});
    s->publish({RenderMonitorEvent::dropped_threshold,RenderMonitorEvent::stats});
}
void RenderMonitor::setFrameSampleCapacity(int value){
    auto s=impl_;s->write();value=std::clamp(value,16,2000);if(s->capacity==value)return;
    auto values=s->ordered();if(values.size()>std::size_t(value))values.erase(values.begin(),values.end()-value);
    s->capacity=value;s->samples=std::move(values);s->samples.reserve(std::size_t(value));s->next=s->samples.size()<std::size_t(value)?s->samples.size():0;
    s->sum=0;for(double frame:s->samples)s->sum+=frame;s->average=s->samples.empty()?0:s->sum/double(s->samples.size());s->percentiles();
    s->publish({RenderMonitorEvent::sample_capacity,RenderMonitorEvent::stats});
}
ii::core::Value::Object RenderMonitor::performanceSnapshot()const{
    using V=ii::core::Value;auto s=impl_;s->readable();
    auto values=ii::core::Value::Object{{"schema",V("lvrs.performance.v1")},{"component",V("RenderMonitor")},{"epochMs",V(0)},
        {"active",V(active())},{"fps",V(fps())},{"lastFrameMs",V(lastFrameMs())},{"avgFrameMs",V(avgFrameMs())},
        {"p95FrameMs",V(p95FrameMs())},{"p99FrameMs",V(p99FrameMs())},{"frameCount",V(frameCount())},
        {"droppedFrameCount",V(droppedFrameCount())},{"droppedFrameThresholdMs",V(droppedFrameThresholdMs())},
        {"recentSampleCount",V(recentSampleCount())},{"frameSampleCapacity",V(frameSampleCapacity())}};
    auto clock=s->epoch_milliseconds;values["epochMs"]=V(clock());return values;
}
ii::core::Result<void> RenderMonitor::observe(std::shared_ptr<const RenderMonitorChangePort> receiver){return impl_->observe(std::move(receiver));}
}
