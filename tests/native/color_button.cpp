#include <LVRS/Widgets.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace ii;
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(){try{
    auto ring=take(lvrs::color_picker_button_hue_ring());std::ifstream file(COLOR_BUTTON_RING_SOURCE,std::ios::binary);std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});auto expected=take(gui::png_image(bytes));check(ring->width()==320&&std::ranges::equal(ring->rgba(),expected->rgba())&&ring==take(lvrs::color_picker_button_hue_ring()),"Original button ring/cache differs");check(ring!=take(lvrs::color_picker_hue_ring()),"Button reused large wheel asset");
    gui::Host host;good(host.run({.opened=[&]{auto model=std::make_shared<lvrs::ColorPickerModel>();auto m=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{lvrs::Widgets w(b);auto root=take(w.hstack({},"root"));take(w.color_picker_button(root,"default"));take(w.color_picker_button(root,"circle",{.model=model,.show_hue_ring=false,.circular=true}));take(w.color_picker_button(root,"red",{.current_color=gui::WidgetColor{1,0,0,.5},.button_size=lvrs::ColorPickerButtonSize::small,.show_hue_ring=false}));take(w.color_picker_button(root,"shared",{.model=model,.button_size=lvrs::ColorPickerButtonSize::large}));check(!w.color_picker_button(root,"conflict",{.model=model,.current_color=gui::WidgetColor{1,0,0,1}}),"Conflicting model/color accepted");check(!w.color_picker_button(root,"size",{.button_size=lvrs::ColorPickerButtonSize(99)}),"Invalid size enum accepted");return {};}));
        auto s=take(take(m.find("default")).snapshot());check(s.size==gui::Size{28,28}&&s.color_state->hex=="874AFF"&&s.accessibility_name=="Choose color","Original defaults differ");s=take(take(m.find("red")).snapshot());check(s.size==gui::Size{22,22}&&s.color_state->hex=="FF0000"&&s.color_state->color.alpha==.5&&!s.show_hue_ring,"Explicit initial color/size/ring differs");good(model->set_color({0,1,0,1}));check(take(take(m.find("shared")).snapshot()).color_state->hex=="00FF00","Shared model not reflected");good(host.request_close());}}));
    std::cout<<"LVRS_NATIVE_COLOR_BUTTON_OK original_png=true sizes=22,28,36 default=874AFF shared_model=true\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
