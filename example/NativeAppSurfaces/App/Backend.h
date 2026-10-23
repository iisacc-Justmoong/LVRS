#pragma once
#include <LVRS/AppCard.h>
#include <LVRS/HierarchyToolbar.h>
#include <LVRS/ListFooter.h>
#include <iiSDKCore/Core/Property/Observable.h>
struct SurfaceModel final {
    ii::core::Observable<std::string> header_title{"Native workspace"},card_title{"Retained content"},card_subtitle{"AppCard body uses the existing C++ scope"},status{"Background result: pending"};
    ii::core::Observable<bool> compact{false};
    ii::core::Observable<double> width{792},height{260};
    ii::core::Observable<lvrs::AppCardShape> shape{lvrs::AppCardShape::round_rect};
    ii::core::Observable<std::string> toolbar_icon{"projectStructure"};
    ii::core::Observable<lvrs::HierarchyToolbarItems> toolbar_items;
    ii::core::Observable<std::shared_ptr<const lvrs::ListFooterSlot>> footer_first;
    unsigned toolbar_events=0;
    SurfaceModel();
};
class SurfaceBackend final {
public:
    inline static unsigned created=0,retired=0;
    std::shared_ptr<SurfaceModel> model=std::make_shared<SurfaceModel>();
    ii::core::Observable<std::string>& header_title=model->header_title;
    ii::core::Observable<std::string>& card_title=model->card_title;
    ii::core::Observable<std::string>& card_subtitle=model->card_subtitle;
    ii::core::Observable<std::string>& status=model->status;
    ii::core::Observable<bool>& compact=model->compact;
    ii::core::Observable<double>& width=model->width;
    ii::core::Observable<double>& height=model->height;
    ii::core::Observable<lvrs::AppCardShape>& shape=model->shape;
    ii::core::Observable<std::string>& toolbar_icon=model->toolbar_icon;
    ii::core::Observable<lvrs::HierarchyToolbarItems>& toolbar_items=model->toolbar_items;
    ii::core::Observable<std::shared_ptr<const lvrs::ListFooterSlot>>& footer_first=model->footer_first;
    std::function<void()> on_close;
    SurfaceBackend(){++created;}
    ~SurfaceBackend(){++retired;}
    SurfaceBackend(const SurfaceBackend&)=delete;
    SurfaceBackend& operator=(const SurfaceBackend&)=delete;
    void menu();
    void update();
    void round();
    void close();
    void list_icon(std::size_t,std::string);
    void footer_clicked(std::size_t,const lvrs::ListFooterSlot&);
    void toolbar_triggered(const lvrs::ToolbarTrigger&);
    void manual_clicked();
};
