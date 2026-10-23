#include "Gallery.generated.h"
#include <iiGUI/Application.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace ii;
using namespace std::chrono_literals;
namespace {
template<class T>T take(core::Result<T> result){
    if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}
    return std::move(*result);
}
void good(core::Result<void> result){if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}}
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
using View=lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();
    std::shared_ptr<gui::Host> host;
    std::shared_ptr<lvrs::Motion> motion;
    gui::Widget modal,open,status,worker,frames,motion_label,title;
    bool test=false,closed=false,background=false,resized=false,closing=false,watchdog_armed=false;
    int opened=0,retired=0,canceled=0,primary=0,outside=0,motion_phase=0;
    std::uint64_t frame=0;
    std::string chrome_id;
    void refresh(){if(status.snapshot())good(status.set_text("Opened: "+std::to_string(opened)+" · closed: "+std::to_string(retired)+" · canceled: "+std::to_string(canceled)+" · primary: "+std::to_string(primary)));}
    void refresh_motion(){if(motion_label.snapshot()){auto value=take(motion->state());good(motion_label.set_text("Motion: "+std::to_string(int(value.speed))+"× · reduced "+(value.reduced_motion?"on":"off")));}}
    void tick(const runtime::FrameContext& context){
        frame=context.frame;
        if(frame%15==0)good(frames.set_text("Core display frames: "+std::to_string(frame)));
        if(!test||!background||!resized||closing)return;
        if(motion_phase==1){
            auto visible=take(modal.snapshot());
            if(!visible.transitioning&&visible.reveal_progress==1){
                auto body=take(modal.popup_content("input"));auto previous=take(body.snapshot()).id;
                good(modal.close_popup());auto exiting=take(modal.snapshot());
                check(exiting.exiting&&exiting.visible&&exiting.bounds.height<visible.bounds.height&&!body.snapshot()&&take(title.snapshot()).visible,"Native exit did not retire body and retain shrinking chrome");
                good(motion->set_reduced_motion(true));check(!take(modal.snapshot()).visible,"Shared AOT motion model did not finish exit immediately");
                good(open.activate());check(!take(modal.snapshot()).transitioning&&take(modal.popup_content("input")).snapshot()->id!=previous&&take(title.snapshot()).id==chrome_id,"Reduced-motion reopen did not keep chrome and replace body");
                good(motion->set_reduced_motion(false));good(modal.close_popup());motion_phase=2;
            }
        }else if(motion_phase==2&&!take(modal.snapshot()).visible){
            motion_phase=3;closing=true;good(take(view->widgets().find("close")).activate());
        }
    }
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto state=std::make_shared<State>();
    state->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1&&!state->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    std::weak_ptr<State> weak=state;
    auto options=View::options();options.frame_loop=&app.frame_loop();auto sender=app.event_loop().sender();
    state->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{
            auto state=weak.lock();if(!state)return;auto mount=state->view->widgets();
            state->modal=take(mount.find("modal"));state->open=take(mount.find("open"));state->status=take(mount.find("status"));
            state->worker=take(mount.find("worker"));state->frames=take(mount.find("frames"));state->motion_label=take(mount.find("motion"));
            state->title=take(mount.find("modal.title"));state->chrome_id=take(state->title.snapshot()).id;state->refresh();
            check(bool(app.post([weak,sender]{(void)sender.post([weak]{if(auto state=weak.lock();state&&!state->closed){state->background=true;good(state->worker.set_text("Background result: 42 · iiSDKCore"));}}); })),"Worker rejected");
            (void)take(app.frame_loop().add([state,sender](const auto& context){
                if(state->test&&!state->watchdog_armed){
                    state->watchdog_armed=true;std::weak_ptr<State> weak_state=state;
                    check(bool(sender.after(5s,[weak_state]{
                        auto state=weak_state.lock();if(!state||state->closed)return;
                        auto popup=take(state->modal.snapshot());
                        throw std::runtime_error("Modal frame completion watchdog: phase="+std::to_string(state->motion_phase)+" frames="+std::to_string(state->frame)+" reveal="+std::to_string(popup.reveal_progress)+" transition="+std::to_string(popup.transitioning)+" open="+std::to_string(popup.opened)+" background="+std::to_string(state->background)+" resized="+std::to_string(state->resized));
                    })),"Frame watchdog rejected");
                }
                state->tick(context);
            },{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(!state->test)return;
            check(state->opened==1&&take(state->modal.snapshot()).opened,"AOT initial open did not execute after mount");
            auto old=take(state->modal.popup_content("input"));auto first=take(old.snapshot()).id;good(old.set_text("Changed"));
            good(take(state->modal.popup_content("modal.primary")).activate());check(state->primary==1&&take(state->modal.snapshot()).opened,"Primary auto-closed dialog");
            good(take(state->modal.popup_content("modal.secondary")).activate());check(state->retired==1&&state->canceled==0&&!old.snapshot()&&bool(state->title.snapshot()),"Programmatic close retired chrome or kept body");
            good(state->open.activate());auto second=take(state->modal.popup_content("input"));check(take(second.snapshot()).text=="Fresh editor"&&take(second.snapshot()).id!=first,"Body retained previous activation");
            good(second.focus());good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::escape}));check(state->canceled==1&&state->retired==2,"Escape cancellation not delivered");
            good(state->open.activate());good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=2,.y=2}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=2,.y=2}));
            check(state->outside==0&&state->canceled==2&&state->retired==3,"Outside dismissal leaked input or lost cancellation");
            good(state->open.activate());auto active=take(state->modal.popup_content("input"));auto before=take(active.snapshot()).id;
            good(state->host->set_content_size({680,720}));check(take(state->modal.snapshot()).bounds.width==632&&take(active.snapshot()).id==before&&state->opened==4,"Resize recreated content or lost geometry");
            check(!take(state->host->capture_canvas()).rgba.empty(),"Modal did not render");good(take(state->modal.popup_content("modal.primary")).activate());good(state->modal.close_popup());
            check(state->retired==4&&state->primary==2&&state->canceled==2&&take(state->title.snapshot()).id==state->chrome_id,"Four fast activation checks changed static chrome");
            good(state->open.activate());state->motion_phase=1;

        },
        .resized=[weak](gui::Size size){if(auto state=weak.lock())state->resized|=size==gui::Size{680,720};},
        .closed=[weak]{if(auto state=weak.lock()){state->closed=true;check(ModalBackend::destroyed==0,"Backend retired before Core view detach");}}
    }));
    auto verify=app.attach({"verification",[](Application&)->core::Result<void>{return {};},[state]()->core::Result<void>{
        check(ModalBackend::created==1&&ModalBackend::destroyed==1,"Parent backend did not retire once");
        if(state->test){check(state->closed&&state->background&&state->resized&&state->frame>=3&&state->motion_phase==3&&state->opened==6&&state->retired==6,"Modal milestones incomplete");
            std::cout<<"LVRS_NATIVE_MODAL_GALLERY_OK opens=6 closed=6 canceled=2 primary=2 parent=1/1 fresh_aot_body=true persistent_chrome=true native_motion=true live_reduced_motion=true background=42 frames="<<state->frame<<'\n';}
        return {};
    }});if(!verify)return verify;
    return app.attach(gui::view_attachment("modal",state->host,state->view,std::function<void(View::Scope&)>{[weak,&app](View::Scope& scope){
        auto& backend=scope.backend_0;if(auto state=weak.lock())state->motion=backend.motion;
        backend.on_motion_changed=[weak]{if(auto state=weak.lock())state->refresh_motion();};
        backend.on_open=[weak]{if(auto state=weak.lock()){good(state->open.focus());good(state->modal.open_at(0,0));}};
        backend.on_opened=[weak]{if(auto state=weak.lock()){++state->opened;state->refresh();}};
        backend.on_closed=[weak]{if(auto state=weak.lock()){++state->retired;state->refresh();}};
        backend.on_canceled=[weak]{if(auto state=weak.lock()){++state->canceled;state->refresh();}};
        backend.on_primary=[weak]{if(auto state=weak.lock()){++state->primary;state->refresh();}};
        backend.on_secondary=[weak]{if(auto state=weak.lock())good(state->modal.close_popup());};
        backend.on_tertiary=[weak]{if(auto state=weak.lock())good(take(state->modal.popup_content("input")).set_text("Fresh editor"));};
        backend.on_edited=[weak](std::string value){if(auto state=weak.lock())good(state->status.set_text("Editor: "+value));};
        backend.on_moved=[weak](double value){if(auto state=weak.lock())good(state->status.set_text("Slider: "+std::to_string(int(value))));};
        backend.on_outside=[weak]{if(auto state=weak.lock()){++state->outside;good(state->status.set_text("Background action: "+std::to_string(state->outside)));}};
        backend.on_close=[&app]{good(app.quit());};
    }}));
}
