#include <LVRS/Theme.h>
#include "../../src/Styling/MaterialDetail.h"
#include <iiJson/Json.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

void check(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
const ii::json::Value& field(const ii::json::Value& value, std::string_view name) {
    auto result=value.find(name); if(!result) throw std::runtime_error("Missing Figma fixture field"); return result->get();
}
double number(const ii::json::Value& value) {
    const auto& s=value.storage(); if(auto v=std::get_if<double>(&s)) return *v;
    if(auto v=std::get_if<std::int64_t>(&s)) return double(*v);
    if(auto v=std::get_if<std::uint64_t>(&s)) return double(*v);
    throw std::runtime_error("Invalid Figma fixture number");
}
bool matches(ii::gui::WidgetColor actual,const ii::json::Value& expected) {
    return std::abs(actual.red-number(field(expected,"r")))<1e-7
        && std::abs(actual.green-number(field(expected,"g")))<1e-7
        && std::abs(actual.blue-number(field(expected,"b")))<1e-7
        && std::abs(actual.alpha-number(field(expected,"a")))<1e-7;
}
int main() {
    try {
        const auto base=lvrs::Theme::background(lvrs::BackgroundRole::window);
        for(unsigned i=0;i<12;++i) {
            lvrs::Theme t(static_cast<lvrs::Accent>(i)); auto a=t.appearance(); auto c=lvrs::Theme::accent_color(t.accent());
            check(a.styled && a.primary==c && a.font_family=="Pretendard","Native theme lost authored values");
            check(a.hover==lvrs::Theme::accent_role(t.accent(),lvrs::AccentRole::hover)
                && a.pressed==lvrs::Theme::accent_role(t.accent(),lvrs::AccentRole::pressed),"Accent state colors incorrect");
            auto channel=[](double background,double accent) {
                return std::round((background*.98+accent*.02)*255)/255;
            };
            check(a.window.red==channel(base.red,c.red) && a.window.green==channel(base.green,c.green)
                && a.window.blue==channel(base.blue,c.blue),"2% native window accent incorrect");
        }
        std::ifstream file(LVRS_ACCENT_FIXTURE_PATH); check(bool(file),"Missing Figma accent role fixture");
        const std::string text((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
        auto document=ii::json::parse(text); check(bool(document),"Invalid Figma accent role fixture");
        auto array=field(*document,"variables").get<ii::json::Value::Array>(); check(bool(array)&&array->get().size()==90,"Incomplete Figma accent role fixture");
        constexpr std::array colors{std::pair{"Blue",lvrs::Accent::blue},std::pair{"Red",lvrs::Accent::red},
            std::pair{"Green",lvrs::Accent::green},std::pair{"Yellow",lvrs::Accent::yellow},std::pair{"Purple",lvrs::Accent::purple},
            std::pair{"Rose",lvrs::Accent::rose},std::pair{"Orange",lvrs::Accent::orange},std::pair{"Brown",lvrs::Accent::brown},
            std::pair{"Cyan",lvrs::Accent::cyan},std::pair{"Teal",lvrs::Accent::teal}};
        constexpr std::array roles{std::pair{"Default",lvrs::AccentRole::standard},std::pair{"Hover",lvrs::AccentRole::hover},
            std::pair{"Pressed",lvrs::AccentRole::pressed},std::pair{"Selected",lvrs::AccentRole::selected},
            std::pair{"Disabled",lvrs::AccentRole::disabled},std::pair{"Subtle",lvrs::AccentRole::subtle},
            std::pair{"Outline",lvrs::AccentRole::outline},std::pair{"Overlay",lvrs::AccentRole::overlay},
            std::pair{"Text",lvrs::AccentRole::text}};
        for(const auto& token:array->get()) {
            auto name=field(token,"name").get<std::string>(); check(bool(name),"Invalid Figma token name");
            const auto& path=name->get(); auto slash=path.find('/',7);
            check(path.starts_with("Accent/") && slash!=std::string::npos,"Invalid Figma token path");
            auto color=std::ranges::find_if(colors,[&](auto item){return path.substr(7,slash-7)==item.first;});
            auto role=std::ranges::find_if(roles,[&](auto item){return path.substr(slash+1)==item.first;});
            check(color!=colors.end() && role!=roles.end(),"Unknown Figma accent role");
            check(matches(lvrs::Theme::accent_role(color->second,role->second),field(token,"value")),"Native accent role differs from Figma");
        }
        auto icons=field(*document,"iconVariables").get<ii::json::Value::Array>();check(bool(icons)&&icons->get().size()==29,"Incomplete icon token fixture");
        for(const auto& token:icons->get()) {
            auto name=field(token,"name").get<std::string>();check(bool(name),"Invalid icon token name");const auto& path=name->get();
            constexpr std::string_view prefix="Icon/Accent/";auto a=path.find('/',prefix.size());auto b=path.find('/',a+1);
            check(path.starts_with(prefix)&&a!=std::string::npos&&b!=std::string::npos&&path.substr(b+1)=="Darker","Invalid icon token path");
            auto color=std::ranges::find_if(colors,[&](auto item){return path.substr(prefix.size(),a-prefix.size())==item.first;});
            auto role=std::ranges::find_if(roles,[&](auto item){return path.substr(a+1,b-a-1)==item.first;});
            check(color!=colors.end()&&role!=roles.end(),"Unknown icon token");
            check(matches(lvrs::Theme::icon_accent_role(color->second,role->second),field(token,"value")),"Native icon token differs from Figma");
        }
        for(auto [name,color]:colors) {
            check(lvrs::Theme::icon_accent_role(color,lvrs::AccentRole::standard)==lvrs::Theme::accent_role(color,lvrs::AccentRole::hover),"Default icon must use Hover");
            check(lvrs::Theme::icon_accent_role(color,lvrs::AccentRole::hover)==lvrs::Theme::accent_role(color,lvrs::AccentRole::pressed),"Hover icon must use Pressed");
        }
        constexpr std::array backgrounds{
            std::pair{"Panel/canvas",lvrs::BackgroundRole::canvas},std::pair{"Panel/surfaceGhost",lvrs::BackgroundRole::surface_ghost},
            std::pair{"Panel/windowAlt",lvrs::BackgroundRole::window_alt},std::pair{"Panel/subSurface",lvrs::BackgroundRole::sub_surface},
            std::pair{"Panel/surfaceSolid",lvrs::BackgroundRole::surface_solid},std::pair{"Panel/surfaceAlt",lvrs::BackgroundRole::surface_alt},
            std::pair{"Panel/surfaceInteractive",lvrs::BackgroundRole::surface_interactive},std::pair{"Panel/surfacePanel",lvrs::BackgroundRole::surface_panel},
            std::pair{"Panel/surfaceControl",lvrs::BackgroundRole::surface_control},std::pair{"Panel/surfaceInput",lvrs::BackgroundRole::surface_input},
            std::pair{"Panel/surfaceInputHover",lvrs::BackgroundRole::surface_input_hover},std::pair{"Panel/surfaceRaised",lvrs::BackgroundRole::surface_raised},
            std::pair{"Glass/Fill",lvrs::BackgroundRole::glass_fill},std::pair{"TextField/Fill",lvrs::BackgroundRole::text_field_fill},
            std::pair{"TextField/Disabled",lvrs::BackgroundRole::text_field_disabled},std::pair{"TextField/Inline",lvrs::BackgroundRole::text_field_inline},
            std::pair{"Surface/Dense/Fill",lvrs::BackgroundRole::dense_fill},std::pair{"Surface/Glass/Fill",lvrs::BackgroundRole::surface_glass_fill},
            std::pair{"Backdrop/Base",lvrs::BackgroundRole::backdrop_base},std::pair{"Panel/window",lvrs::BackgroundRole::window},
            std::pair{"Alert/canvas",lvrs::BackgroundRole::alert_canvas},std::pair{"Alert/surface",lvrs::BackgroundRole::alert_surface}};
        auto bg=field(*document,"backgroundVariables").get<ii::json::Value::Array>();check(bool(bg)&&bg->get().size()==backgrounds.size(),"Incomplete Figma background fixture");
        for(const auto& token:bg->get()) {
            auto name=field(token,"name").get<std::string>();check(bool(name),"Invalid background name");
            auto role=std::ranges::find_if(backgrounds,[&](auto item){return name->get()==item.first;});
            check(role!=backgrounds.end()&&matches(lvrs::Theme::background(role->second),field(token,"value")),"Native background differs from Figma");
        }
        auto material=lvrs::material_detail::prepare({},lvrs::MaterialVariant::window,lvrs::Theme{}.appearance());
        check(bool(material),"Window material preparation failed");
        auto dense=lvrs::Theme::background(lvrs::BackgroundRole::dense_fill);
        check(std::abs(material->target.tint.red-dense.red)<1e-7
            && std::abs(material->target.tint.green-dense.green)<1e-7
            && std::abs(material->target.tint.blue-dense.blue)<1e-7
            && std::abs(material->target.tint.alpha-.5)<1e-7,"Window material does not use Figma dense fill");
        bool rejected=false;try{(void)lvrs::Theme::accent_color(static_cast<lvrs::Accent>(99));}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid accent accepted");
        rejected=false;try{(void)lvrs::Theme::accent_role(lvrs::Accent::blue,static_cast<lvrs::AccentRole>(99));}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid role accepted");
        rejected=false;try{(void)lvrs::Theme::background(static_cast<lvrs::BackgroundRole>(99));}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid background accepted");
        std::cout<<"LVRS_NATIVE_THEME_OK accents=90 icons=29 backgrounds=22\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
