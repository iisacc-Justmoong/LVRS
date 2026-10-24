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
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
void submitted(runtime::Submission r){check(bool(r),"Core color gallery submission rejected");}
using View=lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();
    std::shared_ptr<gui::Host> host;
    std::shared_ptr<gui::ColorPickerModel> model;
    std::weak_ptr<gui::ColorPickerModel> watched_model;
    std::shared_ptr<gui::ColorChangePort> receiver;
    gui::Widget color,channels,message,hex,hue,saturation,value,alpha,worker,frames;
    bool self_test=false,background=false,resized=false,closed=false,closing=false;
    std::uint64_t frame=0;
    int changes=0;
    void refresh(){
        if(closed || !model)return;
        const auto s=take(model->state());
        good(color.set_text("Color: #"+s.hex+" · alpha "+std::to_string(int(std::lround(s.color.alpha*100)))+"% · previous #"+hex_rgb(s.previous_color)));
        good(channels.set_text("H "+std::to_string(std::lround(s.hue))+"°  S "+std::to_string(std::lround(s.saturation*100))+"%  V "+std::to_string(std::lround(s.brightness*100))+"% · R "+
            std::to_string(std::lround(take(model->channel_value(gui::ColorChannel::red))))+"  G "+std::to_string(std::lround(take(model->channel_value(gui::ColorChannel::green))))+"  B "+
            std::to_string(std::lround(take(model->channel_value(gui::ColorChannel::blue))))+" · CMYK K "+std::to_string(std::lround(take(model->channel_value(gui::ColorChannel::black))))+"%"));
        good(hue.set_value(s.hue));good(saturation.set_value(s.saturation*100));good(value.set_value(s.brightness*100));good(alpha.set_value(s.color.alpha*100));
        if(!take(hex.snapshot()).focused)good(hex.set_text(s.hex));
    }
    static std::string hex_rgb(gui::WidgetColor c){constexpr char digits[]="0123456789ABCDEF";std::string out;for(auto v:{c.red,c.green,c.blue}){auto n=int(std::lround(v*255));out+=digits[n>>4];out+=digits[n&15];}return out;}
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto state=std::make_shared<State>();state->self_test=app.arguments().size()==2 && app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1 && !state->self_test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    auto options=View::options();options.frame_loop=&app.frame_loop();auto ui=app.event_loop().sender();auto weak=std::weak_ptr<State>(state);
    state->host=take(gui::attach_application(app,options,{
        .opened=[weak,ui,&app]{
            auto state=weak.lock();if(!state)return;
            auto widgets=state->view->widgets();
            state->color=take(widgets.find("color"));state->channels=take(widgets.find("channels"));state->message=take(widgets.find("message"));state->hex=take(widgets.find("hex"));
            state->hue=take(widgets.find("hue"));state->saturation=take(widgets.find("saturation"));state->value=take(widgets.find("value"));state->alpha=take(widgets.find("alpha"));state->worker=take(widgets.find("worker"));state->frames=take(widgets.find("frames"));
            state->model=take(take(widgets.find("wheel")).color_model());
            state->watched_model=state->model;
            for(auto id:{"wheel","hs","sv","spectrum","hueRail","valueRail"}){auto field=take(widgets.find(id));check(take(field.color_model())==state->model && take(field.snapshot()).native_class=="iiGUI.ColorPickerSurface","AOT fields do not share one native model");}
            auto weak=std::weak_ptr<State>(state);state->receiver=std::make_shared<gui::ColorChangePort>([weak](const auto&){if(auto s=weak.lock()){++s->changes;s->refresh();}});good(state->model->observe(state->receiver));state->refresh();
            submitted(app.post([state,ui]{(void)ui.post([state]{state->background=true;good(state->worker.set_text("Background result: 42 · iiSDKCore"));});}));
            (void)take(app.frame_loop().add([state](const auto& c){state->frame=c.frame;if(c.frame%15==0)good(state->frames.set_text("Core display frames: "+std::to_string(c.frame)));if(state->self_test && state->background && state->resized && c.frame>=3 && !state->closing){state->closing=true;good(take(state->view->widgets().find("close")).activate());}},
                {.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(state->self_test){
                const auto initial_hex=take(state->model->state()).hex;
                const auto rgb="R "+std::to_string(std::stoi(initial_hex.substr(0,2),nullptr,16))+"  G "+std::to_string(std::stoi(initial_hex.substr(2,2),nullptr,16))+"  B "+std::to_string(std::stoi(initial_hex.substr(4,2),nullptr,16));
                check(take(state->channels.snapshot()).text.find(rgb)!=std::string::npos,"Displayed RGB integers disagree with HEX rounding");
                good(take(widgets.find("red")).activate());check(take(state->model->state()).hex=="FF0000","Compiled RGB action failed");
                good(take(widgets.find("ink")).activate());check(std::abs(take(state->model->channel_value(gui::ColorChannel::black))-10)<1e-6,"Compiled CMYK action failed");
                good(state->hex.set_text("11223380"));good(take(widgets.find("commit")).activate());check(take(state->model->state()).hex=="112233" && std::abs(take(state->model->state()).color.alpha-128.0/255)<1e-6,"Compiled HEX/alpha commit failed");
                const auto revision=take(state->model->state()).revision;good(state->hex.set_text("invalid"));good(take(widgets.find("commit")).activate());check(take(state->model->state()).revision==revision && take(state->message.snapshot()).text=="Invalid HEX","Invalid HEX changed state or lacked feedback");
                good(state->hex.set_text("AABBCCFF"));good(take(widgets.find("commit")).activate());good(take(widgets.find("accept")).activate());good(take(widgets.find("red")).activate());good(take(widgets.find("cancel")).activate());check(take(state->model->state()).hex=="AABBCC" && take(state->color.snapshot()).text.find("AABBCC")!=std::string::npos,"Compiled apply/cancel or reactive color display failed");
                good(state->alpha.focus());good(widgets.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::left}));check(std::abs(take(state->model->state()).color.alpha-.99)<1e-6,"Compiled alpha control did not edit original model");
                check(state->changes>=7,"Model changes did not reach native receiver");auto pixels=take(state->host->capture_canvas());check(!pixels.rgba.empty() && take(state->host->graphics_info()).api==native_api,"Color gallery did not render natively");
                good(state->host->set_content_size({1040,800}));submitted(ui.after(5s,[state]{throw std::runtime_error("Color gallery watchdog: frames="+std::to_string(state->frame)+" background="+std::to_string(state->background)+" resized="+std::to_string(state->resized));}));
            }
        },
        .resized=[weak](gui::Size size){if(auto state=weak.lock())state->resized=state->resized || size==gui::Size{1040,800};},
        .closed=[weak]{if(auto state=weak.lock()){state->closed=true;state->receiver.reset();state->model.reset();check(NativeColorBackend::destroyed==0,"Color backend retired before Core attachments");}}}));
    auto verify=app.attach({"verification",[](Application&)->core::Result<void>{return {};},[state]()->core::Result<void>{check(NativeColorBackend::created==1 && NativeColorBackend::destroyed==1 && state->watched_model.expired(),"Color backend/model did not retire exactly once");if(state->self_test){check(state->closed && state->background && state->frame>=3,"Color gallery completion milestones missing");std::cout<<"LVRS_NATIVE_COLOR_OK fields=6 shared_model=true rgba_hex_cmyk=true background=42 closed=true scopes=1/1 model_retired=true frames="<<state->frame<<'\n';}return {};}});
    if(!verify)return verify;
    return app.attach(gui::view_attachment("color_gallery",state->host,state->view,std::function<void(View::Scope&)>{[weak,&app](View::Scope& scope){
        scope.backend_0.on_commit=[weak]{if(auto state=weak.lock();state && !state->closed){auto valid=take(state->model->edit_hex(take(state->hex.snapshot()).text));good(state->message.set_text(valid?"HEX committed":"Invalid HEX"));}};
        scope.backend_0.on_accept=[weak]{if(auto state=weak.lock();state && !state->closed)good(state->message.set_text("Applied · previous color updated"));};scope.backend_0.on_cancel=[weak]{if(auto state=weak.lock();state && !state->closed)good(state->message.set_text("Canceled · previous color restored"));};scope.backend_0.on_close=[&app]{good(app.quit());};
    }}));
}
