#include <LVRS/Alert.h>
#include <LVRS/Theme.h>
#include <iiSDKCore/Runtime/FrameLoop.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace ii;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>void good(const T& r){if(!r){if constexpr(requires{r.error().cause;}){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}else throw std::runtime_error("Task rejected");}}
template<class T>T take(core::Result<T> r){good(r);return std::move(*r);}
void tokens(){
    check(lvrs::MotionTokens::press==90ms&&lvrs::MotionTokens::hover==160ms&&lvrs::MotionTokens::release==360ms&&lvrs::MotionTokens::button_release==180ms&&lvrs::MotionTokens::surface==420ms&&lvrs::MotionTokens::exit==150ms&&lvrs::MotionTokens::color==130ms&&lvrs::MotionTokens::overshoot==1.45,"Original LVRS Motion tokens differ");
    lvrs::Motion model;good(model.set_speed(2));check(take(model.duration(lvrs::MotionTokens::surface))==210ms,"Native Motion duration did not use shared speed");
}
void alert(){
    runtime::EventLoop events;runtime::FrameLoop frames(events);gui::Host host({.size={900,700},.event_loop=&events,.frame_loop=&frames});lvrs::Alert alert;auto preferences=std::make_shared<lvrs::Motion>();int closed=0;
    good(host.run({.opened=[&]{
        auto mount=take(host.mount_widgets([&](gui::WidgetBuilder& builder)->core::Result<void>{auto root=take(builder.vstack({},"root"));alert=take(lvrs::Alert::create(builder,root,"alert",{.motion=preferences},{.closed=[&]{++closed;}}));return {};},lvrs::Theme().appearance()));
        auto now=runtime::Clock::now();good(alert.open());auto start=take(alert.root().snapshot());check(start.reveal_progress==0&&start.transitioning,"Default Alert did not start native entry motion");
        take(host.capture_canvas());auto initial=take(host.graphics_info());check(initial.shadow_blurs>0&&initial.backdrop_blurs>0,"Authored unscaled shadow/glass were absent at the first frame");
        (void)frames.step(now+90ms);take(host.capture_canvas());auto part=take(alert.root().snapshot());check(part.reveal_progress>0&&part.reveal_progress<1&&part.visual_bounds.width<part.bounds.width,"Alert did not animate its card on Core frames");
        (void)frames.step(now+210ms);auto overshoot=take(alert.root().snapshot());check(overshoot.reveal_progress>1&&overshoot.visual_bounds.width>overshoot.bounds.width,"Original OutBack overshoot was lost");take(host.capture_canvas());auto reused=take(host.graphics_info());
        if(initial.api==gui::GraphicsApi::metal){
            std::cerr<<"METAL_MOTION_COUNTS shadow="<<initial.shadow_blurs<<"->"<<reused.shadow_blurs<<" backdrop="<<initial.backdrop_blurs<<"->"<<reused.backdrop_blurs<<" foreground="<<initial.foreground_rasterizations<<"->"<<reused.foreground_rasterizations<<'\n';
            check(reused.shadow_blurs==initial.shadow_blurs&&reused.backdrop_blurs==initial.backdrop_blurs&&reused.foreground_rasterizations==initial.foreground_rasterizations,"Metal pure motion re-rasterized retained effects");
            check(reused.shadow_masks==initial.shadow_masks&&reused.backdrop_copies==initial.backdrop_copies,"Metal pure motion regenerated invariant masks or source captures");
        }
        else check(reused.shadow_blurs>initial.shadow_blurs&&reused.backdrop_blurs>initial.backdrop_blurs,"Inline native renderer did not execute the changed effect frame");
        good(alert.close());auto exit=take(alert.root().snapshot());check(closed==1&&exit.exiting&&exit.visible&&!exit.enabled&&exit.input_blocked,"Close did not retain pixels and revoke input immediately");take(host.capture_canvas());check(take(host.graphics_info()).backdrop_texture_bytes==0,"Glass remained active during the retained exit");
        good(preferences->set_reduced_motion(true));check(!take(alert.root().snapshot()).visible&&!take(alert.root().snapshot()).transitioning,"Reduced motion did not immediately retire the closing card");
        good(preferences->set_reduced_motion(false));good(alert.open());(void)frames.step(now+300ms);good(alert.set_motion_enabled(false));check(take(alert.root().snapshot()).reveal_progress==1&&!take(alert.root().snapshot()).transitioning,"Local motionEnabled did not settle the entry");
        good(alert.close());check(!take(alert.root().snapshot()).visible,"Disabled native Alert retained exit pixels");good(host.request_close());
    }}));
}
}
int main(int argc,char** argv){try{std::string name=argc>1?argv[1]:"tokens";if(name=="tokens")tokens();else if(name=="alert")alert();else throw std::runtime_error("Unknown Motion test");std::cout<<"NATIVE_MOTION_OK "<<name<<'\n';}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
