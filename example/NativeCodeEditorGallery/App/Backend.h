#pragma once
#include <LVRS/TextDocumentModel.h>
#include <iiSDKCore/Core/Property/Observable.h>
#include <iiSDKCore/Runtime/Application.h>

struct CodeEditorSession final: std::enable_shared_from_this<CodeEditorSession> {
    std::shared_ptr<lvrs::TextDocumentModel> document;
    std::shared_ptr<lvrs::TextDocumentPort> receiver;
    ii::core::Observable<std::string> status{"Preparing native editor"},background{"Background: pending"},title{"Example.cpp"};
    ii::core::Observable<double> width{428},height{360};
    ii::core::Observable<bool> locked{false};
    ii::runtime::TaskScope jobs;
    bool retired=false,background_done=false;
    void initialize(ii::runtime::Application&,const std::filesystem::path&);
    void start_background(ii::runtime::Application&);
    void refresh(const lvrs::TextDocumentState&);
    void retire();
};
class CodeEditorBackend final {
public:
    inline static unsigned created=0,retired=0;
    std::shared_ptr<CodeEditorSession> session=std::make_shared<CodeEditorSession>();
    std::shared_ptr<lvrs::TextDocumentModel>& document=session->document;
    ii::core::Observable<std::string>& status=session->status;
    ii::core::Observable<std::string>& background=session->background;
    ii::core::Observable<std::string>& title=session->title;
    ii::core::Observable<double>& editorWidth=session->width;
    ii::core::Observable<double>& editorHeight=session->height;
    ii::core::Observable<bool>& locked=session->locked;
    std::function<void()> on_close;
    CodeEditorBackend(){++created;}
    ~CodeEditorBackend(){session->retire();++retired;}
    CodeEditorBackend(const CodeEditorBackend&)=delete;
    CodeEditorBackend& operator=(const CodeEditorBackend&)=delete;
    void undo();void redo();void toggle_lock();void save();void close();
    void changed(const lvrs::TextDocumentChange&);
};
