#pragma once
#include <LVRS/HierarchyItem.h>
namespace lvrs {
struct HierarchyDelegateState final {
    std::uint64_t revision=1,applied_revision=0;
    bool has_item=false,legacy=false,closed=false;
};
// Per-mounted-row view input and native product. Copies share the same source;
// the item handle is weak and never owns a document or a widget generation.
class HierarchyDelegateContext final {
public:
    [[nodiscard]] static ii::core::Result<HierarchyDelegateContext> create(HierarchyItemOptions,HierarchyItemEvents={});
    [[nodiscard]] ii::core::Result<HierarchyItemOptions> options()const;
    [[nodiscard]] ii::core::Result<HierarchyDelegateState> state()const;
    [[nodiscard]] ii::core::Result<HierarchyItem> item()const;
    [[nodiscard]] ii::core::Result<HierarchyItem> create_item(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,HierarchyItemOptions,HierarchyItemEvents={},HierarchyCountFactory={})const;
    [[nodiscard]] ii::core::Result<void> apply(HierarchyItemOptions)const;
    [[nodiscard]] ii::core::Result<void> adopt_legacy(HierarchyItem)const;
    [[nodiscard]] ii::core::Result<bool> update_source(HierarchyItemOptions)const;
    [[nodiscard]] ii::core::Result<void> close()const;
private:
    struct Impl;std::shared_ptr<Impl> state_;
};
}
