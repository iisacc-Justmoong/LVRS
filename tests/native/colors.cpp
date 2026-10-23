#include <LVRS/ColorPicker.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>
int main(){
    try {
        static_assert(std::is_same_v<lvrs::ColorPickerModel,ii::gui::ColorPickerModel>);
        auto a=lvrs::color_picker_hue_ring(),b=lvrs::color_picker_hue_ring();
        if(!a || !b || *a!=*b || (*a)->width()!=448 || (*a)->height()!=448)throw std::runtime_error("Native wheel asset/cache differs");
        std::ifstream input(COLOR_RING_SOURCE,std::ios::binary);std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
        auto expected=ii::gui::png_image(bytes);
        if(!expected || !std::ranges::equal((*a)->rgba(),(*expected)->rgba()))throw std::runtime_error("Embedded color wheel changed original PNG pixels");
        lvrs::ColorPickerModel model;auto edited=model.edit_hex("00E71F80");auto state=model.state();
        if(!edited || !*edited || !state || state->hex!="00E71F")throw std::runtime_error("Native LVRS model alias does not use common color state");
        std::cout<<"LVRS_NATIVE_COLOR_ASSET_OK original_png=true decoded_pixels_identical=true shared_model=true\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
