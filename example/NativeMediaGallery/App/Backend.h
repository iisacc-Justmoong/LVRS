#pragma once
#include <LVRS/Card.h>
#include <iiSDKCore/Core/Property/Observable.h>
struct MediaModel final {
    ii::core::Observable<std::shared_ptr<const ii::gui::RasterImage>> image;
    ii::core::Observable<std::string> title{"Native image"},status{"Background image: pending"};
    ii::core::Observable<bool> selected{false};
    ii::core::Observable<double> progress{.42},preview_width{256};
    ii::core::Observable<std::vector<lvrs::CardRow>> rows{std::vector<lvrs::CardRow>{{"Files","1,024"},{"Size","256 MB"}}};
    ii::core::Observable<lvrs::CardComponent> preview,icon;
    unsigned menu_events=0,action_events=0;
};
class MediaBackend final {
public:
    inline static unsigned created=0,retired=0;
    std::shared_ptr<MediaModel> model=std::make_shared<MediaModel>();
    ii::core::Observable<std::shared_ptr<const ii::gui::RasterImage>>& image=model->image;
    ii::core::Observable<std::string>& title=model->title;ii::core::Observable<std::string>& status=model->status;
    ii::core::Observable<bool>& selected=model->selected;
    ii::core::Observable<double>& progress=model->progress;ii::core::Observable<double>& preview_width=model->preview_width;
    ii::core::Observable<std::vector<lvrs::CardRow>>& rows=model->rows;
    ii::core::Observable<lvrs::CardComponent>& preview=model->preview;ii::core::Observable<lvrs::CardComponent>& icon=model->icon;
    std::function<void()> on_close;
    MediaBackend();~MediaBackend(){++retired;}
    MediaBackend(const MediaBackend&)=delete;MediaBackend& operator=(const MediaBackend&)=delete;
    void select(bool);void update();void menu();void action();void close();
};
