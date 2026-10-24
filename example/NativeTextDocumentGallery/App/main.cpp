#include "TextDocuments.generated.h"
#include <iiGUI/Application.h>
#include <iiFileProviderNative.h>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
std::filesystem::path path(std::string_view text){return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()),text.size()));}
using View=lvui_generated::TextDocuments;
struct State final{
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;
    std::shared_ptr<TextDocumentSession> model;std::shared_ptr<iiFileProvider::native::TemporaryDirectory> temporary;
    std::filesystem::path document;bool test=false,checked=false,resized=false,closed=false;std::uint64_t frames=0;
    gui::Widget widget(std::string_view name){return take(view->widgets().find(name));}
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto p=std::make_shared<State>();const auto arguments=app.arguments();
    if(arguments.size()==3&&arguments[1]=="--self-test"){
        p->test=true;p->temporary=std::make_shared<iiFileProvider::native::TemporaryDirectory>(path(arguments[2]));p->document=p->temporary->path()/"document.txt";
        std::string bytes;for(unsigned i=0;i<4096;++i){bytes+=std::to_string(i);bytes+='\n';}iiFileProvider::native::File::write(p->document,bytes);
    }else if(arguments.size()==3&&arguments[1]=="--document")p->document=path(arguments[2]);
    else if(arguments.size()!=1)return core::fail(core::ErrorCode::invalid_argument,"Use --document PATH, --self-test BUILD_DIRECTORY or no arguments");
    std::weak_ptr<State> weak=p;auto options=View::options();options.frame_loop=&app.frame_loop();
    p->host=take(gui::attach_application(app,options,{
        .opened=[weak,&app]{auto p=weak.lock();if(!p)return;p->model->start_background(app);if(!p->test)return;
            take(app.frame_loop().add([weak](const auto& frame){auto p=weak.lock();if(!p||p->closed)return;p->frames=frame.frame;
                const auto state=take(p->model->document->state());if(state.loading||!p->model->background_done)return;
                if(!p->checked){check(state.line_count==4097&&state.file_backed_line_count==4097,"Gallery did not use the file-backed model");const auto revision=state.document_revision;
                    good(p->widget("next").activate());check(take(p->model->document->state()).document_revision==revision,"Viewport navigation edited the document");
                    good(p->widget("insert").activate());check(take(p->model->document->state()).memory_line_count==1,"Gallery promoted unrelated source lines");
                    good(p->widget("undo").activate());good(p->widget("save").activate());check(!take(p->model->document->state()).dirty,"Gallery failed to publish its document");
                    check(take(p->model->document->stats()).materializations==0,"Reactive gallery materialized the complete document");
                    check(TextDocumentBackend::created==1,"Document updates reconstructed the backend");take(p->host->capture_canvas());p->checked=true;good(p->host->set_content_size({960,660}));return;
                }
                if(p->resized&&frame.frame>=3)good(p->widget("close").activate());
            },{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
        },
        .resized=[weak](gui::Size size){if(auto p=weak.lock())p->resized|=size==gui::Size{960,660};},
        .closed=[weak]{if(auto p=weak.lock())p->closed=true;}
    }));
    if(auto r=app.attach({"document-verification",[](Application&)->core::Result<void>{return {};},[p]()->core::Result<void>{
        check(TextDocumentBackend::created==TextDocumentBackend::retired,"Document backend outlived its view");
        if(p->test){check(p->checked&&p->resized&&p->closed&&p->model->retired&&p->model->background_done,"Document gallery milestones incomplete");std::cout<<"LVRS_NATIVE_TEXT_DOCUMENT_GALLERY_OK frames="<<p->frames<<'\n';}return {};
    }});!r)return r;
    return app.attach(gui::view_attachment("text-documents",p->host,p->view,std::function<void(View::Scope&)>{[weak,&app](auto& scope){if(auto p=weak.lock()){
        p->model=scope.backend_0.model;p->model->initialize(app,p->document);scope.backend_0.on_close=[&app]{good(app.quit());};
    }}}));
}
