#pragma once
#include "ListModel.h"
#include "AppCard.h"
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
struct ListOptions final {
    std::shared_ptr<ListModel> model;
    double width=240,height=400,row_height=28,indent=16;
    bool hierarchy=false,reorderable=true,editable=true,show_checks=false;
    ii::gui::WidgetColor background{.1,.1,.1,1},selection_color{.18,.31,.52,1},text_color{.92,.92,.92,1};
    double selection_radius=3;
    double font_size=13;
};
struct ListEvents final {
    std::function<void(std::string)> selected,activated;
    std::function<void(std::string,std::string)> renamed;
    std::function<void(std::string,std::string,std::string)> moved;
    std::function<void(std::string,bool)> checked;
};
struct ListViewState final {std::string selected_key;double scroll_offset=0;std::size_t visible_rows=0,painted_rows=0;bool editing=false;};
class List final {
public:
    List()=default;
    [[nodiscard]] static ii::core::Result<List> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,ListOptions={},ListEvents={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<ListViewState> snapshot()const;
    [[nodiscard]] ii::core::Result<void> begin_rename()const;
    [[nodiscard]] ii::core::Result<void> set_scroll_offset(double)const;
private:
    struct Impl;
    ii::gui::Widget root_;
    std::weak_ptr<Impl> state_;
};
}
