#include <LVRS/Theme.h>
#include <iiJson/Json.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
void check(bool v,const char* why) { if(!v) throw std::runtime_error(why); }
const ii::json::Value& field(const ii::json::Value& value,std::string_view name) {
    auto result=value.find(name);if(!result)throw std::runtime_error("Missing Figma fixture field");return result->get();
}
double number(const ii::json::Value& value) {
    const auto& s=value.storage();if(auto v=std::get_if<double>(&s))return *v;
    if(auto v=std::get_if<std::int64_t>(&s))return double(*v);
    if(auto v=std::get_if<std::uint64_t>(&s))return double(*v);
    throw std::runtime_error("Invalid Figma fixture number");
}
int main() {
    try {
        for(unsigned i=0;i<12;++i) {
            lvrs::Theme t(static_cast<lvrs::Accent>(i)); auto a=t.appearance(); auto c=lvrs::Theme::accent_color(t.accent());
            check(a.styled && a.primary==c && a.font_family=="Pretendard","Native theme lost authored values");
            check(std::abs(a.hover.green-c.green/1.12)<1e-12 && std::abs(a.pressed.blue-c.blue/1.2)<1e-12,"Accent state colors incorrect");
            auto channel=[](double c) { return std::round(20*.98+c*255*.02)/255; };
            check(a.window.red==channel(c.red) && a.window.green==channel(c.green) && a.window.blue==channel(c.blue),"2% native window accent incorrect");
        }
        auto green=lvrs::Theme::accent_color(lvrs::Accent::green); check(green.red==0 && std::abs(green.green-231.0/255)<1e-7 && std::abs(green.blue-31.0/255)<1e-7,"LVRS green must be #00E71F");
        std::ifstream file(LVRS_ACCENT_FIXTURE_PATH);check(bool(file),"Missing Figma accent role fixture");
        const std::string text((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
        auto document=ii::json::parse(text);check(bool(document),"Invalid Figma accent role fixture");
        auto array=field(*document,"variables").get<ii::json::Value::Array>();check(bool(array)&&array->get().size()==90,"Incomplete Figma accent role fixture");
        constexpr std::array colors{std::pair{"Blue",lvrs::Accent::blue},std::pair{"Red",lvrs::Accent::red},
            std::pair{"Green",lvrs::Accent::green},std::pair{"Yellow",lvrs::Accent::yellow},std::pair{"Purple",lvrs::Accent::purple},
            std::pair{"Rose",lvrs::Accent::rose},std::pair{"Orange",lvrs::Accent::orange},std::pair{"Brown",lvrs::Accent::brown},
            std::pair{"Cyan",lvrs::Accent::cyan},std::pair{"Teal",lvrs::Accent::teal}};
        constexpr std::array roles{std::pair{"Default",lvrs::AccentRole::standard},std::pair{"Hover",lvrs::AccentRole::hover},
            std::pair{"Pressed",lvrs::AccentRole::pressed},std::pair{"Selected",lvrs::AccentRole::selected},
            std::pair{"Disabled",lvrs::AccentRole::disabled},std::pair{"Subtle",lvrs::AccentRole::subtle},
            std::pair{"Outline",lvrs::AccentRole::outline},std::pair{"Overlay",lvrs::AccentRole::overlay},std::pair{"Text",lvrs::AccentRole::text}};
        for(const auto& token:array->get()) {
            auto name=field(token,"name").get<std::string>();check(bool(name),"Invalid Figma token name");
            const auto& path=name->get();auto slash=path.find('/',7);check(path.starts_with("Accent/")&&slash!=std::string::npos,"Invalid Figma token path");
            auto color=std::ranges::find_if(colors,[&](auto item){return path.substr(7,slash-7)==item.first;});
            auto role=std::ranges::find_if(roles,[&](auto item){return path.substr(slash+1)==item.first;});
            check(color!=colors.end()&&role!=roles.end(),"Unknown Figma accent role");
            const auto actual=lvrs::Theme::accent_role(color->second,role->second);const auto& expected=field(token,"value");
            check(std::abs(actual.red-number(field(expected,"r")))<1e-7 && std::abs(actual.green-number(field(expected,"g")))<1e-7
                && std::abs(actual.blue-number(field(expected,"b")))<1e-7 && std::abs(actual.alpha-number(field(expected,"a")))<1e-7,
                "Native accent role differs from Figma");
        }
        bool rejected=false; try { (void)lvrs::Theme::accent_color(static_cast<lvrs::Accent>(99)); } catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"Invalid accent accepted");
        rejected=false;try{(void)lvrs::Theme::accent_role(lvrs::Accent::blue,static_cast<lvrs::AccentRole>(99));}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Invalid accent role accepted");
        std::cout<<"LVRS_NATIVE_THEME_OK palettes=12 roles=90 window_accent=2%\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
