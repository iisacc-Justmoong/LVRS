#include "Gallery.generated.h"
#include <iiGUI/Application.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace ii;
using namespace std::chrono_literals;
namespace {
template<class T>T take(core::Result<T> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}return std::move(*r);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
using View=lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;std::shared_ptr<lvrs::Motion> motion;
    gui::Widget desktop,mobile,status,worker,frames,model_label;bool test=false,closed=false,background=false,resized=false,watchdog=false;
    int opened=0,hidden=0,applied=0,phase=0;std::uint64_t frame=0;
    gui::Widget get(std::string_view name){return take(view->widgets().find(name));}
    void refresh(){if(status.snapshot())good(status.set_text("Opened: "+std::to_string(opened)+" · closed: "+std::to_string(hidden)+" · applied: "+std::to_string(applied)));}
    void refresh_motion(){if(model_label.snapshot()){auto value=take(motion->state());good(model_label.set_text("Motion: "+std::to_string(int(value.speed))+"× · reduced "+(value.reduced_motion?"on":"off")));}}
    void drag(double distance){auto r=take(get("mobile.grabber").snapshot()).bounds;const auto x=r.x+r.width/2,y=r.y+r.height/2;auto mount=view->widgets();good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=x,.y=y}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=x,.y=y+distance}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=x,.y=y+distance}));}
    void tick(const runtime::FrameContext& time){
        frame=time.frame;if(frame%15==0)good(frames.set_text("Core display frames: "+std::to_string(frame)));
        if(!test||!background||!resized)return;
        if(phase==1&&!take(mobile.snapshot()).transitioning){
            auto r=take(mobile.snapshot()).bounds;check(r.width==820&&r.height==353&&r.y==267,"AOT mobile medium geometry wrong");good(get("mobileInput").set_text("Retained mobile edit"));auto v=take(get("mobile.viewport").snapshot()).bounds;
            good(view->widgets().dispatch_input({.kind=gui::WidgetInputKind::scroll,.x=v.x+30,.y=v.y+20,.scroll_y=48}));check(take(get("mobile.viewport").snapshot()).scroll_offset>0,"AOT native body did not scroll");take(host->capture_canvas());drag(35);check(take(mobile.snapshot()).transitioning,"Native short drag did not animate");phase=2;
        }else if(phase==2&&!take(mobile.snapshot()).transitioning){drag(130);check(!take(mobile.snapshot()).opened&&take(mobile.snapshot()).exiting,"Native grabber dismissal did not enter exit");phase=3;
        }else if(phase==3&&!take(mobile.snapshot()).visible){
            good(motion->set_reduced_motion(true));good(get("openMobile").activate());check(!take(mobile.snapshot()).transitioning&&take(get("mobileInput").snapshot()).text=="Retained mobile edit"&&take(get("mobile.viewport").snapshot()).scroll_offset==0,"Reduced-motion reopen recreated content or retained scroll");good(mobile.close_popup());good(motion->set_reduced_motion(false));phase=4;good(get("close").activate());
        }
    }
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto state=std::make_shared<State>();state->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1&&!state->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    std::weak_ptr<State> weak=state;auto sender=app.event_loop().sender();auto options=View::options();options.frame_loop=&app.frame_loop();
    state->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{auto s=weak.lock();if(!s)return;s->desktop=s->get("desktop");s->mobile=s->get("mobile");s->status=s->get("status");s->worker=s->get("worker");s->frames=s->get("frames");s->model_label=s->get("motion");s->refresh();
            check(bool(app.post([weak,sender]{(void)sender.post([weak]{if(auto s=weak.lock();s&&!s->closed){s->background=true;good(s->worker.set_text("Background result: 42 · iiSDKCore"));}});})),"Worker rejected");
            (void)take(app.frame_loop().add([s,sender](const auto& frame){if(s->test&&!s->watchdog){s->watchdog=true;std::weak_ptr<State> weak=s;check(bool(sender.after(5s,[weak]{if(auto s=weak.lock();s&&!s->closed)throw std::runtime_error("Sheet completion watchdog: phase="+std::to_string(s->phase)+" frame="+std::to_string(s->frame)+" background="+std::to_string(s->background)+" resized="+std::to_string(s->resized));})),"Watchdog rejected");}s->tick(frame);},{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(!s->test)return;check(s->opened==1&&take(s->desktop.snapshot()).opened,"AOT initial Sheet open missing");auto editor=s->get("desktopInput");auto id=take(editor.snapshot()).id;good(editor.set_text("Retained desktop edit"));good(s->get("apply").activate());check(s->applied==1&&take(s->desktop.snapshot()).opened,"AOT apply auto-closed Sheet");good(s->desktop.close_popup());
            for(int i=0;i<10;++i){good(s->get("openDesktop").activate());check(take(editor.snapshot()).id==id&&take(editor.snapshot()).text=="Retained desktop edit"&&take(s->get("desktop.viewport").snapshot()).scroll_offset==0,"AOT reopen recreated retained content");good(s->get("desktop.viewport").set_scroll_offset(60));good(s->desktop.close_popup());}
            check(s->opened==11&&s->hidden==11,"AOT desktop lifecycle count wrong");good(s->host->set_content_size({820,620}));good(s->get("openMobile").activate());s->phase=1;
        },
        .resized=[weak](gui::Size size){if(auto s=weak.lock())s->resized|=size==gui::Size{820,620};},
        .closed=[weak]{if(auto s=weak.lock()){s->closed=true;check(SheetBackend::destroyed==0,"Backend retired before Core detach");}}
    }));
    auto verify=app.attach({"verification",[](Application&)->core::Result<void>{return {};},[state]()->core::Result<void>{check(SheetBackend::created==1&&SheetBackend::destroyed==1,"Backend did not retire once");if(state->test){check(state->closed&&state->background&&state->resized&&state->frame>=3&&state->phase==4&&state->opened==13&&state->hidden==13,"Sheet milestones incomplete");std::cout<<"LVRS_NATIVE_SHEET_GALLERY_OK opens=13 closed=13 parent=1/1 persistent_body=true native_scroll=true mobile_drag=true live_reduced_motion=true background=42 frames="<<state->frame<<'\n';}return {};}});if(!verify)return verify;
    return app.attach(gui::view_attachment("sheet",state->host,state->view,std::function<void(View::Scope&)>{[weak,&app](View::Scope& scope){auto& b=scope.backend_0;if(auto s=weak.lock())s->motion=b.motion;
        b.on_desktop=[weak]{if(auto s=weak.lock()){good(s->get("openDesktop").focus());good(s->desktop.open_at(0,0));}};
        b.on_mobile=[weak]{if(auto s=weak.lock()){good(s->get("openMobile").focus());good(s->mobile.open_at(0,0));}};
        b.on_opened=[weak]{if(auto s=weak.lock()){++s->opened;s->refresh();}};b.on_closed=[weak]{if(auto s=weak.lock()){++s->hidden;s->refresh();}};b.on_apply=[weak]{if(auto s=weak.lock()){++s->applied;s->refresh();}};
        b.on_edited=[weak](std::string value){if(auto s=weak.lock())good(s->status.set_text("Editor: "+value));};b.on_motion_changed=[weak]{if(auto s=weak.lock())s->refresh_motion();};b.on_close=[&app]{good(app.quit());};
    }}));
}
