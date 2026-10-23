#include <LVRS/Widgets.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void edit(gui::WidgetMount m,std::string_view id,std::string text,bool enter=true){auto w=take(m.find(id));good(w.focus());good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(m.dispatch_input({.kind=gui::WidgetInputKind::text,.text=std::move(text)}));if(enter)good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));}
void layout(){gui::Host host({.size={800,760}});lvrs::ColorPicker picker;
    good(host.run({.opened=[&]{auto m=take(host.mount_widgets([&](gui::WidgetBuilder& w)->core::Result<void>{auto root=take(w.vstack({},"root",{.spacing=0}));picker=take(lvrs::ColorPicker::create(w,root,"picker"));return {};}));
        auto model=take(picker.model());auto before=take(model->state());constexpr int heights[]{461,437,437,361,413,439};
        for(int i=0;i<6;++i){good(picker.set_type(lvrs::ColorPickerType(i)));check(take(picker.root().snapshot()).size==gui::Size{320,double(heights[i])},"Picker variant differs from original dimensions");check(take(model->state()).revision==before.revision,"Type switch edited the color model");}
        good(picker.set_type(lvrs::ColorPickerType::wheel));good(picker.show_recent_colors(false));check(take(picker.root().snapshot()).size.height==429,"Hiding recent colors did not remove 32 units");good(picker.show_actions(false));check(take(picker.root().snapshot()).size.height==395,"Hiding actions did not remove 34 units");
        good(picker.set_type(lvrs::ColorPickerType::hue_saturation));good(picker.set_width(360));auto field=take(take(m.find("picker.field")).snapshot());check(field.size==gui::Size{300,300},"Plane did not expand with width");
        check(!picker.set_width(100) && !picker.set_type(lvrs::ColorPickerType(99)),"Invalid picker geometry/type accepted");good(picker.set_enabled(false));check(!take(take(m.find("picker.hex")).snapshot()).enabled,"Disabled picker left inputs active");good(host.request_close());}}));check(!picker.model(),"Closed picker handle retained model access");}
void editing(){gui::Host host({.size={800,760}});lvrs::ColorPicker picker;int edits=0,accepted=0,canceled=0,eyedropper=0;
    good(host.run({.opened=[&]{auto m=take(host.mount_widgets([&](gui::WidgetBuilder& w)->core::Result<void>{picker=take(lvrs::ColorPicker::create(w,{},"picker",{}, {.color_edited=[&](auto){++edits;},.accepted=[&](auto){++accepted;},.canceled=[&]{++canceled;},.eyedropper_requested=[&]{++eyedropper;}}));return {};}));auto model=take(picker.model());
        edit(m,"picker.hex","00E71F80");auto green=take(model->state());check(green.hex=="00E71F" && std::abs(green.color.alpha-128./255)<1e-6,"HEX input lost alpha");
        edit(m,"picker.hex","garbage");check(take(model->state()).revision==green.revision && take(take(m.find("picker.hex")).snapshot()).text=="00E71F","Invalid HEX changed state or was not restored");
        edit(m,"picker.alpha","25",false);good(take(m.find("picker.apply")).activate());check(accepted==1 && std::abs(take(model->state()).color.alpha-.25)<1e-6,"Apply did not commit pending alpha");check(take(picker.root().snapshot()).focused&&!take(take(m.find("picker.alpha")).snapshot()).focused,"Programmatic/AX Apply left an editor focused");
        good(picker.set_type(lvrs::ColorPickerType::rgb));edit(m,"picker.channel.3.input","128",false);good(take(m.find("picker.channel.4.input")).focus());check(std::abs(take(model->state()).color.red-128./255)<1e-6,"Channel blur did not commit");
        auto original=take(model->state());good(take(m.find("picker.channel.4.input")).focus());good(take(m.find("picker.hex")).focus());check(take(model->state()).revision==original.revision,"Focus alone quantized a channel");
        good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::escape}));check(canceled==1,"Escape did not cancel");
        check(take(model->state()).color==take(model->state()).previous_color,"Cancel did not restore accepted baseline");
        good(take(m.find("picker.eyedropper")).activate());check(eyedropper==1,"Eyedropper request did not reach owner");
        good(picker.set_recent_colors({{1,0,0,.5},{0,1,0,1}}));good(take(m.find("picker.hex")).focus());good(take(m.find("picker.recent.0")).activate());check(take(model->state()).color==gui::WidgetColor{1,0,0,.5},"Recent swatch lost selected alpha");check(take(take(m.find("picker.hex")).snapshot()).text=="FF0000"&&!take(take(m.find("picker.hex")).snapshot()).focused,"AX swatch activation left stale focused HEX text");check(edits>=4,"User edits did not propagate");(void)take(host.capture_canvas());good(host.request_close());}}));}
void lifetime(){gui::Host host;std::weak_ptr<lvrs::ColorPickerModel> model;lvrs::ColorPicker picker;
    good(host.run({.opened=[&]{auto m=take(host.mount_widgets([&](gui::WidgetBuilder& w)->core::Result<void>{picker=take(lvrs::ColorPicker::create(w,{},"picker",{}, {.color_edited=[&](auto){good(host.unmount_widgets());}}));model=take(picker.model());return {};}));
        auto result=picker.set_color({1,0,0,1});good(result);check(model.expired() && !picker.root().snapshot() && !picker.model(),"Unmount from edit callback retained picker/model");good(host.request_close());}}));}
}
int main(int argc,char**argv){try{std::string name=argc>1?argv[1]:"layout";if(name=="layout")layout();else if(name=="editing")editing();else if(name=="lifetime")lifetime();else throw std::runtime_error("Unknown color picker case");std::cout<<"LVRS_NATIVE_PICKER_OK "<<name<<'\n';}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
