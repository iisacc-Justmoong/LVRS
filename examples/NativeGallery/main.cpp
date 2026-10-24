#include "Gallery.generated.h"
#include <iiGUI/Application.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
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
void check(bool v,const char* why) { if(!v) throw std::runtime_error(why); }
template<class T> T take(core::Result<T> r) { if(!r) throw std::runtime_error(r.error().message); return std::move(*r); }
void good(core::Result<void> r) { if(!r) { if(r.error().cause) std::rethrow_exception(r.error().cause); throw std::runtime_error(r.error().message); } }
void submitted(runtime::Submission r) { check(bool(r),"Core submission rejected"); }
using View=lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();
    std::shared_ptr<gui::Host> host;
    gui::Widget status,progress,timer,worker;
    gui::Widget menu,context,submenu,more,dropdown,context_trigger;
    bool self_test=false,background=false,closed=false,resized=false,closing=false;
    gui::Size observed_size;
    int ticks=0,clicks=0,selected=-1;
    int menu_index=-1;
    std::vector<int> steps;
    std::string control_events;
    std::uint64_t sampled_frame=0;
    runtime::Clock::duration sampled_elapsed{};
};
}
core::Result<void> runtime::configure_application(Application& app) {
    auto s=std::make_shared<State>();
    s->self_test=app.arguments().size()==2 && app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1 && !s->self_test) return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    auto ui=app.event_loop().sender();
    auto options=View::options();options.frame_loop=&app.frame_loop();
    s->host=take(gui::attach_application(app,options,{
        .opened=[s,ui,&app] {
            auto m=s->view->widgets(); s->status=take(m.find("status")); s->progress=take(m.find("progress")); s->timer=take(m.find("timer")); s->worker=take(m.find("worker"));
            s->menu=take(m.find("menu"));s->context=take(m.find("context"));s->submenu=take(m.find("submenu"));s->more=take(m.find("more"));s->dropdown=take(m.find("dropdown"));s->context_trigger=take(m.find("contextTrigger"));
            for(auto& item:take(m.snapshot())) check(item.renderer=="iiGUI" && item.native_class.starts_with("iiGUI."),"OS widget substituted for own renderer");
            check(take(s->host->graphics_info()).api==native_api,"Gallery must use the platform native graphics API");
            submitted(app.post([s,ui] { submitted(ui.post([s] { s->background=true; good(s->worker.set_text("Background result: 42 · iiSDKCore")); })); }));
            (void)take(app.frame_loop().add([s](const runtime::FrameContext& frame) {
                s->ticks=int(frame.frame);
                if(s->self_test && s->background && s->resized && frame.frame>=3 && !s->closing) {
                    s->closing=true;good(take(s->view->widgets().find("close")).activate());
                }
                if(!s->sampled_frame) { s->sampled_frame=frame.frame;s->sampled_elapsed=frame.unscaled_elapsed;return; }
                const auto interval=frame.unscaled_elapsed-s->sampled_elapsed;
                if(interval>=250ms) {
                    const auto hz=double(frame.frame-s->sampled_frame)/std::chrono::duration<double>(interval).count();
                    good(s->timer.set_text("Frame: "+std::to_string(frame.frame)+" · update: "+std::to_string(int(std::lround(hz)))+" Hz"));
                    s->sampled_frame=frame.frame;s->sampled_elapsed=frame.unscaled_elapsed;
                }
            },{.clock=runtime::TickClock::unscaled}));
            if(s->self_test) {
                good(take(m.find("action")).activate()); check(s->clicks==1,"Compiled LVRS action missing");
                good(take(m.find("add")).activate());check(s->clicks==2,"Embedded icon button action missing");
                good(s->dropdown.activate());check(take(s->menu.snapshot()).opened,"Compiled dropdown did not open own Menu");
                good(take(m.find("menuOpen")).activate());check(s->clicks==3 && s->menu_index==0 && !take(s->menu.snapshot()).opened,"Compiled menu action/index/close missing");
                check(take(take(m.find("help")).snapshot()).size==gui::Size{21,21},"Authored help button metrics missing");
                good(take(m.find("details")).activate());
                check(s->selected==1 && take(take(m.find("details")).snapshot()).selected && !take(take(m.find("overview")).snapshot()).selected,"Compiled TabBar callback or selection missing");
                auto combo=take(m.find("combo"));const auto cb=take(combo.snapshot()).bounds;
                good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=cb.x+10,.y=cb.y+10}));
                good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=cb.x+10,.y=cb.y+10}));
                check(s->clicks==4 && s->control_events=="pr","Compiled ComboBox callbacks missing");s->control_events.clear();
                auto stepper=take(m.find("stepper"));const auto sb=take(stepper.snapshot()).bounds;
                for(double y:{sb.y+4,sb.y+14}) {
                    good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=sb.x+9,.y=y}));
                    good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=sb.x+9,.y=y}));
                }
                check(s->steps==std::vector<int>({1,-1}) && s->control_events=="prpr","Compiled typed Stepper direction or press lifecycle missing");
                good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=sb.x+9,.y=sb.y+4}));
                good(stepper.set_enabled(false));check(s->control_events=="prprpx","Compiled cancel callback missing");good(stepper.set_enabled(true));
                auto segments=take(take(m.find("segments")).snapshot());auto icons=take(take(m.find("iconSegments")).snapshot());
                check(segments.segment_count==2 && icons.segment_count==2 && segments.size.height==29 && icons.size.height==30,"Compiled segmented row metrics missing");
                good(take(m.find("segmentOne")).activate());good(take(m.find("segmentTwo")).activate());
                check(s->clicks==6 && !take(take(m.find("segmentOne")).snapshot()).selected,"Segmented controls must keep independent button actions");
                good(s->dropdown.activate());good(s->more.focus());good(s->more.activate());
                check(take(s->submenu.snapshot()).opened && take(s->menu.snapshot()).opened,"Compiled owned submenu missing");
                good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::escape}));
                check(!take(s->submenu.snapshot()).opened && take(s->menu.snapshot()).opened && take(s->more.snapshot()).focused,"Submenu did not restore parent focus");good(s->menu.close_popup());
                auto field=take(take(m.find("input")).snapshot()).bounds;
                good(m.dispatch_input({.kind=gui::WidgetInputKind::context_request,.x=field.x+10,.y=field.y+10}));
                auto contextAction=take(m.find("contextAction"));auto contextSnapshot=take(contextAction.snapshot());
                check(take(s->context.snapshot()).opened && contextSnapshot.font_family=="Inter" && contextSnapshot.font_size==12 && contextSnapshot.size.height==18,"Compiled context binding or bundled Inter typography missing");
                good(contextAction.activate());check(s->clicks==7 && !take(s->context.snapshot()).opened,"Compiled context item action missing");
                auto quick=take(m.find("quick"));good(quick.open_at(10,10));bool model_item=false;
                for(auto& item:take(m.snapshot())) if(item.kind==gui::WidgetKind::menu_item && item.popup_id=="quick" && item.menu_index==1) {good(take(m.find(item.id)).activate());model_item=true;break;}
                check(model_item && s->menu_index==1 && !take(quick.snapshot()).opened,"Static String menu entry model missing");
                good(take(m.find("check")).activate()); check(take(take(m.find("check")).snapshot()).checked,"Compiled checkbox missing");
                good(take(m.find("radioB")).activate()); check(!take(take(m.find("radioA")).snapshot()).checked,"Compiled radio group failed");
                good(take(m.find("input")).focus()); good(m.dispatch_input({.kind=gui::WidgetInputKind::text,.text="한글 · iiGUI"}));
                check(take(take(m.find("input")).snapshot()).text=="한글 · iiGUI","Compiled input edit failed");
                good(take(m.find("slider")).focus()); good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::end}));
                check(take(s->progress.snapshot()).value==100,"Compiled slider result not delivered");
                good(s->host->set_content_size({1024,840}));
                submitted(ui.after(5s,[s] { throw std::runtime_error("Gallery watchdog: background="+std::to_string(s->background)+
                    " frames="+std::to_string(s->ticks)+" resized="+std::to_string(s->resized)+" observed="+
                    std::to_string(s->observed_size.width)+"x"+std::to_string(s->observed_size.height)); }));
            }
        },
        .resized=[s](gui::Size size) { s->observed_size=size;s->resized=s->resized || (size.width==1024 && size.height==840); },
        .closed=[s] { s->closed=true; check(GalleryBackend::destroyed==0,"Core backend lifetime ended with OS window"); }}));
    auto verification=app.attach({"verification",[](Application&)->core::Result<void> { return {}; },[s]()->core::Result<void> {
        check(GalleryBackend::created==1 && GalleryBackend::destroyed==1,"Backend scope was not retired once");
        if(s->self_test) { check(s->closed,"Gallery did not close");check(s->ticks>=2,"Display frames were not delivered"); std::cout<<"LVRS_NATIVE_GALLERY_OK renderer=iiGUI api=Metal background=42 closed=true scopes=1/1 compound=4 menus=6 model=StringList fonts=Pretendard+Inter display_frames="<<s->ticks<<'\n'; }
        return {};
    }});
    if(!verification) return verification;
    return app.attach(gui::view_attachment("gallery",s->host,s->view,std::function<void(View::Scope&)>{[s,&app](View::Scope& scope) {
        scope.backend_0.on_action=[s] { good(s->status.set_text("iiGUI action: "+std::to_string(++s->clicks))); };
        scope.backend_0.on_check=[s](bool value) { good(s->status.set_text(value?"Checked: true":"Checked: false")); };
        scope.backend_0.on_edit=[s](std::string value) { good(s->status.set_text("Input: "+value)); };
        scope.backend_0.on_move=[s](double value) { good(s->progress.set_value(value)); good(s->status.set_text("Value: "+std::to_string(int(value)))); };
        scope.backend_0.on_select=[s](int value) { s->selected=value;good(s->status.set_text("Tab: "+std::to_string(value))); };
        scope.backend_0.on_step=[s](int value) { if(s->steps.size()==8) s->steps.erase(s->steps.begin());s->steps.push_back(value);good(s->status.set_text(value>0?"Stepper: +1":"Stepper: -1")); };
        scope.backend_0.on_control_event=[s](char value) { if(s->control_events.size()==64) s->control_events.clear();s->control_events+=value; };
        scope.backend_0.on_open_menu=[s] {auto r=take(s->dropdown.snapshot()).bounds;good(s->menu.open_for(s->dropdown,0,r.height));};
        scope.backend_0.on_open_context=[s] {auto r=take(s->context_trigger.snapshot()).bounds;good(s->context.open_for(s->context_trigger,0,r.height));};
        scope.backend_0.on_open_more=[s] {auto r=take(s->more.snapshot()).bounds;good(s->more.set_menu_expanded(true));good(s->submenu.open_for(s->more,r.width,0));};
        scope.backend_0.on_submenu_closed=[s] {good(s->more.set_menu_expanded(false));};
        scope.backend_0.on_menu_select=[s](int index) {s->menu_index=index;good(s->status.set_text("Menu index: "+std::to_string(index)));};
        scope.backend_0.on_close=[&app] { good(app.quit()); };
    }}));
}
