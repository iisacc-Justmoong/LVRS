#include "CodeEditorGallery.generated.h"
#include <iiGUI/Application.h>
#include <iiFileProviderNative.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::filesystem::path path(std::string_view text){return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()),text.size()));}
using View=lvui_generated::CodeEditorGallery;
struct State final{
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;std::shared_ptr<CodeEditorSession> session;
    std::shared_ptr<iiFileProvider::native::TemporaryDirectory> temporary;std::filesystem::path document;
    bool test=false,checked=false,resized=false,closed=false;std::uint64_t frames=0;
    gui::Widget widget(std::string_view id){return take(view->widgets().find(id));}
};
}
core::Result<void> runtime::configure_application(Application& app){
    auto p=std::make_shared<State>();const auto args=app.arguments();
    if(args.size()==3&&args[1]=="--self-test"){
        p->test=true;p->temporary=std::make_shared<iiFileProvider::native::TemporaryDirectory>(path(args[2]));p->document=p->temporary->path()/"source.cpp";
        std::string text;for(unsigned i=0;i<10000;++i)text+="// source "+std::to_string(i)+"\n";iiFileProvider::native::File::write(p->document,text);
    }else if(args.size()==3&&args[1]=="--document")p->document=path(args[2]);
    else if(args.size()!=1)return core::fail(core::ErrorCode::invalid_argument,"Use no arguments, --document PATH or --self-test BUILD_DIRECTORY");
    auto weak=std::weak_ptr<State>(p);auto options=View::options();options.frame_loop=&app.frame_loop();
    p->host=take(gui::attach_application(app,options,{
        .opened=[weak,&app]{auto p=weak.lock();if(!p)return;p->session->start_background(app);if(!p->test)return;
            take(app.frame_loop().add([weak](const auto& f){auto p=weak.lock();if(!p||p->closed)return;p->frames=f.frame;auto document=take(p->session->document->state());
                if(document.loading||!p->session->background_done)return;
                if(!p->checked){
                    check(document.line_count==10001&&document.file_backed_line_count==10001,"Gallery did not load file-offset lines");
                    auto editor=p->widget("editor");auto native=take(editor.snapshot());check(native.text.empty()&&native.text_input&&native.multiline,"Gallery substituted an ordinary input field");
                    good(editor.focus());const auto revision=document.document_revision;
                    good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::home,.control=true}));
                    check(take(p->session->document->state()).document_revision==revision,"Editor navigation modified storage");
                    good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::text,.text="// edited\n"}));
                    check(take(p->session->document->state()).memory_line_count<=2,"One editor insertion promoted unrelated file lines");
                    auto shared=p->widget("sharedEditor");good(shared.focus());good(p->view->widgets().dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::right,.shift=true}));
                    check(take(shared.snapshot()).text_selection.length==1&&take(editor.snapshot()).text_selection.length==0,"Shared views did not retain independent selections");
                    good(p->widget("undo").activate());
                    check(take(shared.snapshot()).text_selection.length==1&&take(shared.snapshot()).text_length==take(editor.snapshot()).text_length,"Shared undo failed to rebase the sibling selection");
                    good(p->widget("save").activate());
                    check(!take(p->session->document->state()).dirty&&take(p->session->document->stats()).materializations==0,"Editor scope materialized or failed to publish the document");
                    check(CodeEditorBackend::created==1,"Editor updates reconstructed the scoped backend");take(p->host->capture_canvas());p->checked=true;good(p->host->set_content_size({980,720}));return;
                }
                if(p->resized&&f.frame>=3)good(p->widget("close").activate());
            },{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
        },
        .resized=[weak](gui::Size size){if(auto p=weak.lock();p&&p->session){p->session->width.set(std::max(180.,(size.width-64)/2));p->session->height.set(std::max(144.,size.height-260));p->resized|=size==gui::Size{980,720};}},
        .closed=[weak]{if(auto p=weak.lock())p->closed=true;}
    }));
    if(auto r=app.attach({"editor-verification",[](Application&)->core::Result<void>{return {};},[p]()->core::Result<void>{
        check(CodeEditorBackend::created==CodeEditorBackend::retired,"Editor backend outlived its view scope");
        if(p->test){check(p->checked&&p->resized&&p->closed&&p->session->retired,"Editor gallery milestones incomplete");std::cout<<"LVRS_NATIVE_CODE_EDITOR_GALLERY_OK frames="<<p->frames<<'\n';}return {};
    }});!r)return r;
    return app.attach(gui::view_attachment("code-editor",p->host,p->view,std::function<void(View::Scope&)>{[weak,&app](auto& scope){if(auto p=weak.lock()){
        p->session=scope.backend_0.session;p->session->initialize(app,p->document);scope.backend_0.on_close=[&app]{good(app.quit());};
    }}}));
}
