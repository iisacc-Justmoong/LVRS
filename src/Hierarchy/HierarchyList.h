#pragma once
#include <LVRS/HierarchyDelegateContext.h>
namespace lvrs {
struct HierarchyListOptions final {
    std::shared_ptr<HierarchyModel> model;
    std::shared_ptr<HierarchyController> controller;
    double width=200,height=400;
    HierarchyItemOptions item;
    bool keyboard_navigation=true,enabled=true;
    std::optional<bool> auto_expand_ancestors,editable;
    std::optional<double> row_extent;
    std::size_t overscan=2,maximum_mounted_rows=512;
};
struct HierarchyListEvents final {
    std::function<void(std::string,std::int64_t,std::size_t)> active_changed;
    std::function<void(std::string,bool)> expansion_changed;
    std::function<void(double,double)> ensure_visible;
    std::function<void(const HierarchyMovePlan&)> item_moved;
};
struct HierarchyListState final {
    HierarchyControllerState logical;
    double scroll_offset=0,viewport_height=0;
    std::size_t mounted_rows=0;
    std::vector<std::string> mounted_keys;
};
using HierarchyItemFactory=std::function<ii::core::Result<HierarchyItem>(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,HierarchyItemOptions,HierarchyItemEvents)>;
using HierarchyDelegateFactory=std::function<ii::core::Result<HierarchyItem>(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,HierarchyDelegateContext)>;
class HierarchyList final {
public:
    [[nodiscard]] static ii::core::Result<HierarchyList> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,HierarchyListOptions={},HierarchyListEvents={},HierarchyItemFactory={},HierarchyDelegateFactory={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<HierarchyListState> state()const;
    // A source change stages new rows before publication. Model sources carry
    // a surviving active key; explicit controllers retain their own selection.
    [[nodiscard]] ii::core::Result<void> set_options(HierarchyListOptions)const;
    [[nodiscard]] ii::core::Result<std::shared_ptr<HierarchyController>> controller()const;
    [[nodiscard]] ii::core::Result<HierarchyItem> mounted_item(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> set_scroll_offset(double)const;
    [[nodiscard]] ii::core::Result<void> ensure_active_visible()const;
    [[nodiscard]] ii::core::Result<void> set_enabled(bool)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
