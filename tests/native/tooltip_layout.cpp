#include <LVRS/Tooltip.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void run(const std::string& name){
    lvrs::TooltipLayoutOptions o{.anchor={320,240},.available={0,0,640,480},.content={120,40}};
    if(name=="top")o.anchor.y=5;
    if(name=="bottom")o.anchor.y=475;
    if(name=="left")o.preferred=lvrs::TooltipPlacement::left_side;
    if(name=="right")o.preferred=lvrs::TooltipPlacement::right_side;
    if(name=="fallback"){o.anchor.x=4;o.preferred=lvrs::TooltipPlacement::left_side;}
    if(name=="safe_area")o.available={20,30,400,280};
    if(name=="clamp")o.anchor.x=4;
    if(name=="bounded"){o.content={10000,100000};o.maximum_width=200;o.maximum_height=120;}
    if(name=="zero")o.available={0,0,0,0};
    if(name=="invalid"){o.anchor.x=std::numeric_limits<double>::quiet_NaN();check(!lvrs::layout_tooltip(o),"Nonfinite anchor admitted");o.anchor={0,0};o.preferred=static_cast<lvrs::TooltipPlacement>(99);check(!lvrs::layout_tooltip(o),"Invalid placement admitted");return;}
    if(name=="duration"){check(take(lvrs::tooltip_milliseconds(123.9)).count()==123&&!lvrs::tooltip_milliseconds(-1)&&!lvrs::tooltip_milliseconds(std::numeric_limits<double>::infinity())&&!lvrs::tooltip_milliseconds(86400001),"AOT duration conversion narrowed unbounded milliseconds");return;}
    const auto s=take(lvrs::layout_tooltip(o));
    if(name=="default")check(s.placement==lvrs::TooltipPlacement::above&&s.body==gui::Rect{248,166,144,64},"Authored Tooltip geometry changed");
    else if(name=="top")check(s.placement==lvrs::TooltipPlacement::below&&s.body.y==15,"Top-edge Tooltip failed to choose below");
    else if(name=="bottom")check(s.placement==lvrs::TooltipPlacement::above,"Bottom-edge Tooltip failed to choose above");
    else if(name=="left"||name=="right")check(s.placement==o.preferred,"Preferred side was ignored despite available space");
    else if(name=="fallback")check(s.placement!=o.preferred&&s.body.width==144,"Preferred side did not fall back to a full-size region");
    else if(name=="safe_area")check(s.body.x>=28&&s.body.y>=38&&s.body.x+s.body.width<=412&&s.body.y+s.body.height<=302,"Bubble body escaped its safe area");
    else if(name=="clamp")check(s.body.x==8&&s.bounds.x==4,"Body clamp lost the physical origin in the tail union");
    else if(name=="bounded")check(s.body.width==200&&s.body.height==120,"Large content exceeded maximum bubble dimensions");
    else if(name=="zero")check(std::isfinite(s.bounds.x)&&std::isfinite(s.bounds.width)&&s.body.width==0&&s.body.height==0,"Empty bounds produced invalid geometry");
    else if(name=="outline"){for(auto side:{lvrs::TooltipPlacement::above,lvrs::TooltipPlacement::below,lvrs::TooltipPlacement::left_side,lvrs::TooltipPlacement::right_side}){o.preferred=side;auto path=take(lvrs::tooltip_outline(take(lvrs::layout_tooltip(o))));check(path.starts_with("M ")&&path.ends_with('Z')&&path.find("nan")==path.npos&&path.size()<1024,"Native rounded-tail SVG path is invalid or unbounded");}}
    else if(name=="corners"){for(double x:{0.,1.,639.,640.})for(double y:{0.,1.,479.,480.}){o.anchor={x,y};auto p=take(lvrs::layout_tooltip(o));check(p.body.x>=8&&p.body.y>=8&&p.body.x+p.body.width<=632&&p.body.y+p.body.height<=472,"Corner placement escaped the inset region");check(p.bounds.x<=x&&p.bounds.y<=y&&p.bounds.x+p.bounds.width>=x&&p.bounds.y+p.bounds.height>=y,"Bubble union omitted its target anchor");}}
    else if(name=="outline_invalid"){auto bad=s;bad.placement=lvrs::TooltipPlacement::automatic;check(!lvrs::tooltip_outline(bad)&&!lvrs::tooltip_outline(s,-1),"Invalid outline parameters accepted");}
    else throw std::runtime_error("Unknown Tooltip layout case");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"default");std::cout<<"TOOLTIP_LAYOUT_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
