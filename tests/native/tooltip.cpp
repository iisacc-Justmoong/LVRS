#include <LVRS/Tooltip.h>
#include <LVRS/Theme.h>
#include <iiGUI/Bindings.h>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace ii;
using namespace std::chrono_literals;
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void run(const std::string& name){
    runtime::EventLoop loop;runtime::FrameLoop frames(loop);gui::Host host({.size={640,480},.event_loop=&loop,.frame_loop=&frames});
    lvrs::Tooltip tooltip;gui::Widget target,second,label;gui::WidgetMount mount;unsigned made=0,clicks=0,opened=0,closed=0;
    lvrs::TooltipOptions o{.text="Native Tooltip",.automatic=false,.motion_enabled=false,.delay=0ms,.timeout=0ms,.hide_delay=0ms};
    if(name=="defaults")o={};
    if(name=="automatic"||name=="hide"||name=="focus"||name=="latch")o.automatic=true;
    if(name=="delay")o.delay=1s;
    if(name=="timeout")o.timeout=1s;
    if(name=="scroll")o.maximum_height=100;
    good(host.run({.opened=[&]{
        mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{
            auto root=take(b.absolute_stack({},"root",{640,480}));target=take(b.button(root,"origin",{.text="Origin",.height=40,.width=160},[&]{++clicks;}));good(target.set_frame({240,220,160,40}));
            second=take(b.button(root,"second",{.text="Next",.height=40,.width=100}));good(second.set_frame({20,30,100,40}));o.target=target;
            lvrs::TooltipContentFactory content;if(name=="content"||name=="scroll")content=[&](gui::WidgetBuilder& c,gui::Widget p)->core::Result<void>{++made;for(unsigned i=0;i<(name=="scroll"?40u:1u);++i){auto l=take(c.label(p,"content"+std::to_string(i),{.text="Retained native content",.font_size=13,.line_height=16}));if(!i)label=l;}return {};};
            tooltip=take(lvrs::Tooltip::create(b,root,"tooltip",o,{.opened=[&]{++opened;},.closed=[&]{++closed;}},content));return {};
        },lvrs::Theme().appearance()));
        take(target.snapshot());
        if(name=="defaults"){auto options=take(tooltip.options());check(options.delay==500ms&&options.timeout==5000ms&&options.hide_delay==120ms&&options.corner_radius==12&&options.maximum_width==320,"Authored Tooltip defaults changed");}
        else if(name=="automatic"||name=="hide"||name=="latch"){
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=320,.y=240}));check(take(tooltip.state()).opened,"Origin hover did not open the native Tooltip");
            if(name=="hide"){good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=600,.y=460}));check(!take(tooltip.state()).opened,"Leaving origin and body did not honor hideDelay=0");}
            if(name=="latch"){good(tooltip.close());auto options=take(tooltip.options());options.text="Updated while dismissed";options.available_rect=gui::Rect{8,8,624,464};good(tooltip.set_options(options));take(target.snapshot());check(!take(tooltip.state()).opened&&opened==1,"Dismissal or a style/area binding immediately reopened over the unchanged origin");}
        }else if(name=="focus"){good(target.focus());take(target.snapshot());check(take(tooltip.state()).opened&&take(target.snapshot()).focused,"Focus-triggered Tooltip stole focus from its origin");}
        else if(name=="delay"){const auto before=loop.stats().pending;good(tooltip.open());check(take(tooltip.state()).pending&&!take(tooltip.state()).opened&&loop.stats().pending==before+1,"Owned show deadline was not reserved");good(tooltip.close());check(!take(tooltip.state()).pending&&loop.stats().pending==before,"Cancel retained a pending show task");}
        else{
            good(tooltip.open());auto state=take(tooltip.state());check(state.opened&&state.layout.body.width>24,"Explicit Tooltip did not open with a body region");
            if(name=="plain"){check(take(tooltip.root().snapshot()).native_class=="iiGUI.Popover"&&take(tooltip.content_item().snapshot()).native_class=="iiGUI.VStack","Tooltip substituted OS controls");take(host.capture_canvas());}
            else if(name=="placement"){auto options=take(tooltip.options());options.preferred_placement=lvrs::TooltipPlacement::right_side;good(tooltip.set_options(options));check(take(tooltip.state()).layout.placement==lvrs::TooltipPlacement::right_side,"Same-instance placement replacement was ignored");}
            else if(name=="content"){good(tooltip.close());good(tooltip.open());check(made==1&&take(label.snapshot()).visible,"Closing reconstructed or retired retained Tooltip content");}
            else if(name=="scroll"){auto v=take(tooltip.viewport().snapshot());check(v.content_height>v.bounds.height,"Tooltip content did not expose a real scroll viewport");good(tooltip.viewport().set_scroll_offset(120));check(take(tooltip.viewport().snapshot()).scroll_offset>0,"Tooltip scrolling did not reach retained content");good(tooltip.close());good(tooltip.open());check(take(tooltip.viewport().snapshot()).scroll_offset==0,"Reopen did not reset the viewport");}
            else if(name=="update"){auto options=take(tooltip.options());options.text="Changed native help";good(tooltip.set_options(options));check(take(take(mount.find("tooltip.label")).snapshot()).text==options.text&&opened==1,"Text update rebuilt or reopened its view");}
            else if(name=="retarget"){good(tooltip.set_target(second));check(!take(tooltip.state()).opened,"Retarget did not dismiss the previous bubble");good(tooltip.open());auto s=take(tooltip.state());auto b=take(second.interaction()).bounds;check(s.layout.anchor==lvrs::TooltipPoint{b.x+b.width/2,b.y+b.height/2},"Retarget retained a stale origin");}
            else if(name=="hidden"){good(target.set_visible(false));take(target.snapshot());check(!take(tooltip.state()).opened,"A hidden target retained its bubble");}
            else if(name=="timeout"){check(loop.stats().pending>=1,"Opened Tooltip has no Core-owned expiry");auto deadline=loop.next_deadline();auto options=take(tooltip.options());options.text="A text update must keep the original timeout";good(tooltip.set_options(options));check(loop.next_deadline()==deadline,"A style/text binding extended the Tooltip expiry");good(tooltip.close());check(loop.stats().pending==0,"Closing retained its timeout task");}
            else if(name=="escape"){good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::escape}));check(!take(tooltip.state()).opened&&closed==1,"Escape did not dismiss a focus-preserving bubble");}
            else if(name=="click"){good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=320,.y=240}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=320,.y=240}));check(clicks==1,"Tooltip tail/origin dismissal consumed a button click");}
            else if(name=="invalid"){auto options=take(tooltip.options());options.maximum_width=0;check(!tooltip.set_options(options)&&take(tooltip.state()).opened,"Rejected options damaged an open Tooltip");}
            else if(name=="owner"){bool rejected=false;std::thread t([&]{auto r=tooltip.state();rejected=!r&&r.error().code==core::ErrorCode::wrong_executor;});t.join();check(rejected,"Tooltip crossed its UI owner thread");}
            else if(name=="retirement"){auto old=tooltip;good(host.unmount_widgets());check(!old.state()&&loop.stats().pending==0,"Retired Tooltip addressed a successor or retained owned timers");}
            else throw std::runtime_error("Unknown Tooltip case");
        }
        good(host.request_close());
    }}));
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"plain");std::cout<<"TOOLTIP_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
