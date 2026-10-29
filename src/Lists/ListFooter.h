#pragma once
#include <LVRS/ToolbarButton.h>
#include <array>
namespace lvrs {
struct ListFooterSlot final {ToolbarButtonOptions button;ii::runtime::Task on_clicked;};
[[nodiscard]] const ListFooterSlot& footer_slot(const ListFooterSlot&);
[[nodiscard]] const ListFooterSlot& footer_slot(const std::shared_ptr<const ListFooterSlot>&);
struct ListFooterOptions final {
    std::array<ListFooterSlot,3> buttons=[] {
        std::array<ListFooterSlot,3> slots;
        slots[0].button.icon_name="addFile";slots[1].button.icon_name="generaldelete";slots[2].button.icon_name="settings";
        slots[2].button.menu=true;slots[2].button.left_padding=4;slots[2].button.content_spacing=-2;
        return slots;
    }();
    double horizontal_padding=2,vertical_padding=2,spacing=0;
    bool interactive=true,enabled=true;
    double width=0,height=0;
};
struct ListFooterEvents final {std::function<void(std::size_t,const ListFooterSlot&)> button_clicked;};
class ListFooter final {
public:
    ListFooter()=default;
    [[nodiscard]] static ii::core::Result<ListFooter> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,ListFooterOptions={},ListFooterEvents={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<ListFooterOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(ListFooterOptions)const;
    [[nodiscard]] ii::core::Result<void> set_slot(int,ListFooterSlot)const;
    [[nodiscard]] ii::core::Result<ii::gui::Widget> button_at(int)const;
    [[nodiscard]] ii::core::Result<void> set_events(ListFooterEvents)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;std::weak_ptr<Impl> state_;
};
}
