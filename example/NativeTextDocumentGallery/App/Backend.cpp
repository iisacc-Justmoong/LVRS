#include "Backend.h"
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void result(const std::shared_ptr<TextDocumentSession>& model,core::Result<bool> r){if(!r)model->status.set("Error: "+r.error().message);}
}
void TextDocumentSession::initialize(runtime::Application& app,const std::filesystem::path& path){
    document=take(lvrs::TextDocumentModel::create(app.event_loop()));std::weak_ptr<TextDocumentSession> weak=shared_from_this();
    receiver=std::make_shared<lvrs::TextDocumentPort>([weak](const auto& change){if(auto p=weak.lock();p&&!p->retired)p->refresh(change.state);});
    good(document->observe(receiver));
    if(path.empty()){take(document->set_text("Native document model\n한글과 😀, e\xcc\x81\nEach viewport query reads one retained line.\nUse --document PATH for file-backed storage."));take(document->mark_clean());}
    else take(document->load_file(path));
    refresh(take(document->state()));
}
void TextDocumentSession::refresh(const lvrs::TextDocumentState& state){
    if(retired||!document)return;
    auto row=take(document->line_text(state.cursor.line));std::size_t length=std::min<std::size_t>(2048,row.size());
    while(length<row.size()&&length&&(static_cast<unsigned char>(row[length])&0xc0)==0x80)--length;
    std::string shown;for(char c:std::string_view(row).substr(0,length)){if(c=='\0')shown+="\\0";else shown+=c;}
    if(length<row.size())shown+="…";preview.set(std::move(shown));
    status.set("Lines: "+std::to_string(state.line_count)+" · File: "+std::to_string(state.file_backed_line_count)+
        " · Memory: "+std::to_string(state.memory_line_count)+" · Cursor line: "+std::to_string(state.cursor.line+1)+
        " · Revision: "+std::to_string(state.document_revision)+(state.dirty?" · modified":" · clean")+
        (state.loading?" · reading "+std::to_string(state.loaded_byte_count)+"/"+std::to_string(state.total_byte_count):"")+
        (state.last_error.empty()?"":" · "+state.last_error));
}
void TextDocumentSession::start_background(runtime::Application& app){
    std::weak_ptr<TextDocumentSession> weak=shared_from_this();auto sender=app.event_loop().sender();auto scope=jobs.sender();
    auto submitted=jobs.post(app.workers(),[weak,sender,scope]()mutable{const int answer=6*7;(void)scope.post(sender,[weak,answer]{if(auto p=weak.lock();p&&!p->retired){p->background_done=true;p->background.set("Background result: "+std::to_string(answer)+" · iiSDKCore");}});});
    if(!submitted)throw std::runtime_error("Background work admission failed");
}
void TextDocumentSession::retire(){if(retired)return;retired=true;jobs.request_close();(void)jobs.close();if(receiver)(void)receiver->close();receiver.reset();if(document)(void)document->close();document.reset();}
void TextDocumentBackend::next_line(){auto state=take(model->document->state());result(model,model->document->move_cursor((state.cursor.line+1)%state.line_count,0));}
void TextDocumentBackend::insert(){if(take(model->document->state()).loading){status.set("Wait for the document read to finish before editing this example.");return;}result(model,model->document->insert_text("Native "));}
void TextDocumentBackend::undo(){result(model,model->document->undo());}
void TextDocumentBackend::save(){const auto state=take(model->document->state());if(!state.has_file_path){status.set("Use --document PATH to select a destination.");return;}if(state.loading||state.loaded_byte_count!=state.total_byte_count||!state.last_error.empty()){status.set("Reload the complete source before saving this example.");return;}result(model,model->document->save_file());}
void TextDocumentBackend::reload(){if(!take(model->document->state()).has_file_path){status.set("Use --document PATH to select a source.");return;}result(model,model->document->reload_file());}
void TextDocumentBackend::close(){if(on_close)on_close();}
