#include "Backend.h"
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void result(CodeEditorSession& s,core::Result<bool> r){if(!r)s.status.set("Error: "+r.error().message);}
}
void CodeEditorSession::initialize(runtime::Application& app,const std::filesystem::path& path){
    document=take(lvrs::TextDocumentModel::create(app.event_loop()));auto weak=weak_from_this();
    receiver=std::make_shared<lvrs::TextDocumentPort>([weak](const auto& c){if(auto s=weak.lock();s&&!s->retired)s->refresh(c.state);});
    good(document->observe(receiver));
    if(path.empty()){take(document->set_text("// LVUI → native C++ CodeEditor\n// 한글 · 😀 · e\xcc\x81\n#include <iostream>\n\nint main() {\n\tstd::cout << \"Hello from iiSDK\" << '\\n';\n}\n"));take(document->mark_clean());}
    else{auto name=path.filename().u8string();title.set(std::string(reinterpret_cast<const char*>(name.data()),name.size()));result(*this,document->load_file(path));}
    refresh(take(document->state()));
}
void CodeEditorSession::refresh(const lvrs::TextDocumentState& s){
    if(retired)return;status.set("Lines: "+std::to_string(s.line_count)+" · file-backed: "+std::to_string(s.file_backed_line_count)+
        " · modified lines: "+std::to_string(s.memory_line_count)+" · revision: "+std::to_string(s.document_revision)+(s.dirty?" · modified":" · clean")+
        (s.loading?" · reading "+std::to_string(s.loaded_byte_count)+"/"+std::to_string(s.total_byte_count):"")+(s.last_error.empty()?"":" · "+s.last_error));
}
void CodeEditorSession::start_background(runtime::Application& app){
    auto weak=weak_from_this();auto owner=app.event_loop().sender();auto scope=jobs.sender();
    auto job=jobs.post(app.workers(),[weak,owner,scope]()mutable{const auto result=6*7;(void)scope.post(owner,[weak,result]{if(auto s=weak.lock();s&&!s->retired){s->background_done=true;s->background.set("Background result: "+std::to_string(result)+" · iiSDKCore");}});});
    if(!job)throw std::runtime_error("Core background work admission failed");
}
void CodeEditorSession::retire(){if(retired)return;retired=true;jobs.request_close();(void)jobs.close();if(receiver)(void)receiver->close();receiver.reset();if(document)(void)document->close();document.reset();}
void CodeEditorBackend::undo(){result(*session,document->undo());}
void CodeEditorBackend::redo(){result(*session,document->redo());}
void CodeEditorBackend::toggle_lock(){locked.set(!locked.get());}
void CodeEditorBackend::save(){
    auto s=take(document->state());if(!s.has_file_path){status.set("Use --document PATH to enable explicit file publication.");return;}
    if(s.loading||s.loaded_byte_count!=s.total_byte_count||!s.last_error.empty()){status.set("Reload the complete source before saving.");return;}result(*session,document->save_file());
}
void CodeEditorBackend::changed(const lvrs::TextDocumentChange& change){session->refresh(change.state);}
void CodeEditorBackend::close(){if(on_close)on_close();}
