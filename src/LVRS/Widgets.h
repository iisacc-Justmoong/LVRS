#pragma once
#include "Theme.h"
#include "Icons.h"
#include "ColorPicker.h"
#include "ColorPickerButton.h"
#include "Modal.h"
#include "Sheet.h"
#include "Alert.h"
#include "TableCellItem.h"
#include "TableRow.h"
#include "TableHeader.h"
#include "Table.h"
#include "List.h"
#include "PageRouter.h"
#include "Link.h"
#include "AppHeader.h"
#include "HierarchyToolbar.h"
#include "ListToolbar.h"
#include "ListFooter.h"
#include <iiGUI/NativeWidgets.h>

namespace lvrs {
// A factory-scoped component API. Every call constructs an iiGUI C++ widget;
// the OS supplies no control, control state or layout for these components.
class Widgets final {
public:
    explicit Widgets(ii::gui::WidgetBuilder& builder):builder_(builder) {}
    using Widget=ii::gui::Widget;
    template<class T> using Result=ii::core::Result<T>;
    [[nodiscard]] Result<Widget> vstack(Widget,std::string_view,ii::gui::StackOptions={});
    [[nodiscard]] Result<Widget> hstack(Widget,std::string_view,ii::gui::StackOptions={});
    [[nodiscard]] Result<Widget> zstack(Widget,std::string_view,ii::gui::StackOptions={});
    [[nodiscard]] Result<Widget> spacer(Widget,std::string_view,double minimum_length=0);
    [[nodiscard]] Result<Widget> table_cell_item(Widget,std::string_view,TableCellOptions={},TableCellEvents={});
    [[nodiscard]] Result<Widget> table_row(Widget,std::string_view,TableRowOptions={},TableRowEvents={});
    [[nodiscard]] Result<Widget> table_header(Widget,std::string_view,TableHeaderOptions={},TableHeaderEvents={});
    [[nodiscard]] Result<Widget> table(Widget,std::string_view,TableOptions={},TableEvents={});
    [[nodiscard]] Result<Widget> list(Widget,std::string_view,ListOptions={},ListEvents={});
    [[nodiscard]] Result<Widget> hierarchy(Widget,std::string_view,ListOptions={},ListEvents={});
    [[nodiscard]] Result<Widget> page_router(Widget,std::string_view,const std::shared_ptr<PageRouter>&,ii::gui::Size={640,400});
    [[nodiscard]] Result<Widget> link(Widget,std::string_view,LinkOptions={},LinkEvents={},LinkContentFactory={});
    [[nodiscard]] Result<Widget> list_item(Widget,std::string_view,ii::gui::ButtonOptions={},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> toolbar_button(Widget,std::string_view,ToolbarButtonOptions={},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> hierarchy_toolbar(Widget,std::string_view,HierarchyToolbarOptions={},std::vector<HierarchyToolbarEntry> = {},HierarchyToolbarEvents={});
    [[nodiscard]] Result<Widget> list_toolbar(Widget,std::string_view,ListToolbarOptions={},ListToolbarEvents={});
    [[nodiscard]] Result<Widget> list_footer(Widget,std::string_view,ListFooterOptions={},ListFooterEvents={});
    [[nodiscard]] Result<Widget> app_card(Widget,std::string_view,AppCardOptions={});
    [[nodiscard]] Result<Widget> app_header(Widget,std::string_view,AppHeaderOptions={},AppHeaderEvents={});
    [[nodiscard]] Result<Widget> event_listener(Widget,std::string_view,ii::gui::Size,ii::gui::WidgetInputHandler={},std::function<void(ii::gui::Rect)> = {});
    [[nodiscard]] Result<Widget> label(Widget,std::string_view,ii::gui::LabelOptions={});
    [[nodiscard]] Result<Widget> button(Widget,std::string_view,ii::gui::ButtonOptions={},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> icon_button(Widget,std::string_view,ii::gui::ButtonOptions={},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> dropdown_button(Widget,std::string_view,ii::gui::ButtonOptions={},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> icon_menu_button(Widget,std::string_view,ii::gui::ButtonOptions={},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> help_button(Widget,std::string_view,ii::gui::ButtonOptions={},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> tab(Widget,std::string_view,ii::gui::TabOptions,ii::runtime::Task={});
    [[nodiscard]] Result<Widget> tab_bar(Widget,std::string_view,ii::gui::TabBarOptions={},std::function<void(int)> = {});
    [[nodiscard]] Result<Widget> combo_box(Widget,std::string_view,ii::gui::ComboBoxOptions={},ii::gui::ControlEvents={});
    [[nodiscard]] Result<Widget> stepper(Widget,std::string_view,ii::gui::StepperOptions={},std::function<void(int)> = {},ii::gui::ControlEvents={});
    [[nodiscard]] Result<Widget> label_segmented_control(Widget,std::string_view,ii::gui::SegmentedOptions={});
    [[nodiscard]] Result<Widget> icon_segmented_control(Widget,std::string_view,ii::gui::SegmentedOptions={.vertical_padding=4});
    [[nodiscard]] Result<Widget> menu(Widget,std::string_view,ii::gui::MenuOptions={},ii::gui::MenuEvents={});
    [[nodiscard]] Result<Widget> popover(Widget,std::string_view,ii::gui::PopoverOptions={},ii::gui::PopoverEvents={});
    [[nodiscard]] Result<Widget> modal(Widget,std::string_view,ModalOptions={},ModalEvents={},ModalContentFactory={});
    [[nodiscard]] Result<Widget> sheet(Widget,std::string_view,SheetOptions={},SheetEvents={},SheetContentFactory={});
    [[nodiscard]] Result<Widget> alert(Widget,std::string_view,AlertOptions={},AlertEvents={});
    [[nodiscard]] Result<Widget> alert_button(Widget,std::string_view,AlertButtonOptions={},AlertButtonEvents={});
    [[nodiscard]] Result<Widget> context_menu(Widget,std::string_view,ii::gui::MenuOptions={},ii::gui::MenuEvents={});
    [[nodiscard]] Result<Widget> menu_item(Widget,std::string_view,ii::gui::MenuItemOptions={},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> context_menu_item(Widget,std::string_view,ii::gui::MenuItemOptions={},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> menu_divider(Widget,std::string_view,ii::gui::MenuDividerOptions={});
    [[nodiscard]] Result<Widget> context_menu_divider(Widget,std::string_view,ii::gui::MenuDividerOptions={});
    [[nodiscard]] Result<Widget> check_box(Widget,std::string_view,ii::gui::CheckOptions={},std::function<void(bool)> = {});
    [[nodiscard]] Result<Widget> radio_button(Widget,std::string_view,ii::gui::CheckOptions={},std::function<void(bool)> = {});
    [[nodiscard]] Result<Widget> toggle_switch(Widget,std::string_view,ii::gui::CheckOptions={},std::function<void(bool)> = {});
    [[nodiscard]] Result<Widget> input_field(Widget,std::string_view,ii::gui::InputOptions={},std::function<void(std::string)> = {},ii::runtime::Task={});
    [[nodiscard]] Result<Widget> slider(Widget,std::string_view,ii::gui::SliderOptions={},std::function<void(double)> = {});
    [[nodiscard]] Result<Widget> progress(Widget,std::string_view,ii::gui::ProgressOptions={});
    [[nodiscard]] Result<Widget> color_surface(Widget,std::string_view,ii::gui::ColorSurfaceOptions,std::function<void(ii::gui::WidgetColor)> = {});
    [[nodiscard]] Result<Widget> color_picker(Widget,std::string_view,ColorPickerOptions={},ColorPickerEvents={});
    [[nodiscard]] Result<Widget> color_picker_button(Widget,std::string_view,ColorPickerButtonOptions={},ii::gui::ControlEvents={});
private:
    ii::gui::WidgetBuilder& builder_;
};
} // namespace lvrs
