#include <LVRS/Sheet.h>
#include <LVRS/Theme.h>
#include <iiGUI/iiGUI.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace ii;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}}
struct Lifetime final{int& count;explicit Lifetime(int& v):count(v){}~Lifetime(){++count;}};
void run(std::string name){
    runtime::EventLoop events;runtime::FrameLoop frames(events);gui::Host host({.size={800,700},.event_loop=&events,.frame_loop=&frames});
    lvrs::Sheet sheet;gui::Widget editor,outside;gui::WidgetMount mount;int made=0,retired=0,opened=0,closed=0;
    auto prefs=std::make_shared<lvrs::Motion>();lvrs::SheetOptions options{.title="Export",.description="Retained native sheet content",.motion_enabled=false,.motion=prefs};
    if(name=="mobile"||name=="drag"||name=="motion"||name=="drag_motion"){options.presentation=lvrs::SheetPresentation::mobile;options.detent=lvrs::SheetDetent::medium;options.top_safe_inset=40;options.bottom_safe_inset=34;}
    if(name=="motion"||name=="drag_motion")options.motion_enabled=true;
    good(host.run({.opened=[&]{
        mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{
            auto root=take(b.vstack({},"root"));outside=take(b.button(root,"outside",{.text="Open sheet"}));
            sheet=take(lvrs::Sheet::create(b,root,"sheet",options,{.opened=[&]{++opened;},.closed=[&]{++closed;}},[&](gui::WidgetBuilder& builder,gui::Widget parent)->core::Result<void>{
                ++made;good(builder.retain(std::make_shared<Lifetime>(retired)));
                auto column=take(builder.vstack(parent,"body",{.spacing=8,.alignment=gui::StackAlignment::stretch}));
                editor=take(builder.input_field(column,"editor",{.text="Persistent editor"}));
                for(int i=0;i<25;++i)take(builder.label(column,"line"+std::to_string(i),{.text="Native retained scroll content "+std::to_string(i)}));return {};
            }));return {};
        },lvrs::Theme().appearance()));
        check(made==1&&retired==0&&bool(editor.snapshot())&&!take(editor.snapshot()).visible,"Sheet did not build/retain inactive content at mount");
        good(outside.focus());good(sheet.open());auto r=take(sheet.root().snapshot()).bounds;auto viewport=take(sheet.viewport().snapshot());auto now=runtime::Clock::now();
        if(name=="desktop"){
            check(r.width==560&&r.height==420&&std::abs(r.x-120)<1&&std::abs(r.y-140)<1,"Desktop Sheet geometry diverged");
            check(viewport.bounds.width==520&&viewport.content_height>viewport.bounds.height,"Sheet chrome/body viewport not separated");
            good(host.set_content_size({450,360}));r=take(sheet.root().snapshot()).bounds;check(r.width==402&&r.height==312&&r.x==24&&r.y==24,"Desktop Sheet did not clamp to margin after resize");
        }else if(name=="mobile"){
            check(r.width==800&&r.height==399&&r.x==0&&r.y==301,"Mobile medium detent not bottom aligned");
            options.detent=lvrs::SheetDetent::large;good(sheet.set_options(options));r=take(sheet.root().snapshot()).bounds;check(r.height==630&&r.y==70,"Mobile large detent did not use 90 percent");
            options.detent=lvrs::SheetDetent::fit;options.top_safe_inset=500;good(sheet.set_options(options));r=take(sheet.root().snapshot()).bounds;check(r.height==200&&r.y==500,"Fit detent ignored safe top limit");
        }else if(name=="lifetime"){
            good(editor.set_text("Kept across opens"));good(sheet.viewport().set_scroll_offset(100));good(sheet.close());check(made==1&&retired==0&&bool(editor.snapshot()),"Sheet close retired persistent content");
            for(int i=0;i<20;++i){good(sheet.open());check(take(editor.snapshot()).text=="Kept across opens"&&take(sheet.viewport().snapshot()).scroll_offset==0,"Reopen recreated body or kept scroll offset");good(sheet.close());}
            check(opened==21&&closed==21&&made==1,"Sheet repeated lifetime was not balanced");
        }else if(name=="scroll"){
            auto v=viewport.bounds;good(mount.dispatch_input({.kind=gui::WidgetInputKind::scroll,.x=v.x+20,.y=v.y+20,.scroll_y=120}));check(take(sheet.viewport().snapshot()).scroll_offset==120,"Sheet body wheel scrolled the whole popup");
            auto header=take(take(mount.find("sheet.title")).snapshot()).bounds;check(header.y<v.y,"Header scrolled with body");
            good(editor.focus());check(take(sheet.viewport().snapshot()).scroll_offset==0,"Focus did not reveal persistent first editor");
        }else if(name=="policies"){
            options.dismiss_on_background=false;options.dismiss_on_escape=false;good(sheet.set_options(options));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=2,.y=2}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=2,.y=2}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::escape}));check(take(sheet.root().snapshot()).opened,"Sheet ignored dismissal policy");
            good(take(mount.find("sheet.close")).activate());check(!take(sheet.root().snapshot()).opened&&take(outside.snapshot()).focused,"Close button failed to close/restore focus");
        }else if(name=="drag"){
            auto grabber=take(take(mount.find("sheet.grabber")).snapshot()).bounds;const auto x=grabber.x+grabber.width/2,y=grabber.y+grabber.height/2;
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=x,.y=y}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=x,.y=y+30}));check(take(sheet.root().snapshot()).visual_bounds.y==r.y+30,"Grabber drag did not translate the surface");good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=x,.y=y+30}));check(take(sheet.root().snapshot()).opened&&take(sheet.root().snapshot()).visual_bounds.y==r.y,"Short drag did not snap back");
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=x,.y=y}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=x,.y=y+130}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=x,.y=y+130}));check(!take(sheet.root().snapshot()).opened&&closed==1,"Long grabber drag did not close");
        }else if(name=="drag_motion"){
            (void)frames.step(now+500ms);auto grabber=take(take(mount.find("sheet.grabber")).snapshot()).bounds;const auto x=grabber.x+grabber.width/2,y=grabber.y+grabber.height/2;
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=x,.y=y}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=x,.y=y+40}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_leave}));check(take(sheet.root().snapshot()).visual_bounds.y==r.y+40,"Captured grabber was canceled at window edge");good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=x,.y=y+40}));check(take(sheet.root().snapshot()).transitioning,"Short animated drag did not reserve a frame track");
            (void)frames.step(now+700ms);check(take(sheet.root().snapshot()).visual_bounds.y<r.y,"Snap-back lost original Back overshoot");good(prefs->set_reduced_motion(true));check(!take(sheet.root().snapshot()).transitioning&&take(sheet.root().snapshot()).visual_bounds.y==r.y,"Live reduced motion did not finish snap-back");
        }else if(name=="motion"){
            auto initial=take(sheet.root().snapshot());check(initial.transitioning&&initial.visual_bounds.y>=690,"Mobile enter did not begin below viewport");
            (void)frames.step(now+210ms);check(take(sheet.root().snapshot()).visual_bounds.y<r.y,"Mobile Back easing lost overshoot");
            good(prefs->set_reduced_motion(true));check(!take(sheet.root().snapshot()).transitioning&&take(sheet.root().snapshot()).visual_bounds.y==r.y,"Reduced motion did not settle Sheet");
            good(prefs->set_reduced_motion(false));frames.set_paused(true);good(frames.set_time_scale(0));good(sheet.close());check(bool(editor.snapshot())&&retired==0&&take(sheet.root().snapshot()).exiting,"Motion close retired Sheet body or skipped exit");(void)frames.step(now+800ms);check(!take(sheet.root().snapshot()).visible,"Game pause froze unscaled Sheet exit");
        }else if(name=="rendering"){
            auto first=take(host.capture_canvas());good(sheet.viewport().set_scroll_offset(80));check(take(host.capture_canvas()).rgba!=first.rgba,"Sheet content did not render native scroll pixels");
        }else throw std::runtime_error("Unknown Sheet case");
        good(host.unmount_widgets());check(retired==1&&!editor.snapshot()&&!sheet.root().snapshot(),"Sheet retained mount-owned work after teardown");good(host.request_close());
    }}));
}
}
int main(int argc,char**argv){try{run(argc>1?argv[1]:"desktop");std::cout<<"NATIVE_SHEET_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
