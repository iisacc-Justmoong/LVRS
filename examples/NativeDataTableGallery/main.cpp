#include "Gallery.generated.h"
#include <iiGUI/Application.h>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> result){if(!result)throw std::runtime_error(result.error().message);return std::move(*result);}
void good(core::Result<void> result){if(!result)throw std::runtime_error(result.error().message);}
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
using View=lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;std::shared_ptr<lvrs::TableModel> model;
    bool test=false,closed=false,background=false,resized=false;int edits=0,submits=0,rejects=0;std::uint64_t frame=0;
    gui::Widget get(std::string_view id){return take(view->widgets().find(id));}
    void status(std::string text){good(get("status").set_text(text));}
    void click_cell(){auto r=take(get("data.body").snapshot()).bounds;good(view->widgets().dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+330,.y=r.y+12}));good(view->widgets().dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+330,.y=r.y+12}));}
    void replace(std::string text){good(view->widgets().dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(view->widgets().dispatch_input({.kind=gui::WidgetInputKind::text,.text=std::move(text)}));}
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto state=std::make_shared<State>();state->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";
    if(app.arguments().size()>1&&!state->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    std::weak_ptr<State> weak=state;auto sender=app.event_loop().sender();auto options=View::options();options.frame_loop=&app.frame_loop();
    state->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{auto p=weak.lock();if(!p)return;
            check(bool(app.post([weak,sender]{(void)sender.post([weak]{if(auto p=weak.lock();p&&!p->closed){p->background=true;good(p->get("worker").set_text("Background result: 42 · iiSDKCore"));}});})),"Worker admission failed");
            take(app.frame_loop().add([p](const auto& frame){p->frame=frame.frame;if(frame.frame%15==0)good(p->get("frames").set_text("Core display frames: "+std::to_string(frame.frame)));if(p->test&&p->background&&p->resized&&frame.frame>=3)good(p->get("close").activate());},{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(!p->test)return;
            p->click_cell();p->replace("42");good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));
            p->click_cell();p->replace("bad");check(take(p->get("data.editor").snapshot()).text=="42","Rejected AOT edit replaced accepted text");good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::escape}));
            const auto body_x=take(p->get("data.body").snapshot()).bounds.x;
            good(p->get("data.viewport").set_horizontal_scroll_offset(500));check(take(p->get("data.viewport").snapshot()).horizontal_scroll_offset==500&&take(p->get("data.body").snapshot()).bounds.x==body_x,"Horizontal AOT viewport translated physical content");
            good(p->get("data.viewport").set_scroll_offset(1200000));check(take(p->get("data.viewport").snapshot()).scroll_offset==1200000,"Deep AOT viewport did not retain logical offset");
            take(p->model->set_cell_value({50000,1},std::int64_t(77)));check(p->edits==1&&p->submits==1&&p->rejects==1,"AOT table callbacks incomplete");
            check(take(p->view->widgets().snapshot()).size()<30,"AOT table allocated widgets per data row");take(p->host->capture_canvas());good(p->host->set_content_size({800,660}));
        },.resized=[weak](gui::Size size){if(auto p=weak.lock())p->resized|=size==gui::Size{800,660};},.closed=[weak]{if(auto p=weak.lock())p->closed=true;}
    }));
    auto attached=app.attach({"verification",[](Application&)->core::Result<void>{return {};},[state]()->core::Result<void>{
        check(DataTableBackend::created==1&&DataTableBackend::destroyed==1,"Scoped table backend lifetime mismatch");
        if(state->test){check(state->closed&&state->background&&state->resized&&state->frame>=3,"Data table gallery milestones incomplete");std::cout<<"LVRS_NATIVE_DATA_TABLE_OK rows=100000 edit=1 submit=1 reject=1 deep_offset=1200000 horizontal_offset=500 backend=1/1 background=42 frames="<<state->frame<<'\n';}return {};
    }});if(!attached)return attached;
    return app.attach(gui::view_attachment("data-table",state->host,state->view,std::function<void(View::Scope&)>{[weak,&app](View::Scope& scope){auto& backend=scope.backend_0;if(auto p=weak.lock())p->model=backend.model;
        backend.on_edited=[weak](std::size_t row,std::size_t column,std::string value){if(auto p=weak.lock()){++p->edits;p->status("Edited "+std::to_string(row)+":"+std::to_string(column)+" = "+value);}};
        backend.on_submitted=[weak](std::size_t,std::size_t,std::string value){if(auto p=weak.lock()){++p->submits;p->status("Submitted "+value);}};
        backend.on_rejected=[weak](std::size_t,std::size_t,std::string value,std::string type){if(auto p=weak.lock()){++p->rejects;p->status("Rejected "+type+": "+value);}};
        backend.on_close=[&app]{good(app.quit());};
        backend.on_operation=[weak](std::string message){if(auto p=weak.lock())p->status(std::move(message));};
    }}));
}
