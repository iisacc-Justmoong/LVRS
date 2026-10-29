#include "InputField.h"
#include "Icons.h"
#include "TableDetail.h"

namespace lvrs {
ii::gui::InputStyle detail::input_field_style(InputFieldShape shape,InputFieldMode mode,bool enabled) {
    const auto radius=shape==InputFieldShape::inline_rectangle?0.:
        shape==InputFieldShape::rounded?6.:11.;
    const auto alpha=shape==InputFieldShape::inline_rectangle?.16:enabled?.64:.36;
    return {.height=22,.font_size=13,.line_height=13,
        .horizontal_padding=mode==InputFieldMode::search?21.:7.,.corner_radius=radius,
        .background=ii::gui::WidgetColor{.1555851549,.1570725590,.1585599482,alpha},
        .text=ii::gui::WidgetColor{1,1,1,enabled?.9:.3},.focus_ring=true};
}
struct InputField::Impl final {
    InputFieldOptions options;
    InputFieldEvents events;
    ii::gui::Widget root,editor,search,clear;

    ii::core::Result<void> update_clear(std::string_view text) {
        return clear.set_visible(options.clear_button_visible&&options.input.enabled
            &&!options.input.read_only&&!text.empty());
    }
    ii::core::Result<void> apply(InputFieldOptions next) {
        using namespace ii;using namespace table_detail;
        if(!metric(next.input.width,1)||!text(next.input.text)||!text(next.input.placeholder)
            ||unsigned(next.shape)>unsigned(InputFieldShape::cylinder)
            ||unsigned(next.mode)>unsigned(InputFieldMode::password))
            return core::fail(core::ErrorCode::invalid_argument,"Invalid LVRS input field options");
        auto native=next.input;
        native.password=next.mode==InputFieldMode::password;
        native.multiline=false;
        native.style=detail::input_field_style(next.shape,next.mode,native.enabled);
        try {
            good(editor.set_input_options(std::move(native)));
            good(root.set_extent({next.input.width,22}));
            good(editor.set_frame({0,0,float(next.input.width),22}));
            good(search.set_frame({7,5,12,12}));
            good(search.set_visible(next.mode==InputFieldMode::search));
            good(clear.set_frame({float(std::max(0.,next.input.width-20)),5,12,12}));
            options=std::move(next);
            return update_clear(options.input.text);
        } catch(const BuildError& error) {return std::unexpected(error.error);}
    }
};

ii::core::Result<InputField> InputField::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,
    std::string_view id,InputFieldOptions options,InputFieldEvents events) {
    using namespace ii;using namespace table_detail;
    try {
        auto s=std::make_shared<Impl>();s->events=std::move(events);
        const auto prefix=std::string(id);
        auto search_image=take(Icons::image("inputFieldSearch"));
        auto clear_image=take(Icons::image("generalcloseSmall"));
        s->root=take(b.absolute_stack(parent,id,{options.input.width,22}));
        good(s->root.set_clip_content(true));
        std::weak_ptr<Impl> weak=s;
        s->editor=take(b.input_field(s->root,prefix+".editor",{},[weak](std::string text){
            if(auto p=weak.lock()){p->options.input.text=text;good(p->update_clear(text));
                auto callback=p->events.edited;if(callback)callback(std::move(text));}
        },[weak]{if(auto p=weak.lock()){auto callback=p->events.submitted;if(callback)callback();}}));
        s->search=take(b.image(s->root,prefix+".search",{.image=std::move(search_image),.width=12,.height=12,.preserve_aspect=true}));
        s->clear=take(b.button(s->root,prefix+".clear",{.text="",.tone=gui::ButtonTone::borderless,
            .icon=std::move(clear_image),.accessibility_name="Clear input",.icon_size=12,.icon_mode=true,
            .height=12,.corner_radius=6,.horizontal_padding=0,
            .style=gui::ButtonStyle{.fill=gui::WidgetColor{1,1,1,.6}}},[weak]{
                if(auto p=weak.lock()){good(p->editor.set_text(""));p->options.input.text.clear();
                    good(p->update_clear(""));good(p->editor.focus());auto callback=p->events.edited;
                    if(callback)callback("");}
            }));
        good(s->apply(std::move(options)));good(b.retain(s));
        InputField result;result.root_=s->root;result.editor_=s->editor;result.state_=s;return result;
    } catch(const BuildError& error) {return std::unexpected(error.error);}
}
ii::core::Result<void> InputField::set_options(InputFieldOptions options)const {
    if(auto result=root_.snapshot();!result)return std::unexpected(result.error());
    auto state=state_.lock();if(!state)return ii::core::fail(ii::core::ErrorCode::closed,"Input field closed");
    return state->apply(std::move(options));
}
ii::core::Result<void> InputField::set_text(std::string_view text)const {
    if(auto result=root_.snapshot();!result)return std::unexpected(result.error());
    auto state=state_.lock();if(!state)return ii::core::fail(ii::core::ErrorCode::closed,"Input field closed");
    auto result=state->editor.set_text(text);if(!result)return result;
    auto snapshot=state->editor.snapshot();if(!snapshot)return std::unexpected(snapshot.error());
    state->options.input.text=snapshot->text;return state->update_clear(snapshot->text);
}
}
