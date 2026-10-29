#pragma once
#include "HierarchyController.h"
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
struct HierarchyItemOptions final {
    [[nodiscard]] bool operator==(const HierarchyItemOptions&)const;
    HierarchyRow row{.label="Label",.has_child_items=true};
    std::weak_ptr<HierarchyController> controller;
    double width=200,row_height=20,indent_step=8,icon_size=16,chevron_size=16;
    double left_padding=8,right_padding=8,spacing=2,corner_radius=5,count_width=24;
    bool drag_preview=false;
    double drag_opacity=.45;
    ii::gui::WidgetColor text{1,1,1,.8},disabled_text{1,1,1,.3},description{1,1,1,.6};
    ii::gui::WidgetColor idle{0,0,0,0},hover{.05507083609700203,.05517596751451492,.05570162087678909,1};
    ii::gui::WidgetColor pressed{.12302529811859131,.12528030574321747,.12753531336784363,1};
    ii::gui::WidgetColor active{.12618358505561073,.23337636029019077,.3558823529411765,1},inactive{.19355684518814087,.1978072077035904,.19993239641189575,1};
    ii::gui::WidgetColor drag{.12618358505561073,.23337636029019077,.3558823529411765,1};
};
struct HierarchyItemEvents final {
    ii::runtime::Task activated;
    std::function<void(bool)> expansion_changed;
    // Main-row local coordinates; true suppresses the release activation.
    ii::gui::WidgetInputHandler pointer;
};
class HierarchyItem;
using HierarchyCountFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget,const HierarchyItem&)>;
class HierarchyItem final {
public:
    [[nodiscard]] static ii::core::Result<void> validate_options(const HierarchyItemOptions&);
    [[nodiscard]] static ii::core::Result<HierarchyItem> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,HierarchyItemOptions={},HierarchyItemEvents={},HierarchyCountFactory={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<HierarchyItemOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(HierarchyItemOptions)const;
    [[nodiscard]] ii::core::Result<void> set_selected(bool)const;
    [[nodiscard]] ii::core::Result<void> set_expanded(bool)const;
    [[nodiscard]] ii::core::Result<void> activate()const;
    [[nodiscard]] ii::core::Result<void> toggle_expanded()const;
    [[nodiscard]] ii::core::Result<HierarchyMetadata> metadata()const;
    [[nodiscard]] ii::core::Result<ii::gui::Widget> count_content(std::string_view local_id={})const;
    [[nodiscard]] ii::core::Result<std::shared_ptr<const ii::gui::WidgetDrawing>> drawing(ii::gui::WidgetPaintState)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
