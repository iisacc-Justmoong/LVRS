#include "RouterGallery.generated.h"
#include "PageBackend.h"
#include <LVRS/PageMonitor.h>
#include <iiGUI/Application.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
using View=lvui_generated::RouterGallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;
    std::shared_ptr<lvrs::PageRouter> router;std::shared_ptr<lvrs::PageNavigationPort> port;
    std::shared_ptr<lvrs::Navigator> navigator;
    std::shared_ptr<lvrs::PageMonitor> visits=std::make_shared<lvrs::PageMonitor>();
    std::shared_ptr<lvrs::ViewStateTracker> tracker;
    std::shared_ptr<lvrs::PageMonitorPort> visits_port;
    std::shared_ptr<lvrs::ViewStatePort> tracker_port;
    bool test=false,closed=false,background=false,resized=false,navigated=false;std::uint64_t frame=0;unsigned changes=0;
    gui::Widget widget(std::string_view id){return take(view->widgets().find(id));}
    void update_tracking_labels(){
        const auto log=take(visits->state());if(auto label=view->widgets().find("visits");label)good(label->set_text("Visits: "+std::to_string(log.count)+" · current "+log.current));
        if(tracker){const auto views=take(tracker->state());if(auto label=view->widgets().find("tracking");label)good(label->set_text("Logical views: "+std::to_string(views.loaded_count)+" · active "+views.current_active_view));}
    }
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto state=std::make_shared<State>();state->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1&&!state->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    std::weak_ptr<State> weak=state;auto sender=app.event_loop().sender();auto options=View::options();options.frame_loop=&app.frame_loop();
    state->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{auto p=weak.lock();if(!p)return;
            p->update_tracking_labels();
            check(bool(app.post([weak,sender]{(void)sender.post([weak]{if(auto p=weak.lock();p&&!p->closed){p->background=true;good(p->widget("worker").set_text("Background result: 42 · iiSDKCore"));}});})),"Worker admission failed");
            take(app.frame_loop().add([p](const auto& frame){p->frame=frame.frame;if(p->test&&p->navigated&&p->resized&&p->background&&frame.frame>=3)good(p->widget("close").activate());},{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(!p->test)return;
            good(p->widget("alternate").activate());auto initial=take(p->router->current_page_item("next"));good(initial.activate());check(!initial.snapshot()&&take(p->router->state()).navigation.current_path=="/reports/43"&&take(p->navigator->state()).current.navigation.current_path=="/reports/43"&&PageBackend::created==2&&PageBackend::retired==1,"Live Link binding did not dispatch and retire the old compiled page scope");
            auto report=take(p->router->current_page_item("previous"));good(report.activate());check(!report.snapshot()&&take(p->router->state()).navigation.current_path=="/"&&PageBackend::created==3&&PageBackend::retired==2,"Back reused a retired page scope");
            take(p->router->replace("/reports/43"));take(p->router->set_root("/"));check(p->changes==5&&PageBackend::created==5&&PageBackend::retired==4,"Page and history publication diverged");
            check(take(p->visits->state()).count==5&&take(p->tracker->state()).loaded_count==1&&take(p->tracker->state()).current_active_view=="/","Visit log or logical view tracking diverged from committed pages");
            good(p->widget("undoVisit").activate());check(take(p->visits->state()).current=="/reports/43"&&take(p->router->state()).navigation.current_path=="/","Visit undo unexpectedly navigated the router");take(p->visits->record("/"));
            p->navigated=true;take(p->host->capture_canvas());good(p->host->set_content_size({900,640}));
        },.resized=[weak](gui::Size size){if(auto p=weak.lock();p&&p->router){auto slot=p->view->widgets().find("pages");if(slot)good(slot->set_extent({std::max(1.,size.width-48),std::max(1.,size.height-270)}));p->resized|=size==gui::Size{900,640};}},.closed=[weak]{if(auto p=weak.lock())p->closed=true;}
    }));
    if(auto result=app.attach({"page-verification",[](Application&)->core::Result<void>{return {};},[state]()->core::Result<void>{
        check(PageBackend::created==PageBackend::retired,"A page backend outlived host shutdown");
        check(!take(state->navigator->state()).has_router,"Closed view retained a global registration");
        check(take(state->tracker->state()).loaded_count==0,"Shutdown retained logical tracking entries");
        if(state->test){check(state->closed&&state->background&&state->resized&&state->navigated&&state->frame>=3&&take(state->visits->state()).count==5,"Page gallery milestones incomplete");std::cout<<"LVRS_NATIVE_PAGE_ROUTER_GALLERY_OK pages=5/5 changes=5 background=42 frames="<<state->frame<<'\n';}return {};
    }});!result)return result;
    return app.attach(gui::view_attachment("pages",state->host,state->view,std::function<void(View::Scope&)>{[weak,&app](auto& scope){if(auto p=weak.lock()){
        p->router=scope.backend_0.router;p->navigator=scope.backend_0.navigator;p->tracker=take(p->navigator->view_state_tracker());
        p->visits_port=std::make_shared<lvrs::PageMonitorPort>([weak](const auto&){if(auto p=weak.lock();p&&!p->closed)p->update_tracking_labels();});good(p->visits->observe(p->visits_port));
        p->tracker_port=std::make_shared<lvrs::ViewStatePort>([weak](const auto&){if(auto p=weak.lock();p&&!p->closed)p->update_tracking_labels();});good(p->tracker->observe(p->tracker_port));
        p->port=std::make_shared<lvrs::PageNavigationPort>([weak](const auto& change){if(auto p=weak.lock();p&&!p->closed){++p->changes;take(p->visits->record(change.navigation.current_path));auto label=p->view->widgets().find("route");if(label)good(label->set_text("Route: "+change.navigation.current_path+" · depth "+std::to_string(change.navigation.depth)));}});good(p->router->observe(p->port));
        scope.backend_0.on_close=[&app]{good(app.quit());};scope.backend_0.on_undo_visit=[weak]{if(auto p=weak.lock())take(p->visits->undo());};
    }}}));
}
