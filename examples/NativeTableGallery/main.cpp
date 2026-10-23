#include "Gallery.generated.h"
#include <iiGUI/Application.h>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
using View=lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;
    bool test=false,closed=false,background=false,resized=false;int edits=0,submits=0,rejects=0,row_edits=0,row_submits=0,columns=0;std::uint64_t frame=0;
    gui::Widget get(std::string_view id){return take(view->widgets().find(id));}
    void status(std::string text){good(get("status").set_text(text));}
    void replace(std::string_view id,std::string text){good(get(id).focus());good(view->widgets().dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(view->widgets().dispatch_input({.kind=gui::WidgetInputKind::text,.text=std::move(text)}));}
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto s=std::make_shared<State>();s->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";if(app.arguments().size()>1&&!s->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    std::weak_ptr<State> weak=s;auto sender=app.event_loop().sender();auto options=View::options();options.frame_loop=&app.frame_loop();
    s->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{auto p=weak.lock();if(!p)return;
            check(bool(app.post([weak,sender]{(void)sender.post([weak]{if(auto p=weak.lock();p&&!p->closed){p->background=true;good(p->get("worker").set_text("Background result: 42 · iiSDKCore"));}});})),"Worker submission failed");
            take(app.frame_loop().add([p,sender](const auto& frame){p->frame=frame.frame;if(frame.frame%15==0)good(p->get("frames").set_text("Core display frames: "+std::to_string(frame.frame)));if(p->test&&frame.frame==1)check(bool(sender.after(std::chrono::seconds(5),[]{throw std::runtime_error("Table gallery watchdog");})),"Watchdog admission failed");if(p->test&&p->background&&p->resized&&frame.frame>=3)good(p->get("close").activate());},{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(!p->test)return;
            p->replace("count.editor","45");good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));p->replace("count.editor","invalid");check(take(p->get("count.editor").snapshot()).text=="45","Rejected AOT edit changed value");
            p->replace("row0.cell1.editor","Edited native row");good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));good(p->get("header.column1").activate());
            check(p->edits==1&&p->submits==1&&p->rejects==1&&p->row_edits==1&&p->row_submits==1&&p->columns==1,"Typed AOT table callbacks incomplete");take(p->host->capture_canvas());good(p->host->set_content_size({860,560}));
        },.resized=[weak](gui::Size size){if(auto p=weak.lock())p->resized|=size==gui::Size{860,560};},.closed=[weak]{if(auto p=weak.lock()){p->closed=true;check(TableBackend::destroyed==0,"Backend retired before Core detach");}}
    }));
    auto r=app.attach({"verification",[](Application&)->core::Result<void>{return {};},[s]()->core::Result<void>{check(TableBackend::created==1&&TableBackend::destroyed==1,"Backend lifetime mismatch");if(s->test){check(s->closed&&s->background&&s->resized&&s->frame>=3,"Table gallery milestones incomplete");std::cout<<"LVRS_NATIVE_TABLE_GALLERY_OK edits=1 submitted=1 rejected=1 row_edit=1 row_submit=1 column=1 backend=1/1 background=42 frames="<<s->frame<<'\n';}return {};}});if(!r)return r;
    return app.attach(gui::view_attachment("table",s->host,s->view,std::function<void(View::Scope&)>{[weak,&app](View::Scope& scope){auto& b=scope.backend_0;
        b.on_edited=[weak](std::string value){if(auto p=weak.lock()){++p->edits;p->status("Integer edited: "+value);}};
        b.on_submitted=[weak](std::string value){if(auto p=weak.lock()){++p->submits;p->status("Integer submitted: "+value);}};
        b.on_rejected=[weak](std::string value,std::string type){if(auto p=weak.lock()){++p->rejects;p->status("Rejected "+type+": "+value);}};
        b.on_row_edited=[weak](std::size_t i,std::string value){if(auto p=weak.lock()){++p->row_edits;p->status("Row column "+std::to_string(i)+": "+value);}};
        b.on_row_submitted=[weak](std::size_t i,std::string value){if(auto p=weak.lock()){++p->row_submits;p->status("Row submitted "+std::to_string(i)+": "+value);}};
        b.on_column=[weak](std::size_t i,const lvrs::TableHeaderColumn& c){if(auto p=weak.lock()){++p->columns;p->status("Header activated "+std::to_string(i)+": "+c.text);}};b.on_close=[&app]{good(app.quit());};
    }}));
}
