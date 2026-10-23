#include "NavigationGallery.generated.h"
#include <iiGUI/Application.h>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
using View=lvui_generated::NavigationGallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();
    std::shared_ptr<gui::Host> host;
    std::shared_ptr<lvrs::NavigationStackModel> model;
    std::shared_ptr<lvrs::NavigationChangePort> port;
    bool test=false,closed=false,background=false,resized=false,navigated=false;
    unsigned changes=0;std::uint64_t frame=0;
    gui::Widget widget(std::string_view id){return take(view->widgets().find(id));}
    void render(){
        if(closed)return;auto current=take(model->state());
        good(widget("route").set_text("Route: "+current.current_path));good(widget("history").set_text("Depth: "+std::to_string(current.depth)));
        std::string values="Parameters:";
        if(auto params=current.current_params.get<core::Value::Object>();params)for(const auto& [key,value]:params->get())if(auto text=value.get<std::string>();text)values+=" "+key+"="+text->get();
        if(values=="Parameters:")values+=" none";good(widget("params").set_text(std::move(values)));
    }
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto state=std::make_shared<State>();state->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1&&!state->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    std::weak_ptr<State> weak=state;auto sender=app.event_loop().sender();auto options=View::options();options.frame_loop=&app.frame_loop();
    state->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{auto p=weak.lock();if(!p)return;p->render();
            check(bool(app.post([weak,sender]{(void)sender.post([weak]{if(auto p=weak.lock();p&&!p->closed){p->background=true;good(p->widget("worker").set_text("Background result: 42 · iiSDKCore"));}});})),"Worker admission failed");
            take(app.frame_loop().add([p](const auto& frame){p->frame=frame.frame;if(p->test&&p->navigated&&p->background&&p->resized&&frame.frame>=3)good(p->widget("close").activate());},{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(!p->test)return;
            good(p->widget("report").activate());check(take(p->model->state()).depth==2&&take(p->model->state()).current_path=="/reports/42","AOT push did not update native history");
            good(p->widget("replace").activate());check(take(p->model->state()).depth==2&&take(p->model->state()).current_path=="/reports/43","AOT replace pushed another page");
            good(p->widget("logs").activate());check(take(p->model->state()).depth==3,"AOT rest route did not push");
            good(p->widget("back").activate());check(take(p->model->state()).current_path=="/reports/43","AOT back lost prior route");
            good(p->widget("root").activate());check(take(p->model->state()).depth==1&&p->changes==5,"AOT root or coalesced route notifications failed");
            p->navigated=true;take(p->host->capture_canvas());good(p->host->set_content_size({740,560}));
        },.resized=[weak](gui::Size size){if(auto p=weak.lock())p->resized|=size==gui::Size{740,560};},.closed=[weak]{if(auto p=weak.lock())p->closed=true;}
    }));
    auto verified=app.attach({"navigation-verification",[](Application&)->core::Result<void>{return {};},[state]()->core::Result<void>{
        check(NavigationGalleryBackend::created==1&&NavigationGalleryBackend::destroyed==1,"Navigation backend scope lifetime mismatch");
        if(state->test){check(state->closed&&state->background&&state->resized&&state->navigated&&state->frame>=3,"Navigation gallery milestones incomplete");std::cout<<"LVRS_NATIVE_NAVIGATION_GALLERY_OK changes=5 backend=1/1 background=42 frames="<<state->frame<<'\n';}return {};
    }});if(!verified)return verified;
    return app.attach(gui::view_attachment("navigation",state->host,state->view,std::function<void(View::Scope&)>{[weak,&app](View::Scope& scope){auto p=weak.lock();if(!p)return;
        p->model=scope.backend_0.model;p->port=std::make_shared<lvrs::NavigationChangePort>([weak](const auto& change){if(auto p=weak.lock();p&&!p->closed){if(change.path)++p->changes;p->render();}});good(p->model->observe(p->port));
        scope.backend_0.on_close=[&app]{good(app.quit());};
    }}));
}
