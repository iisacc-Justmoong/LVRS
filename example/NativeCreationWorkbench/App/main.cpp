#include "Workbench.generated.h"
#include <iiGUI/Application.h>
#include <algorithm>
#include <iostream>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> v){if(!v){if(v.error().cause)std::rethrow_exception(v.error().cause);throw std::runtime_error(v.error().message);}return std::move(*v);}
void good(core::Result<void> v){if(!v){if(v.error().cause)std::rethrow_exception(v.error().cause);throw std::runtime_error(v.error().message);}}
void check(bool b,const char* text){if(!b)throw std::runtime_error(text);}
using View=lvui_generated::Workbench;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();
    std::shared_ptr<gui::Host> host;
    std::shared_ptr<CreationModel> model;
    bool test=false,closed=false,resized=false,verified=false;
    gui::Widget widget(std::string_view id){return take(view->widgets().find(id));}
    void dispatch(gui::WidgetInput e){good(view->widgets().dispatch_input(e));}
};
void verify(State& p){
    auto m=p.model;check(m->frames.size()==24&&m->results.get().size()==4,"Background native renderer did not publish real results");
    m->seek(0);auto a=take(p.host->capture_canvas());m->seek(12);auto b=take(p.host->capture_canvas());
    auto bounds=take(m->canvas_widget.snapshot()).bounds;auto scale=take(p.host->snapshot()).backing_scale;
    auto x=std::uint32_t((bounds.x+bounds.width/2)*scale),y=std::uint32_t((bounds.y+bounds.height/2)*scale);
    check(x<a.width&&y<a.height&&a.width==b.width&&a.height==b.height,"Native canvas capture has invalid geometry");
    auto offset=(std::size_t(y)*a.width+x)*4;check(a.rgba[offset+3]>200&&b.rgba[offset+3]>200&&a.rgba[offset]!=b.rgba[offset],"Native frame pixels were cleared or did not change");
    auto timeline=take(m->timeline_widget.snapshot()).bounds;
    p.dispatch({.kind=gui::WidgetInputKind::pointer_down,.x=timeline.x+timeline.width*.3,.y=timeline.y+24});
    p.dispatch({.kind=gui::WidgetInputKind::pointer_move,.x=timeline.x+timeline.width*.75,.y=timeline.y+24,.device=gui::PointerDevice::pen,.pressure=.6});
    p.dispatch({.kind=gui::WidgetInputKind::pointer_up,.x=timeline.x+timeline.width*.75,.y=timeline.y+24});
    check(m->frame>=16&&m->frame<=18&&m->file_revision==0,"Native timeline input lost seeking or mutated the document");
    p.dispatch({.kind=gui::WidgetInputKind::pointer_down,.x=bounds.x+40,.y=bounds.y+40});
    p.dispatch({.kind=gui::WidgetInputKind::pointer_move,.x=bounds.x+48,.y=bounds.y+45});
    p.dispatch({.kind=gui::WidgetInputKind::pointer_up,.x=bounds.x+48,.y=bounds.y+45});
    check(m->pan_x==8&&m->pan_y==5&&m->file_revision==0,"Native canvas drag did not remain view-only");
    bool selected=false;for(const auto& item:take(p.view->widgets().snapshot()))if(item.id.ends_with("/choose")){good(take(p.view->widgets().find(item.id)).activate());selected=true;break;}
    check(selected&&m->selected.get()!="None","Generated result row did not call its C++ selection method");
    check(m->view_builds==2&&take(p.widget("prompt").snapshot()).text=="Preserved native draft","Result delivery recreated a surface or replaced the prompt");
    good(p.host->set_content_size({1100,860}));good(p.widget("play").activate());p.verified=true;
}
}
core::Result<void> runtime::configure_application(Application& app){
    auto p=std::make_shared<State>();p->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1&&!p->test)return core::fail(core::ErrorCode::invalid_argument,"Use no arguments or --self-test");
    std::weak_ptr<State> weak=p;auto sender=app.event_loop().sender();auto options=View::options();options.frame_loop=&app.frame_loop();
    p->host=take(gui::attach_application(app,options,{
        .opened=[weak,&app]{auto p=weak.lock();if(!p)return;if(!p->test)return;
            good(p->widget("prompt").set_text("Preserved native draft"));good(p->widget("prompt").focus());good(p->widget("render").activate());
            take(app.frame_loop().add([weak](const auto&){auto p=weak.lock();if(!p||p->closed)return;if(!p->verified&&!p->model->frames.empty())verify(*p);if(p->verified&&p->resized&&p->model->play_ticks>=2){p->model->playing=false;p->model->touch();good(p->widget("close").activate());}},{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
        },
        .resized=[weak](gui::Size size){if(auto p=weak.lock();p&&p->model){p->model->view_width.set(std::max(320.,size.width-300));p->resized|=size==gui::Size{1100,860};}},
        .closed=[weak]{if(auto p=weak.lock())p->closed=true;}
    }));
    if(auto r=app.attach({"workbench-check",[](Application&)->core::Result<void>{return {};},[p]()->core::Result<void>{
        check(CreationBackend::created==CreationBackend::retired,"Native workbench backend did not retire");
        if(p->test){check(p->closed&&p->verified&&p->resized&&p->model->play_ticks>=2&&p->model->view_builds==p->model->view_retires&&p->model->file_revision==0,"Native workbench milestones incomplete");std::cout<<"LVRS_NATIVE_CREATION_WORKBENCH_OK frames=24 results=4 sources=2 backend=1/1 document_revisions=0\n";}return {};
    }});!r)return r;
    return app.attach(gui::view_attachment("workbench",p->host,p->view,std::function<void(View::Scope&)>{[weak,sender,&app](auto& scope){if(auto p=weak.lock()){
        p->model=scope.backend_0.model;
        p->model->view_width.set(std::max(320.,take(p->host->snapshot()).size.width-300));
        scope.backend_0.on_close=[&app]{good(app.quit());};
        scope.backend_0.on_render=[weak,sender,&app](CreationSettings settings){auto accepted=app.post([weak,sender,settings]{
            try{auto output=render_creation_probe(settings);(void)sender.post([weak,output=std::move(output),settings]()mutable{if(auto p=weak.lock();p&&!p->closed)p->model->accept(std::move(output),settings.count);});}
            catch(const std::exception& e){(void)sender.post([weak,text=std::string(e.what())]{if(auto p=weak.lock();p&&!p->closed)p->model->status.set("Failed: "+text);});}
        });if(!accepted)if(auto p=weak.lock())p->model->status.set("Native worker queue rejected the probe");};
    }}}));
}
