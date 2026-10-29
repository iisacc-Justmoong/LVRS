#pragma once
#include <LVRS/ToolbarButton.h>
#include <iiSDKCore/Core/Value/Value.h>
namespace lvrs {
struct ToolbarEvent final {std::string name;ii::core::Value payload;};
struct ToolbarTrigger final {
    ii::gui::Widget button;ToolbarId button_id=std::int64_t(-1);int index=-1;
    ii::core::Value item;std::string event_name;ii::core::Value payload;
    std::function<ii::core::Result<void>(std::string_view,ii::core::Value)> emit;
    std::function<ii::core::Result<bool>(int)> activate;
};
struct HierarchyToolbarEntry final {
    std::optional<ToolbarId> button_id;
    ToolbarButtonOptions button;
    bool selected=false;
    ii::core::Value item;
    std::vector<ToolbarEvent> events;
    std::function<void(const ToolbarTrigger&)> on_triggered;
};
using HierarchyToolbarItems=std::shared_ptr<const std::vector<HierarchyToolbarEntry>>;
[[nodiscard]] const std::vector<HierarchyToolbarEntry>& toolbar_items(const std::vector<HierarchyToolbarEntry>&);
[[nodiscard]] const std::vector<HierarchyToolbarEntry>& toolbar_items(const HierarchyToolbarItems&);
struct HierarchyToolbarOptions final {
    double minimum_width=200,horizontal_padding=8,vertical_padding=2,spacing=0,slot_size=22;
    ii::gui::WidgetColor background{.0951643139,.0965511352,.0979379490,1};
    double background_opacity=0;
    bool distribute_spacing=false,enabled=true;
    double width=0,height=0;
};
struct ToolbarSelection final {ii::gui::Widget button;ToolbarId button_id=std::int64_t(-1);int index=-1;};
struct HierarchyToolbarEvents final {
    std::function<void(const ToolbarSelection&)> active_changed;
    std::function<void(const ToolbarTrigger&)> button_triggered;
    std::function<void(const ToolbarEvent&,const ToolbarTrigger&)> button_event_triggered;
};
using ToolbarContentFactory=std::function<ii::core::Result<void>(ii::gui::WidgetBuilder&,ii::gui::Widget)>;
class HierarchyToolbar final {
public:
    HierarchyToolbar()=default;
    [[nodiscard]] static ii::core::Result<HierarchyToolbar> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,HierarchyToolbarOptions={},std::vector<HierarchyToolbarEntry> = {},HierarchyToolbarEvents={},ToolbarContentFactory={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget manual_root()const noexcept{return manual_;}
    [[nodiscard]] ii::core::Result<HierarchyToolbarOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(HierarchyToolbarOptions)const;
    [[nodiscard]] ii::core::Result<void> set_items(std::vector<HierarchyToolbarEntry>)const;
    [[nodiscard]] ii::core::Result<std::vector<HierarchyToolbarEntry>> items()const;
    [[nodiscard]] ii::core::Result<void> register_button(ToolbarButton,bool normalize_now=true)const;
    [[nodiscard]] ii::core::Result<void> normalize()const;
    [[nodiscard]] ii::core::Result<void> set_active_id(ToolbarId)const;
    [[nodiscard]] ii::core::Result<ToolbarSelection> selection()const;
    [[nodiscard]] ii::core::Result<ii::gui::Widget> button_at(int)const;
    [[nodiscard]] ii::core::Result<int> button_count()const;
    [[nodiscard]] ii::core::Result<int> visible_button_count()const;
    [[nodiscard]] ii::core::Result<bool> trigger_index(int)const;
    [[nodiscard]] ii::core::Result<double> distributed_spacing()const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_,manual_;
    std::weak_ptr<Impl> state_;
};
}
