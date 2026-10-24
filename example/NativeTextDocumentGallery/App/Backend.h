#pragma once
#include <LVRS/TextDocumentModel.h>
#include <iiSDKCore/Core/Property/Observable.h>
#include <iiSDKCore/Runtime/Application.h>

struct TextDocumentSession final : std::enable_shared_from_this<TextDocumentSession> {
    ii::core::Observable<std::string> status{"Preparing document"}, preview{""}, background{"Background: pending"};
    std::shared_ptr<lvrs::TextDocumentModel> document;
    std::shared_ptr<lvrs::TextDocumentPort> receiver;
    ii::runtime::TaskScope jobs;
    bool retired=false, background_done=false;
    void initialize(ii::runtime::Application&,const std::filesystem::path&);
    void refresh(const lvrs::TextDocumentState&);
    void retire();
    void start_background(ii::runtime::Application&);
};
class TextDocumentBackend final {
public:
    inline static unsigned created=0,retired=0;
    std::shared_ptr<TextDocumentSession> model=std::make_shared<TextDocumentSession>();
    ii::core::Observable<std::string>& status=model->status;
    ii::core::Observable<std::string>& preview=model->preview;
    ii::core::Observable<std::string>& background=model->background;
    std::function<void()> on_close;
    TextDocumentBackend(){++created;}
    ~TextDocumentBackend(){model->retire();++retired;}
    TextDocumentBackend(const TextDocumentBackend&)=delete;
    TextDocumentBackend& operator=(const TextDocumentBackend&)=delete;
    void next_line();void insert();void undo();void save();void reload();void close();
};
