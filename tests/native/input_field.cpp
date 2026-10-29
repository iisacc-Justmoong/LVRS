#include <LVRS/InputField.h>
#include <LVRS/Theme.h>
#include <LVRS/Widgets.h>
#include <iostream>
#include <stdexcept>

using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> result){if(!result)throw std::runtime_error(result.error().message);return std::move(*result);}
void good(core::Result<void> result){if(!result)throw std::runtime_error(result.error().message);}
}
int main(){
    try {
        gui::Host host({.size={500,300}});gui::WidgetMount mount;lvrs::InputField field;gui::Widget factory_field;int edits=0;
        good(host.run({.opened=[&]{
            mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{
                auto root=take(b.vstack({},"root",{.padding=20}));
                field=take(lvrs::InputField::create(b,root,"input",{},
                    {.edited=[&](std::string){++edits;}}));
                lvrs::Widgets widgets(b);factory_field=take(widgets.input_field(root,"factory"));return {};
            },lvrs::Theme{}.appearance()));
            auto editor=field.editor();auto search=take(mount.find("input.search"));auto clear=take(mount.find("input.clear"));
            check(take(field.root().snapshot()).bounds.height==22&&!take(search.snapshot()).visible&&!take(clear.snapshot()).visible,"Input defaults differ");
            check(take(factory_field.snapshot()).bounds.height==22,"Widgets input factory did not use LVRS dimensions");
            lvrs::InputFieldOptions options;options.mode=lvrs::InputFieldMode::search;
            options.shape=lvrs::InputFieldShape::inline_rectangle;options.input.text="query";
            good(field.set_options(options));check(take(search.snapshot()).visible&&take(clear.snapshot()).visible&&!take(editor.snapshot()).password,"Search mode or clear affordance missing");
            options.mode=lvrs::InputFieldMode::password;options.shape=lvrs::InputFieldShape::cylinder;
            options.input.text="secret";good(field.set_options(options));
            check(!take(search.snapshot()).visible&&take(editor.snapshot()).password,"Password mode did not select secure input");
            good(editor.focus());good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::text,.text="changed"}));
            check(take(editor.snapshot()).text=="changed"&&edits==1,"Native editing callback failed");
            good(clear.activate());check(take(editor.snapshot()).text.empty()&&!take(clear.snapshot()).visible&&edits==2,"Clear did not restore an empty focused editor");
            good(field.set_text("programmatic"));check(take(clear.snapshot()).visible&&edits==2,"Programmatic update emitted an edit or left stale controls");
            options.input.enabled=false;options.input.text="locked";good(field.set_options(options));
            check(!take(clear.snapshot()).visible&&!take(editor.snapshot()).enabled,"Disabled input retained interactive clear");
            good(host.unmount_widgets());check(!field.set_text("after close"),"Retired input accepted mutation");
            good(host.request_close());
        }}));
        std::cout<<"LVRS_INPUT_FIELD_OK\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
