#include "Gallery.generated.h"
#include <iiGUI/Application.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace ii;
using namespace std::chrono_literals;
namespace {
#if defined(__APPLE__)
constexpr auto native_api=gui::GraphicsApi::metal;
#elif defined(_WIN32)
constexpr auto native_api=gui::GraphicsApi::direct3d11;
#else
constexpr auto native_api=gui::GraphicsApi::vulkan;
#endif
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
using View=lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;std::shared_ptr<gui::ColorPickerModel> model;std::weak_ptr<gui::ColorPickerModel> watched;
    std::shared_ptr<gui::ColorChangePort> receiver;gui::Widget popup,anchor,color,status,worker,frames;
    bool test=false,applied=false,background=false,resized=false,closed=false,closing=false;std::uint64_t frame=0;int opened=0,dismissed=0,outside=0;
    void refresh(){if(closed||!model)return;auto s=take(model->state());good(color.set_text("Color: #"+s.hex+" · opacity "+std::to_string(std::lround(s.color.alpha*100))+"%"));}
};
void edit(gui::WidgetMount m,std::string text){good(take(m.find("picker.hex")).focus());good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(m.dispatch_input({.kind=gui::WidgetInputKind::text,.text=std::move(text)}));good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));}
}
core::Result<void> runtime::configure_application(Application& app){
    auto state=std::make_shared<State>();state->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";if(app.arguments().size()>1&&!state->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    auto options=View::options();options.frame_loop=&app.frame_loop();auto sender=app.event_loop().sender();std::weak_ptr<State> weak=state;
    state->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{auto s=weak.lock();if(!s)return;auto m=s->view->widgets();s->popup=take(m.find("editor"));s->anchor=take(m.find("open"));s->color=take(m.find("color"));s->status=take(m.find("status"));s->worker=take(m.find("worker"));s->frames=take(m.find("frames"));s->model=take(take(m.find("picker.field")).color_model());s->watched=s->model;s->receiver=std::make_shared<gui::ColorChangePort>([weak](const auto&){if(auto s=weak.lock())s->refresh();});good(s->model->observe(s->receiver));s->refresh();
            check(bool(app.post([s,sender]{(void)sender.post([s]{s->background=true;good(s->worker.set_text("Background result: 42 · iiSDKCore"));});})),"Worker rejected");
            (void)take(app.frame_loop().add([s](const auto& f){s->frame=f.frame;if(f.frame%15==0)good(s->frames.set_text("Core display frames: "+std::to_string(f.frame)));if(s->test&&s->background&&s->resized&&f.frame>=3&&!s->closing){s->closing=true;good(take(s->view->widgets().find("close")).activate());}}, {.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(s->test){auto preview=take(s->anchor.snapshot());check(preview.color_button&&preview.size==gui::Size{36,36}&&preview.color_state->hex=="874AFF","LVUI did not mount the native shared-model color button");check(!take(s->popup.snapshot()).opened,"Popover initially exposed");good(s->anchor.activate());edit(m,"00E71F80");good(take(m.find("picker.apply")).activate());auto baseline=take(s->model->state()).color;check(!take(s->popup.snapshot()).opened&&take(s->model->state()).hex=="00E71F"&&take(s->anchor.snapshot()).color_state->color==baseline,"Apply did not commit/dismiss/update preview");
                good(s->anchor.activate());edit(m,"FF0000");auto r=take(take(m.find("outside")).snapshot()).bounds;good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+2,.y=r.y+2}));good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+2,.y=r.y+2}));check(!take(s->popup.snapshot()).opened&&s->outside==1&&take(s->model->state()).color==baseline,"Outside close did not restore baseline and forward click");
                good(s->anchor.activate());edit(m,"11223380");good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::escape}));check(!take(s->popup.snapshot()).opened&&take(s->model->state()).color==baseline&&take(s->anchor.snapshot()).color_state->color==baseline,"Child Escape did not cancel owner popup/preview");
                good(s->anchor.activate());auto pixels=take(s->host->capture_canvas());check(!pixels.rgba.empty()&&take(s->host->graphics_info()).api==native_api,"Popover did not render native content");good(s->host->set_content_size({820,720}));good(s->popup.close_popup());check(s->opened==4&&s->dismissed==4&&NativePopoverBackend::created==1&&NativePopoverBackend::destroyed==0,"Reopening reconstructed backend or lost callbacks");check(bool(sender.after(5s,[]{throw std::runtime_error("Popover gallery completion watchdog");})),"Watchdog rejected");}
        },
        .resized=[weak](gui::Size size){if(auto s=weak.lock())s->resized=s->resized||size==gui::Size{820,720};},
        .closed=[weak]{if(auto s=weak.lock()){s->closed=true;s->receiver.reset();s->model.reset();check(NativePopoverBackend::destroyed==0,"Backend retired before Core view teardown");}}
    }));
    auto verify=app.attach({"verification",[](Application&)->core::Result<void>{return {};},[state]()->core::Result<void>{check(NativePopoverBackend::created==1&&NativePopoverBackend::destroyed==1&&state->watched.expired(),"Popover backend/model did not retire once");if(state->test){check(state->closed&&state->background&&state->resized&&state->frame>=3,"Popover milestones incomplete");std::cout<<"LVRS_NATIVE_POPOVER_GALLERY_OK opens="<<state->opened<<" closes="<<state->dismissed<<" scopes=1/1 model_retired=true outside_forwarded=true background=42 frames="<<state->frame<<'\n';}return {};}});if(!verify)return verify;
    return app.attach(gui::view_attachment("popover",state->host,state->view,std::function<void(View::Scope&)>{[weak,&app](View::Scope& scope){
        scope.backend_0.on_open=[weak]{if(auto s=weak.lock();s&&!s->closed){if(!take(s->popup.snapshot()).opened){good(s->model->accept());s->applied=false;good(s->anchor.focus());}auto anchor=take(s->anchor.snapshot());good(s->popup.open_for(s->anchor,0,anchor.size.height+8));}};
        scope.backend_0.on_opened=[weak]{if(auto s=weak.lock();s&&!s->closed){++s->opened;good(s->status.set_text("Opened "+std::to_string(s->opened)+" · same C++ content"));}};
        scope.backend_0.on_closed=[weak]{if(auto s=weak.lock();s&&!s->closed){++s->dismissed;if(!s->applied)good(s->model->cancel());good(s->status.set_text(s->applied?"Applied · popover closed":"Canceled/dismissed · baseline restored"));}};
        scope.backend_0.on_apply=[weak]{if(auto s=weak.lock();s&&!s->closed){s->applied=true;good(s->popup.close_popup());}};scope.backend_0.on_cancel=[weak]{if(auto s=weak.lock();s&&!s->closed)good(s->popup.close_popup());};
        scope.backend_0.on_outside=[weak]{if(auto s=weak.lock();s&&!s->closed){if(take(s->popup.snapshot()).opened)good(s->popup.close_popup());++s->outside;good(s->status.set_text("Outside action: "+std::to_string(s->outside)));}};scope.backend_0.on_close=[&app]{good(app.quit());};
    }}));
}
