#include "Gallery.generated.h"
#include <iiGUI/Application.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace ii;
using namespace std::chrono_literals;
namespace {
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
using View=lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;
    std::shared_ptr<gui::ColorPickerModel> model;std::weak_ptr<gui::ColorPickerModel> watched;
    gui::Widget status,worker,frames;bool test=false,background=false,resized=false,closed=false,closing=false;std::uint64_t frame=0;
};
void edit(gui::WidgetMount m,std::string_view id,std::string text){good(take(m.find(id)).focus());good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(m.dispatch_input({.kind=gui::WidgetInputKind::text,.text=std::move(text)}));good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));}
}
core::Result<void> runtime::configure_application(Application& app){
    auto state=std::make_shared<State>();state->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1&&!state->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    auto options=View::options();options.frame_loop=&app.frame_loop();auto sender=app.event_loop().sender();std::weak_ptr<State> weak=state;
    state->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{auto s=weak.lock();if(!s)return;auto m=s->view->widgets();s->status=take(m.find("status"));s->worker=take(m.find("worker"));s->frames=take(m.find("frames"));s->model=take(take(m.find("wheel.field")).color_model());s->watched=s->model;
            for(auto id:{"wheel.field","hs.field","cmyk.field"})check(take(take(m.find(id)).color_model())==s->model,"AOT composites do not share one model");
            check(bool(app.post([s,sender]{(void)sender.post([s]{s->background=true;good(s->worker.set_text("Background result: 42 · iiSDKCore"));});})),"Worker rejected");
            (void)take(app.frame_loop().add([s](const auto& frame){s->frame=frame.frame;if(frame.frame%15==0)good(s->frames.set_text("Core display frames: "+std::to_string(frame.frame)));if(s->test&&s->background&&s->resized&&frame.frame>=3&&!s->closing){s->closing=true;good(take(s->view->widgets().find("close")).activate());}}, {.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(s->test){edit(m,"wheel.hex","00E71F80");check(take(s->model->state()).hex=="00E71F"&&take(take(m.find("hs.hex")).snapshot()).text=="00E71F"&&take(take(m.find("cmyk.alpha")).snapshot()).text=="50","AOT HEX/alpha did not update other pickers");
                edit(m,"cmyk.channel.9.input","25");check(std::abs(take(s->model->channel_value(gui::ColorChannel::black))-25)<1e-6,"CMYK numeric input lost latent ink");good(take(m.find("wheel.apply")).activate());const auto baseline=take(s->model->state()).color;
                good(take(m.find("hs.recent.1")).activate());for(auto id:{"wheel.hex","hs.hex","cmyk.hex"})check(take(take(m.find(id)).snapshot()).text==take(s->model->state()).hex,"AOT action left a stale focused HEX editor");good(take(m.find("hs.cancel")).activate());check(take(s->model->state()).color==baseline,"Composite recent/apply/cancel failed");
                auto revision=take(s->model->state()).revision;edit(m,"hs.hex","bad");check(take(s->model->state()).revision==revision,"Invalid composite HEX changed model");good(take(m.find("wheel.eyedropper")).activate());check(take(s->status.snapshot()).text.find("Eyedropper requested")!=std::string::npos,"Typed eyedropper callback missing");
                auto pixels=take(s->host->capture_canvas());check(!pixels.rgba.empty()&&take(s->host->graphics_info()).api==gui::GraphicsApi::metal,"Picker composition did not render on Metal");good(s->host->set_content_size({1120,780}));check(bool(sender.after(5s,[]{throw std::runtime_error("Picker gallery completion watchdog");})),"Watchdog rejected");}
        },
        .resized=[weak](gui::Size size){if(auto s=weak.lock())s->resized=s->resized||size==gui::Size{1120,780};},
        .closed=[weak]{if(auto s=weak.lock()){s->closed=true;s->model.reset();check(NativePickerBackend::destroyed==0,"Backend retired before Core view teardown");}}
    }));
    auto verify=app.attach({"verification",[](Application&)->core::Result<void>{return {};},[state]()->core::Result<void>{check(NativePickerBackend::created==1&&NativePickerBackend::destroyed==1&&state->watched.expired(),"Picker backend/model did not retire once");if(state->test){check(state->closed&&state->background&&state->resized&&state->frame>=3,"Picker gallery completion incomplete");std::cout<<"LVRS_NATIVE_PICKER_GALLERY_OK composites=3 shared_model=true background=42 scopes=1/1 model_retired=true frames="<<state->frame<<'\n';}return {};}});if(!verify)return verify;
    return app.attach(gui::view_attachment("pickers",state->host,state->view,std::function<void(View::Scope&)>{[weak,&app](View::Scope& scope){scope.backend_0.report=[weak](std::string message){if(auto s=weak.lock();s&&!s->closed)good(s->status.set_text(message+" · #"+take(s->model->state()).hex));};scope.backend_0.on_close=[&app]{good(app.quit());};}}));
}
