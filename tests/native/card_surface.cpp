#include <LVRS/CardDetail.h>
#include <stdexcept>
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
int main(){
    lvrs::card_detail::Prepared prepared;prepared.primary={.1,.5,1,1};
    for(unsigned type=0;type<8;++type)for(unsigned display=1;display<=3;++display){
        lvrs::CardOptions options;options.type=lvrs::CardType(type);options.display_state=lvrs::CardDisplayState(display);
        auto state=lvrs::card_detail::resolve(options,prepared,{},false);
        auto layout=lvrs::card_detail::layout(options,prepared,state,{256,280});
        auto background=lvrs::card_detail::background(options,prepared,state,layout,false,false,true);
        auto foreground=lvrs::card_detail::foreground(options,prepared,state,layout,true);
        if(type>=2){
            require(state.surface.alpha==0,"Information card retained an opaque outer background");
            for(const auto& panel:background->panels)require(panel.bounds.width!=256 || panel.bounds.height!=280,"Information card painted its full surface");
            require(!foreground->panels.empty() && foreground->panels.front().radius==0,"Information card border is not square");
            require(foreground->panels.front().stroke_width==(display==3?2:1),"Information card border state changed");
        }else{
            require(state.surface.alpha>0 && !background->panels.empty(),"File card background changed");
            if(type==0)require(!foreground->images.empty() && foreground->images.front().radius==12,"File card radius changed");
        }
    }
}
