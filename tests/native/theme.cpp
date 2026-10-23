#include <LVRS/Theme.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool v,const char* why) { if(!v) throw std::runtime_error(why); }
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
        bool rejected=false; try { (void)lvrs::Theme::accent_color(static_cast<lvrs::Accent>(99)); } catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"Invalid accent accepted");
        std::cout<<"LVRS_NATIVE_THEME_OK palettes=12 window_accent=2%\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
