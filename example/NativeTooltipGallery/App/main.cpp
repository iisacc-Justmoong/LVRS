#include "TooltipGallery.generated.h"
#include <iiGUI/Application.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
using View=lvui_generated::TooltipGallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;std::shared_ptr<TooltipSession> session;
    bool test=false,started=false,checked=false,resized=false,closed=false;runtime::Clock::time_point start;
    gui::Widget widget(std::string_view id){return take(view->widgets().find(id));}
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto p=std::make_shared<State>();const auto args=app.arguments();p->test=args.size()==2&&args[1]=="--self-test";
    if(args.size()!=1&&!p->test)return core::fail(core::ErrorCode::invalid_argument,"Use no arguments or --self-test");
    auto weak=std::weak_ptr<State>(p);auto options=View::options();options.frame_loop=&app.frame_loop();
    p->host=take(gui::attach_application(app,options,{
        .opened=[weak,&app]{auto p=weak.lock();if(!p)return;p->session->start(app);if(!p->test)return;
            take(app.frame_loop().add([weak](const auto&){auto p=weak.lock();if(!p||p->closed)return;
                if(!p->started){const auto r=take(p->widget("origin").snapshot()).bounds;good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=r.x+r.width/2,.y=r.y+r.height/2}));p->started=true;p->start=runtime::Clock::now();return;}
                if(runtime::Clock::now()-p->start>std::chrono::seconds(3))throw std::runtime_error("Tooltip gallery deadline did not settle");
                if(!p->checked){
                    auto bubble=take(p->widget("helpBubble").snapshot());if(!bubble.opened||!p->session->background_done)return;
                    check(!bubble.modal&&!bubble.dim&&!take(p->widget("origin").snapshot()).focused,"Hover Tooltip changed focus or modal state");
                    p->session->help.set("Updated help in the same retained Tooltip");check(take(p->widget("bubbleTitle").snapshot()).text==p->session->help.get(),"Tooltip child binding did not reach its retained label");
                    good(p->widget("bubbleAction").activate());check(p->session->detail.get().find("same retained")!=std::string::npos,"Tooltip child action did not reach its scoped backend");
                    take(p->host->capture_canvas());const auto origin=take(p->widget("origin").snapshot()).bounds;
                    good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=origin.x+origin.width/2,.y=origin.y+origin.height/2}));
                    good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=origin.x+origin.width/2,.y=origin.y+origin.height/2}));
                    check(p->session->clicks==1&&p->session->opens==1&&p->session->closes==1,"Origin click was consumed or Tooltip dismissal reopened it");
                    check(TooltipBackend::created==1,"Tooltip bindings recreated the backend");p->checked=true;good(p->host->set_content_size({920,640}));return;
                }
                if(p->resized){check(!take(p->widget("helpBubble").snapshot()).opened,"Available-area binding reset Tooltip dismissal");good(p->widget("close").activate());}
            },{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
        },
        .resized=[weak](gui::Size size){if(auto p=weak.lock();p&&p->session){p->session->width.set(std::max(200.,size.width-48));p->session->area.set({12,40,float(std::max(0.,size.width-24)),float(std::max(0.,size.height-52))});p->resized|=size==gui::Size{920,640};}},
        .closed=[weak]{if(auto p=weak.lock())p->closed=true;}
    }));
    if(auto r=app.attach({"tooltip-verification",[](Application&)->core::Result<void>{return {};},[p]()->core::Result<void>{
        check(TooltipBackend::created==TooltipBackend::retired,"Tooltip backend outlived its view scope");if(p->test){check(p->checked&&p->resized&&p->closed&&p->session->retired,"Tooltip gallery milestones incomplete");std::cout<<"LVRS_NATIVE_TOOLTIP_GALLERY_OK\n";}return {};
    }});!r)return r;
    return app.attach(gui::view_attachment("tooltip",p->host,p->view,std::function<void(View::Scope&)>{[weak,&app](auto& scope){if(auto p=weak.lock()){p->session=scope.backend_0.session;scope.backend_0.on_close=[&app]{good(app.quit());};}}}));
}
