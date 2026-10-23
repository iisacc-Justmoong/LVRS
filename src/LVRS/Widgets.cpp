#include "Widgets.h"
#include <cmath>
namespace lvrs {
ii::core::Result<ii::gui::Widget> Widgets::link(Widget parent,std::string_view id,LinkOptions options,LinkEvents events,LinkContentFactory content){auto result=Link::create(builder_,parent,id,std::move(options),std::move(events),std::move(content));if(!result)return std::unexpected(result.error());return result->root();}
ii::core::Result<ii::gui::Widget> Widgets::page_router(Widget parent,std::string_view id,const std::shared_ptr<PageRouter>& router,ii::gui::Size size){
    if(!router)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"PageRouter requires a scoped controller");return router->mount(builder_,parent,id,size);
}
Widgets::Result<Widgets::Widget> Widgets::table(Widget p,std::string_view id,TableOptions o,TableEvents e){auto r=Table::create(builder_,p,id,std::move(o),std::move(e));if(!r)return std::unexpected(r.error());return r->root();}
Widgets::Result<Widgets::Widget> Widgets::list(Widget p,std::string_view id,ListOptions o,ListEvents e){auto view=List::create(builder_,p,id,std::move(o),std::move(e));if(!view)return std::unexpected(view.error());return view->root();}
Widgets::Result<Widgets::Widget> Widgets::hierarchy(Widget p,std::string_view id,ListOptions o,ListEvents e){o.hierarchy=true;return list(p,id,std::move(o),std::move(e));}
Widgets::Result<Widgets::Widget> Widgets::list_item(Widget p,std::string_view id,ii::gui::ButtonOptions o,ii::runtime::Task action){o.tone=ii::gui::ButtonTone::borderless;return builder_.button(p,id,std::move(o),std::move(action));}
Widgets::Result<Widgets::Widget> Widgets::event_listener(Widget p,std::string_view id,ii::gui::Size size,ii::gui::WidgetInputHandler input,std::function<void(ii::gui::Rect)> layout){
    auto view=builder_.absolute_stack(p,id,size);if(!view)return view;
    if(auto result=view->set_input_handler(std::move(input));!result)return std::unexpected(result.error());
    if(auto result=view->set_layout_handler(std::move(layout));!result)return std::unexpected(result.error());return view;
}
Widgets::Result<Widgets::Widget> Widgets::toolbar_button(Widget p,std::string_view id,ToolbarButtonOptions o,ii::runtime::Task f){auto view=ToolbarButton::create(builder_,p,id,std::move(o),std::move(f));if(!view)return std::unexpected(view.error());return view->root();}
Widgets::Result<Widgets::Widget> Widgets::hierarchy_toolbar(Widget p,std::string_view id,HierarchyToolbarOptions o,std::vector<HierarchyToolbarEntry> entries,HierarchyToolbarEvents e){auto view=HierarchyToolbar::create(builder_,p,id,std::move(o),std::move(entries),std::move(e));if(!view)return std::unexpected(view.error());return view->root();}
Widgets::Result<Widgets::Widget> Widgets::list_toolbar(Widget p,std::string_view id,ListToolbarOptions o,ListToolbarEvents e){auto view=ListToolbar::create(builder_,p,id,std::move(o),std::move(e));if(!view)return std::unexpected(view.error());return view->root();}
Widgets::Result<Widgets::Widget> Widgets::list_footer(Widget p,std::string_view id,ListFooterOptions o,ListFooterEvents e){auto view=ListFooter::create(builder_,p,id,std::move(o),std::move(e));if(!view)return std::unexpected(view.error());return view->root();}
Widgets::Result<Widgets::Widget> Widgets::app_card(Widget p,std::string_view id,AppCardOptions options){
    auto card=AppCard::create(builder_,p,id,std::move(options));if(!card)return std::unexpected(card.error());return card->root();
}
Widgets::Result<Widgets::Widget> Widgets::app_header(Widget p,std::string_view id,AppHeaderOptions options,AppHeaderEvents events){auto header=AppHeader::create(builder_,p,id,std::move(options),std::move(events));if(!header)return std::unexpected(header.error());return header->root();}
Widgets::Result<Widgets::Widget> Widgets::table_cell_item(Widget p,std::string_view id,TableCellOptions o,TableCellEvents e){auto r=TableCellItem::create(builder_,p,id,std::move(o),std::move(e));if(!r)return std::unexpected(r.error());return r->root();}
Widgets::Result<Widgets::Widget> Widgets::table_row(Widget p,std::string_view id,TableRowOptions o,TableRowEvents e){auto r=TableRow::create(builder_,p,id,std::move(o),std::move(e));if(!r)return std::unexpected(r.error());return r->root();}
Widgets::Result<Widgets::Widget> Widgets::table_header(Widget p,std::string_view id,TableHeaderOptions o,TableHeaderEvents e){auto r=TableHeader::create(builder_,p,id,std::move(o),std::move(e));if(!r)return std::unexpected(r.error());return r->root();}
Widgets::Result<Widgets::Widget> Widgets::sheet(Widget p,std::string_view id,SheetOptions o,SheetEvents e,SheetContentFactory f){auto sheet=Sheet::create(builder_,p,id,std::move(o),std::move(e),std::move(f));if(!sheet)return std::unexpected(sheet.error());return sheet->root();}
Widgets::Result<Widgets::Widget> Widgets::alert(Widget p,std::string_view id,AlertOptions o,AlertEvents e){auto result=Alert::create(builder_,p,id,std::move(o),std::move(e));if(!result)return std::unexpected(result.error());return result->root();}
Widgets::Result<Widgets::Widget> Widgets::alert_button(Widget p,std::string_view id,AlertButtonOptions o,AlertButtonEvents e){auto result=AlertButton::create(builder_,p,id,std::move(o),std::move(e));if(!result)return std::unexpected(result.error());return result->root();}
Widgets::Result<Widgets::Widget> Widgets::modal(Widget p,std::string_view id,ModalOptions o,ModalEvents e,ModalContentFactory f){auto modal=Modal::create(builder_,p,id,std::move(o),std::move(e),std::move(f));if(!modal)return std::unexpected(modal.error());return modal->root();}
Widgets::Result<Widgets::Widget> Widgets::color_picker(Widget p,std::string_view id,ColorPickerOptions o,ColorPickerEvents e) {auto picker=ColorPicker::create(builder_,p,id,std::move(o),std::move(e));if(!picker)return std::unexpected(picker.error());return picker->root();}
Widgets::Result<Widgets::Widget> Widgets::color_surface(Widget p,std::string_view id,ii::gui::ColorSurfaceOptions o,std::function<void(ii::gui::WidgetColor)> edited) {
    if(o.field==ii::gui::ColorField::wheel && !o.hue_ring) {auto image=color_picker_hue_ring();if(!image)return std::unexpected(image.error());o.hue_ring=*image;}
    return builder_.color_surface(p,id,std::move(o),std::move(edited));
}
namespace {
auto stepper_icon(ii::gui::StepperArrow arrow) {
    return Icons::image(arrow==ii::gui::StepperArrow::up?"StepperUpBorderless":arrow==ii::gui::StepperArrow::down?"StepperDownBorderless":"StepperUpDownBorderless");
}
}
Widgets::Result<Widgets::Widget> Widgets::vstack(Widget p,std::string_view id,ii::gui::StackOptions o) { return builder_.vstack(p,id,o); }
Widgets::Result<Widgets::Widget> Widgets::hstack(Widget p,std::string_view id,ii::gui::StackOptions o) { return builder_.hstack(p,id,o); }
Widgets::Result<Widgets::Widget> Widgets::zstack(Widget p,std::string_view id,ii::gui::StackOptions o) { return builder_.zstack(p,id,o); }
Widgets::Result<Widgets::Widget> Widgets::spacer(Widget p,std::string_view id,double n) { return builder_.spacer(p,id,n); }
Widgets::Result<Widgets::Widget> Widgets::label(Widget p,std::string_view id,ii::gui::LabelOptions o) { return builder_.label(p,id,std::move(o)); }
Widgets::Result<Widgets::Widget> Widgets::button(Widget p,std::string_view id,ii::gui::ButtonOptions o,ii::runtime::Task f) {
    if(o.icon_mode && !o.icon) {
        o.icon_name=Icons::normalize(o.icon_name.empty()?"projectStructure":o.icon_name);
        auto image=Icons::image(o.icon_name);if(!image) return std::unexpected(image.error());o.icon=*image;
    }
    return builder_.button(p,id,std::move(o),std::move(f));
}
Widgets::Result<Widgets::Widget> Widgets::icon_button(Widget p,std::string_view id,ii::gui::ButtonOptions o,ii::runtime::Task f) { o.icon_mode=true;return button(p,id,std::move(o),std::move(f)); }
Widgets::Result<Widgets::Widget> Widgets::dropdown_button(Widget p,std::string_view id,ii::gui::ButtonOptions o,ii::runtime::Task f) {
    const auto name=o.tone==ii::gui::ButtonTone::borderless?"generalchevronDownBorderless":
        o.tone==ii::gui::ButtonTone::primary || o.tone==ii::gui::ButtonTone::destructive?"generalchevronDownAccent":"generalchevronDown";
    auto image=Icons::image(name);if(!image) return std::unexpected(image.error());o.indicator=*image;
    image=Icons::image("generalchevronDownDisabled");if(!image) return std::unexpected(image.error());o.disabled_indicator=*image;
    return button(p,id,std::move(o),std::move(f));
}
Widgets::Result<Widgets::Widget> Widgets::icon_menu_button(Widget p,std::string_view id,ii::gui::ButtonOptions o,ii::runtime::Task f) { o.icon_mode=true;return dropdown_button(p,id,std::move(o),std::move(f)); }
Widgets::Result<Widgets::Widget> Widgets::help_button(Widget p,std::string_view id,ii::gui::ButtonOptions o,ii::runtime::Task f) { o.help=true;if(o.text.empty()) o.text="?";if(o.accessibility_name.empty()) o.accessibility_name="Help";return button(p,id,std::move(o),std::move(f)); }
Widgets::Result<Widgets::Widget> Widgets::tab(Widget p,std::string_view id,ii::gui::TabOptions o,ii::runtime::Task f) {
    if(Icons::normalize(o.icon_name).empty()) return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Tab iconName is required");
    o.icon_name=Icons::normalize(o.icon_name);
    if(!o.icon) { auto image=Icons::image(o.icon_name);if(!image) return std::unexpected(image.error());o.icon=*image; }
    return builder_.tab(p,id,std::move(o),std::move(f));
}
Widgets::Result<Widgets::Widget> Widgets::tab_bar(Widget p,std::string_view id,ii::gui::TabBarOptions o,std::function<void(int)> f) { return builder_.tab_bar(p,id,o,std::move(f)); }
Widgets::Result<Widgets::Widget> Widgets::combo_box(Widget p,std::string_view id,ii::gui::ComboBoxOptions o,ii::gui::ControlEvents e) {
    if(!o.indicator) { auto image=stepper_icon(o.arrow);if(!image) return std::unexpected(image.error());o.indicator=*image; }
    return builder_.combo_box(p,id,std::move(o),std::move(e));
}
Widgets::Result<Widgets::Widget> Widgets::stepper(Widget p,std::string_view id,ii::gui::StepperOptions o,std::function<void(int)> f,ii::gui::ControlEvents e) {
    if(!o.icon) { auto image=stepper_icon(o.arrow);if(!image) return std::unexpected(image.error());o.icon=*image; }
    return builder_.stepper(p,id,std::move(o),std::move(f),std::move(e));
}
Widgets::Result<Widgets::Widget> Widgets::label_segmented_control(Widget p,std::string_view id,ii::gui::SegmentedOptions o) { return builder_.segmented_control(p,id,o); }
Widgets::Result<Widgets::Widget> Widgets::icon_segmented_control(Widget p,std::string_view id,ii::gui::SegmentedOptions o) { return builder_.segmented_control(p,id,o); }
Widgets::Result<Widgets::Widget> Widgets::menu(Widget p,std::string_view id,ii::gui::MenuOptions o,ii::gui::MenuEvents e) { return builder_.menu(p,id,std::move(o),std::move(e)); }
Widgets::Result<Widgets::Widget> Widgets::popover(Widget p,std::string_view id,ii::gui::PopoverOptions o,ii::gui::PopoverEvents e) {return builder_.popover(p,id,std::move(o),std::move(e));}
Widgets::Result<Widgets::Widget> Widgets::context_menu(Widget p,std::string_view id,ii::gui::MenuOptions o,ii::gui::MenuEvents e) { o.style=ii::gui::MenuStyle::context;return menu(p,id,std::move(o),std::move(e)); }
Widgets::Result<Widgets::Widget> Widgets::menu_item(Widget p,std::string_view id,ii::gui::MenuItemOptions o,ii::runtime::Task f) {
    if(o.show_icon_slot && !o.icon && !Icons::normalize(o.icon_name).empty()) {
        o.icon_name=Icons::normalize(o.icon_name);auto image=Icons::image(o.icon_name);if(!image) return std::unexpected(image.error());o.icon=*image;
    }
    if(o.show_chevron && o.has_child_items) {
        for(auto [name,target]:{std::pair{"generalchevronRight",&o.chevron_right},std::pair{"generalchevronLeft",&o.chevron_left},std::pair{"generalchevronUp",&o.chevron_up},std::pair{"generalchevronDown",&o.chevron_down}}) {
            if(!*target) {auto image=Icons::image(name);if(!image) return std::unexpected(image.error());*target=*image;}
        }
    }
    return builder_.menu_item(p,id,std::move(o),std::move(f));
}
Widgets::Result<Widgets::Widget> Widgets::context_menu_item(Widget p,std::string_view id,ii::gui::MenuItemOptions o,ii::runtime::Task f) { o.style=ii::gui::MenuStyle::context;return menu_item(p,id,std::move(o),std::move(f)); }
Widgets::Result<Widgets::Widget> Widgets::menu_divider(Widget p,std::string_view id,ii::gui::MenuDividerOptions o) { return builder_.menu_divider(p,id,std::move(o)); }
Widgets::Result<Widgets::Widget> Widgets::context_menu_divider(Widget p,std::string_view id,ii::gui::MenuDividerOptions o) {o.style=ii::gui::MenuStyle::context;return menu_divider(p,id,std::move(o));}
Widgets::Result<Widgets::Widget> Widgets::check_box(Widget p,std::string_view id,ii::gui::CheckOptions o,std::function<void(bool)> f) { return builder_.check_box(p,id,std::move(o),std::move(f)); }
Widgets::Result<Widgets::Widget> Widgets::radio_button(Widget p,std::string_view id,ii::gui::CheckOptions o,std::function<void(bool)> f) { return builder_.radio_button(p,id,std::move(o),std::move(f)); }
Widgets::Result<Widgets::Widget> Widgets::toggle_switch(Widget p,std::string_view id,ii::gui::CheckOptions o,std::function<void(bool)> f) { return builder_.toggle_switch(p,id,std::move(o),std::move(f)); }
Widgets::Result<Widgets::Widget> Widgets::input_field(Widget p,std::string_view id,ii::gui::InputOptions o,std::function<void(std::string)> f,ii::runtime::Task s) { return builder_.input_field(p,id,std::move(o),std::move(f),std::move(s)); }
Widgets::Result<Widgets::Widget> Widgets::slider(Widget p,std::string_view id,ii::gui::SliderOptions o,std::function<void(double)> f) { return builder_.slider(p,id,o,std::move(f)); }
Widgets::Result<Widgets::Widget> Widgets::progress(Widget p,std::string_view id,ii::gui::ProgressOptions o) { return builder_.progress(p,id,o); }
} // namespace lvrs
