#include "Backend.h"
SurfaceModel::SurfaceModel(){
    std::vector<lvrs::HierarchyToolbarEntry> items(3);
    for(int i=0;i<3;++i){items[i].button_id=std::string("tool-")+std::to_string(i);items[i].button.icon_name=i==2?"settings":"projectStructure";items[i].events={{"workspace.tool",ii::core::Value(i)}};}
    toolbar_items.set(std::make_shared<const std::vector<lvrs::HierarchyToolbarEntry>>(std::move(items)));
    lvrs::ListFooterSlot first;first.button.icon_name="addFile";footer_first.set(std::make_shared<const lvrs::ListFooterSlot>(std::move(first)));
}
void SurfaceBackend::menu(){model->compact.set(!model->compact.get());model->header_title.set(model->compact.get()?"Compact native workspace":"Native workspace");}
void SurfaceBackend::update(){
    model->card_title.set("Updated without reconstruction");model->card_subtitle.set("The editor below retains its original native handle and text");model->toolbar_icon.set("settings");
    auto items=*model->toolbar_items.get();items[0].button.icon_name="settings";model->toolbar_items.set(std::make_shared<const std::vector<lvrs::HierarchyToolbarEntry>>(std::move(items)));
    auto first=*model->footer_first.get();first.button.icon_name="settings";model->footer_first.set(std::make_shared<const lvrs::ListFooterSlot>(std::move(first)));
}
void SurfaceBackend::round(){model->shape.set(model->shape.get()==lvrs::AppCardShape::round_rect?lvrs::AppCardShape::cylinder:lvrs::AppCardShape::round_rect);}
void SurfaceBackend::close(){if(on_close)on_close();}
void SurfaceBackend::list_icon(std::size_t index,std::string source){++model->toolbar_events;model->status.set("List toolbar "+std::to_string(index)+": "+source);}
void SurfaceBackend::footer_clicked(std::size_t index,const lvrs::ListFooterSlot&){++model->toolbar_events;model->status.set("Footer slot "+std::to_string(index));}
void SurfaceBackend::toolbar_triggered(const lvrs::ToolbarTrigger& context){++model->toolbar_events;model->status.set("Hierarchy tool "+std::to_string(context.index));}
void SurfaceBackend::manual_clicked(){++model->toolbar_events;model->status.set("Retained manual tool");}
