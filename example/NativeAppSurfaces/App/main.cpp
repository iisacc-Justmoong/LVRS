#include "Surfaces.generated.h"
#include <iiGUI/Application.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
using View=lvui_generated::Surfaces;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;
    std::shared_ptr<SurfaceModel> model;
    bool test=false,closed=false,background=false,resized=false,updated=false;
    std::uint64_t frame=0;
    gui::Widget widget(std::string_view id){return take(view->widgets().find(id));}
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto p=std::make_shared<State>();p->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1&&!p->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    std::weak_ptr<State> weak=p;auto sender=app.event_loop().sender();auto options=View::options();options.frame_loop=&app.frame_loop();
    p->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{auto p=weak.lock();if(!p)return;
            check(bool(app.post([weak,sender]{(void)sender.post([weak]{if(auto p=weak.lock();p&&!p->closed){p->background=true;p->model->status.set("Background result: 42 · iiSDKCore");}});})),"Worker result admission failed");
            take(app.frame_loop().add([p](const auto& frame){p->frame=frame.frame;if(p->test&&p->updated&&p->background&&p->resized&&frame.frame>=3)good(p->widget("close").activate());},{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(!p->test)return;
            auto editor=p->widget("editor"),model_button=p->widget("modelTools/model");
            auto retained_tool=take(model_button.content_item("button_0")),retained_footer=p->widget("footerTools/button_0");
            good(editor.set_text("Retained editor"));good(p->widget("header/menu").activate());good(p->widget("update").activate());good(p->widget("shape").activate());
            check(p->model->compact.get()&&p->model->shape.get()==lvrs::AppCardShape::cylinder&&take(editor.snapshot()).text=="Retained editor"&&SurfaceBackend::created==1&&SurfaceBackend::retired==0,"Surface update reconstructed its scope or lost reactive state");
            check(take(p->widget("body").snapshot()).parent_id=="card/content"&&take(p->widget("update").snapshot()).parent_id=="header/actions"&&take(p->widget("header/menu").snapshot()).bounds.height==28,"Native default slots or compact menu geometry diverged");
            check(take(retained_tool.snapshot()).icon_name==lvrs::Icons::normalize("settings")&&take(retained_footer.snapshot()).icon_name==lvrs::Icons::normalize("settings"),"Immutable toolbar input rebuilt or failed to update native sources");
            check(take(p->widget("manualStructure").snapshot()).parent_id=="manualTools/manual","LVUI manual toolbar registration escaped its native slot");
            good(p->widget("listTools/button_0").activate());good(retained_footer.activate());good(retained_tool.activate());good(p->widget("manualStructure").activate());check(p->model->toolbar_events==4,"Toolbar callbacks did not share the existing backend scope");
            p->updated=true;take(p->host->capture_canvas());good(p->host->set_content_size({960,700}));
        },
        .resized=[weak](gui::Size size){if(auto p=weak.lock();p&&p->model){p->model->width.set(std::max(80.,size.width-48));p->model->height.set(std::max(100.,size.height-400));p->resized|=size==gui::Size{960,700};}},
        .closed=[weak]{if(auto p=weak.lock())p->closed=true;}
    }));
    if(auto result=app.attach({"surface-verification",[](Application&)->core::Result<void>{return {};},[p]()->core::Result<void>{
        check(SurfaceBackend::created==SurfaceBackend::retired,"Surface backend outlived native view shutdown");
        if(p->test){check(p->closed&&p->background&&p->resized&&p->updated&&p->frame>=3,"Surface gallery milestones incomplete");std::cout<<"LVRS_NATIVE_APP_SURFACES_OK scope=1/1 background=42 frames="<<p->frame<<'\n';}return {};
    }});!result)return result;
    return app.attach(gui::view_attachment("surfaces",p->host,p->view,std::function<void(View::Scope&)>{[weak,&app](auto& scope){if(auto p=weak.lock()){p->model=scope.backend_0.model;scope.backend_0.on_close=[&app]{good(app.quit());};}}}));
}
