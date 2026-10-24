#include "TooltipLayout.h"
#include <iiSDKCore/Core/Text/Number.h>
#include <algorithm>
#include <array>
#include <cmath>
namespace lvrs {
namespace {
bool metric(double n,double limit=8192){return std::isfinite(n)&&n>=0&&n<=limit;}
double clamp(double n,double low,double high){return std::max(low,std::min(high,n));}
bool rect(ii::gui::Rect r){return std::isfinite(r.x)&&std::isfinite(r.y)&&std::abs(r.x)<=32767&&std::abs(r.y)<=32767&&metric(r.width,32767)&&metric(r.height,32767);}
}
ii::core::Result<TooltipLayout> layout_tooltip(TooltipLayoutOptions o){
    using namespace ii;
    if(!rect(o.available)||!std::isfinite(o.anchor.x)||!std::isfinite(o.anchor.y)||std::abs(o.anchor.x)>32767||std::abs(o.anchor.y)>32767||!metric(o.content.width,1e15)||!metric(o.content.height,1e15)||unsigned(o.preferred)>4||!metric(o.maximum_width)||!metric(o.maximum_height)||!metric(o.content_padding)||!metric(o.edge_margin)||!metric(o.tail_length))return core::fail(core::ErrorCode::invalid_argument,"Invalid Tooltip layout input");
    const double left=o.available.x+o.edge_margin,top=o.available.y+o.edge_margin;
    const double right=std::max(left,o.available.x+o.available.width-o.edge_margin),bottom=std::max(top,o.available.y+o.available.height-o.edge_margin);
    const auto desired_w=std::min(o.maximum_width,std::max(1.,o.content.width+o.content_padding*2)),desired_h=std::min(o.maximum_height,std::max(1.,o.content.height+o.content_padding*2));
    std::array order{TooltipPlacement::above,TooltipPlacement::below,TooltipPlacement::right_side,TooltipPlacement::left_side};
    if(o.preferred!=TooltipPlacement::automatic){auto found=std::find(order.begin(),order.end(),o.preferred);std::rotate(order.begin(),found,found+1);}
    TooltipLayout best;best.anchor=o.anchor;double best_area=-1;
    for(auto side:order){
        auto l=left,t=top,r=right,b=bottom;
        if(side==TooltipPlacement::above)b=std::min(b,o.anchor.y-o.tail_length);
        if(side==TooltipPlacement::below)t=std::max(t,o.anchor.y+o.tail_length);
        if(side==TooltipPlacement::left_side)r=std::min(r,o.anchor.x-o.tail_length);
        if(side==TooltipPlacement::right_side)l=std::max(l,o.anchor.x+o.tail_length);
        const auto w=std::max(0.,std::min(desired_w,r-l)),h=std::max(0.,std::min(desired_h,b-t));
        const auto x=side==TooltipPlacement::left_side?r-w:side==TooltipPlacement::right_side?l:clamp(o.anchor.x-w/2,l,r-w);
        const auto y=side==TooltipPlacement::above?b-h:side==TooltipPlacement::below?t:clamp(o.anchor.y-h/2,t,b-h);
        if(w*h>best_area||(w>=desired_w&&h>=desired_h)){best.placement=side;best.body={float(x),float(y),float(w),float(h)};best_area=w*h;}
        if(w>=desired_w&&h>=desired_h)break;
    }
    const auto b=best.body;const auto x=std::min(double(b.x),o.anchor.x),y=std::min(double(b.y),o.anchor.y);
    best.bounds={float(x),float(y),float(std::max(double(b.x+b.width),o.anchor.x)-x),float(std::max(double(b.y+b.height),o.anchor.y)-y)};
    return best;
}
ii::core::Result<std::string> tooltip_outline(const TooltipLayout& o,double corner,double tail){
    using namespace ii;
    if(!rect(o.body)||!rect(o.bounds)||unsigned(o.placement)<1||unsigned(o.placement)>4||!std::isfinite(o.anchor.x)||!std::isfinite(o.anchor.y)||std::abs(o.anchor.x)>32767||std::abs(o.anchor.y)>32767||!metric(corner)||!metric(tail))return core::fail(core::ErrorCode::invalid_argument,"Invalid Tooltip outline input");
    const double l=o.body.x-o.bounds.x,t=o.body.y-o.bounds.y,r=l+o.body.width,d=t+o.body.height,px=o.anchor.x-o.bounds.x,py=o.anchor.y-o.bounds.y;
    const auto radius=std::min({corner,double(o.body.width)/2,double(o.body.height)/2});
    const auto hx=std::min(tail/2,std::max(0.,double(o.body.width)/2-radius)),hy=std::min(tail/2,std::max(0.,double(o.body.height)/2-radius));
    const auto cx=clamp(px,l+radius+hx,r-radius-hx),cy=clamp(py,t+radius+hy,d-radius-hy);
    try{
        std::string path;path.reserve(640);
        const auto point=[&](std::string_view command,double x,double y){path+=command;for(auto v:{x,y}){path+=core::format_decimal(v);path+=' ';}};
        point("M ",l+radius,t);
        if(o.placement==TooltipPlacement::below){point("L ",cx-hx,t);point("L ",px,py);point("L ",cx+hx,t);}
        point("L ",r-radius,t);point("Q ",r,t);point("",r,t+radius);
        if(o.placement==TooltipPlacement::left_side){point("L ",r,cy-hy);point("L ",px,py);point("L ",r,cy+hy);}
        point("L ",r,d-radius);point("Q ",r,d);point("",r-radius,d);
        if(o.placement==TooltipPlacement::above){point("L ",cx+hx,d);point("L ",px,py);point("L ",cx-hx,d);}
        point("L ",l+radius,d);point("Q ",l,d);point("",l,d-radius);
        if(o.placement==TooltipPlacement::right_side){point("L ",l,cy+hy);point("L ",px,py);point("L ",l,cy-hy);}
        point("L ",l,t+radius);point("Q ",l,t);point("",l+radius,t);path+='Z';return path;
    }catch(const std::bad_alloc&){return core::fail(core::ErrorCode::full,"Tooltip outline allocation failed");}
}
} // namespace lvrs
