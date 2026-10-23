#include "TableCellItem.h"
#include "TableDetail.h"
namespace lvrs {
namespace {
constexpr double fonts[]{26,22,17,15,13,12,11,11};
}
struct TableCellItem::Impl final {
    TableCellOptions options;TableCellEvents events;TableCellState value;ii::gui::Widget root,selection,divider,label,editor;ii::gui::WidgetColor primary;bool validating=false;std::uint64_t revision=0;
    ii::core::Result<void> paint(){
        using namespace ii;using namespace table_detail;
        try {
            const auto& o=options;const auto font=fonts[unsigned(o.text_style)],left=o.show_divider?1+o.content_spacing:0.,width=std::max(1.,o.width-left);
            good(root.set_extent({o.width,o.cell_height}));good(root.set_clip_content(o.clip_content));
            auto tint=o.selection_color.value_or(gui::WidgetColor{primary.red,primary.green,primary.blue,primary.alpha*(31./255)});
            auto ink=table_detail::ink(o.width,o.cell_height,o.selected?tint:gui::WidgetColor{0,0,0,0});ink.border=o.current_border_color.value_or(primary);ink.border_width=o.current?o.current_border_width:0;
            good(selection.set_image_options(std::move(ink)));good(selection.set_frame({0,0,float(o.width),float(o.cell_height)}));
            good(divider.set_image_options(table_detail::ink(1,o.cell_height,o.divider_color)));good(divider.set_frame({0,0,1,float(o.cell_height)}));good(divider.set_visible(o.show_divider));
            good(label.set_label_options({.text=value.input_result,.font_size=font,.role=o.text_style,.color=o.text_color,.line_height=font,.elide_right=true}));good(label.set_frame({float(left),float((o.cell_height-font)/2),float(width),float(font)}));good(label.set_visible(!o.inputable.value_or(false)));
            good(editor.set_input_options({.text=value.input_result,.enabled=o.enabled,.width=width,.style=gui::InputStyle{.height=font,.font_size=13,.line_height=13,.horizontal_padding=0,.corner_radius=0,.background=gui::WidgetColor{0,0,0,0},.text=o.text_color,.focus_ring=false}}));
            good(editor.set_frame({float(left),float((o.cell_height-font)/2),float(width),float(font)}));good(editor.set_visible(o.inputable.value_or(false)));return {};
        }catch(const BuildError& e){return std::unexpected(e.error);}
    }
    ii::core::Result<std::string> apply(std::string_view input){
        using namespace ii;using namespace table_detail;
        if(!text(input))return core::fail(core::ErrorCode::invalid_argument,"Invalid cell input");
        if(validating)return core::fail(core::ErrorCode::busy,"Cell validator is already active");
        const auto previous=revision;TableValidation result;auto validator=options.validator;auto type=options.value_type;
        struct Reset final {bool& value;~Reset(){value=false;}} reset{validating};validating=true;
        try {result=validator?validator(input,type):TableValidation{true,std::string(input),std::string(input)};}
        catch(...){return core::fail(core::ErrorCode::failed,"Cell validator threw",std::current_exception());}
        if(auto r=root.snapshot();!r)return std::unexpected(r.error());
        if(revision!=previous)return core::fail(core::ErrorCode::busy,"Cell options changed during validation");
        if(result.accepted&&(!text(result.text)||(std::holds_alternative<double>(result.value)&&!std::isfinite(std::get<double>(result.value)))||(std::holds_alternative<std::string>(result.value)&&!text(std::get<std::string>(result.value)))))return core::fail(core::ErrorCode::invalid_argument,"Validator returned invalid text/value");
        value.input_accepted=result.accepted;
        if(!result.accepted){good(editor.set_text(value.input_result));auto callback=events.rejected;if(callback)callback(std::string(input),type);return value.input_result;}
        value.input_result=std::move(result.text);value.typed_value=std::move(result.value);value.has_input_override=true;
        good(label.set_text(value.input_result));good(editor.set_text(value.input_result));return value.input_result;
    }
};
ii::core::Result<TableCellItem> TableCellItem::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,std::string_view id,TableCellOptions o,TableCellEvents e){
    using namespace ii;using namespace table_detail;if(!valid_cell(o))return core::fail(core::ErrorCode::invalid_argument,"Invalid TableCellItem options");
    try {
        auto s=std::make_shared<Impl>();s->options=o;s->events=std::move(e);s->value={o.text,o.text,true,false};s->primary=take(b.appearance()).primary;const auto prefix=std::string(id);
        s->root=take(b.absolute_stack(parent,id,{o.width,o.cell_height}));s->selection=take(b.image(s->root,prefix+".selection",ink(o.width,o.cell_height,{0,0,0,0})));s->divider=take(b.image(s->root,prefix+".divider",ink(1,o.cell_height,o.divider_color)));s->label=take(b.label(s->root,prefix+".label"));
        std::weak_ptr<Impl> weak=s;
        s->editor=take(b.input_field(s->root,prefix+".editor",{},[weak](std::string input){if(auto p=weak.lock()){const auto revision=p->revision;auto r=p->apply(input);if(!r){if(r.error().code!=core::ErrorCode::closed)throw BuildError{r.error()};return;}if(p->value.input_accepted&&p->revision==revision){auto callback=p->events.edited;if(callback)callback(*r);}}},[weak]{if(auto p=weak.lock()){auto input=p->editor.snapshot();if(!input)return;const auto revision=p->revision;auto r=p->apply(input->text);if(!r){if(r.error().code!=core::ErrorCode::closed)throw BuildError{r.error()};return;}if(p->value.input_accepted&&p->revision==revision){auto callback=p->events.submitted;if(callback)callback(*r);}}}));
        good(s->paint());good(b.retain(s));TableCellItem cell;cell.root_=s->root;cell.editor_=s->editor;cell.state_=s;return cell;
    }catch(const BuildError& error){return std::unexpected(error.error);}
}
ii::core::Result<TableCellState> TableCellItem::snapshot()const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Cell closed");return s->value;}
ii::core::Result<std::string> TableCellItem::apply_input_result(std::string_view input)const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Cell closed");try{return s->apply(input);}catch(const table_detail::BuildError& e){return std::unexpected(e.error);}}
ii::core::Result<void> TableCellItem::set_options(TableCellOptions o)const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());if(!table_detail::valid_cell(o))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid cell options");auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Cell closed");s->options=std::move(o);++s->revision;s->value={s->options.text,s->options.text,true,false};return s->paint();}
ii::core::Result<void> TableCellItem::set_selected(bool selected,bool current)const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Cell closed");s->options.selected=selected;s->options.current=current;return s->paint();}
}
