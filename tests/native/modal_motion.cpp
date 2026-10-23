#include <LVRS/Modal.h>
#include <LVRS/Theme.h>
#include <iiGUI/iiGUI.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace ii;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>void good(const T& r){if(!r){if constexpr(requires{r.error().cause;}){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}else throw std::runtime_error("Task admission rejected");}}
template<class T>T take(core::Result<T> r){good(r);return std::move(*r);}
struct Lifetime final {int& count;explicit Lifetime(int& n):count(n){}~Lifetime(){++count;}};
void run(const std::string& name){
    runtime::EventLoop events;runtime::FrameLoop frames(events);gui::Host host({.size={1000,800},.event_loop=&events,.frame_loop=&frames});
    lvrs::Modal modal;gui::WidgetMount mount;auto prefs=std::make_shared<lvrs::Motion>();int made=0,retired=0,closed=0,clicked=0;
    good(host.run({.opened=[&]{
        mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));
            lvrs::ModalOptions o{.title="Static title",.description="Persistent header",.button_count=3,.primary_text="Apply",.secondary_text="Cancel",.tertiary_text="More",.motion=prefs};
            if(name=="transparent")o.frame_color={1,0,0,.5};
            if(name=="body_only"){o.title.clear();o.description.clear();o.show_icon=false;o.button_count=0;o.primary_text.clear();o.secondary_text.clear();o.tertiary_text.clear();o.motion_enabled=false;}
            if(name=="failure")o.motion_enabled=false;
            modal=take(lvrs::Modal::create(b,root,"modal",o,{.primary_clicked=[&]{++clicked;},.closed=[&]{check(retired==made,"Closed before body lifetime retired");++closed;if(name=="failure"&&closed==1)good(modal.open());}},[&](gui::WidgetBuilder& child,gui::Widget parent)->core::Result<void>{
                ++made;good(child.retain(std::make_shared<Lifetime>(retired)));
                if(name=="failure"&&made==1)return core::fail(core::ErrorCode::failed,"Expected first body admission failure");
                if(name=="body_only"){take(child.image(parent,"bodyImage",{.image=take(gui::rgba_image(1,1,{0,255,0,255})),.width=380,.height=300}));return {};}
                take(child.input_field(parent,"editor",{.text="Fresh",.width=380}));take(child.image(parent,"bodyImage",{.image=take(gui::rgba_image(1,1,{0,255,0,255})),.width=380,.height=180}));return {};
            }));return {};
        },lvrs::Theme().appearance()));
        if(name=="body_only"){good(modal.open());check(take(modal.root().snapshot()).bounds.height==340,"Zero-height header or hidden actions left a Column gap");good(host.request_close());return;}
        auto title=take(mount.find("modal.title")),action=take(mount.find("modal.primary"));const auto title_id=take(title.snapshot()).id;auto now=runtime::Clock::now();
        if(name=="failure"){auto failed=modal.open();check(!failed&&retired==1&&!take(modal.root().snapshot()).opened&&bool(title.snapshot()),"Body failure kept partial content or retired static chrome");}
        good(modal.open());auto editor=take(modal.root().popup_content("editor"));const auto first=take(editor.snapshot()).id;
        if(name=="chrome"){
            good(prefs->set_reduced_motion(true));good(modal.close());check(retired==1&&bool(title.snapshot())&&!take(title.snapshot()).visible&&!editor.snapshot(),"Close destroyed chrome or retained body");
            for(int i=0;i<20;++i){good(modal.open());check(take(take(mount.find("modal.title")).snapshot()).id==title_id&&take(modal.root().popup_content("editor")).snapshot()->id!=first,"Reopen recreated static chrome or reused body");good(modal.close());}
            check(made==21&&retired==21&&closed==21&&bool(action.snapshot()),"Repeated chrome/body lifetime mismatch");
        }else if(name=="motion"){
            (void)frames.step(now+210ms);check(take(modal.root().snapshot()).reveal_progress>1,"Modal lost original Back overshoot");const auto expanded=take(modal.root().snapshot()).bounds.height;
            good(modal.close());auto exit=take(modal.root().snapshot());check(exit.exiting&&exit.visible&&exit.bounds.height<expanded&&take(title.snapshot()).visible&&!editor.snapshot()&&retired==1,"Exit did not immediately drop Loader body and retain shrinking chrome");
            check(!action.activate(),"Exit accepted action input");(void)frames.step(now+280ms);auto progress=take(modal.root().snapshot()).reveal_progress;good(modal.open());check(made==2&&retired==1&&take(modal.root().snapshot()).reveal_progress==progress&&bool(title.snapshot()),"Reversed Modal did not use fresh body and continuous chrome motion");
            good(prefs->set_reduced_motion(true));check(!take(modal.root().snapshot()).transitioning,"Reduced motion failed to complete current Modal");
        }else if(name=="preferences"){
            good(modal.set_motion_enabled(false));check(take(modal.root().snapshot()).reveal_progress==1&&!take(modal.root().snapshot()).transitioning,"Local motion switch did not settle");good(modal.close());check(!take(title.snapshot()).visible,"Disabled motion retained chrome exit");
            good(modal.set_motion_enabled(true));good(prefs->set_speed(2));good(modal.open());(void)frames.step(now+240ms);check(!take(modal.root().snapshot()).transitioning,"Shared speed did not shorten Modal duration");
            frames.set_paused(true);good(frames.set_time_scale(0));good(modal.close());(void)frames.step(now+600ms);check(!take(modal.root().snapshot()).visible,"Game pause froze Modal exit");
        }else if(name=="input"){
            (void)frames.step(now+70ms);auto r=take(editor.snapshot()).visual_bounds;good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+r.width/2,.y=r.y+r.height/2}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+r.width/2,.y=r.y+r.height/2}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::text,.text="Edited"}));check(take(editor.snapshot()).text=="Edited","Body input did not follow transformed frame");
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::tab}));check(take(take(mount.find("modal.tertiary")).snapshot()).focused,"Body Tab did not enter static action row");
            good(action.activate());check(clicked==1&&take(modal.root().snapshot()).opened,"Static action auto-closed Modal");
        }else if(name=="failure"){
            good(modal.close());check(take(modal.root().snapshot()).opened&&made==3&&retired==2&&!editor.snapshot()&&take(title.snapshot()).id==title_id,"Closed callback did not reopen one fresh body with stable chrome");good(modal.close());check(retired==3&&closed==2,"Reentrant lifecycle did not balance");
        }else if(name=="transparent"){
            good(prefs->set_reduced_motion(true));auto image=take(host.capture_canvas());auto r=take(modal.root().snapshot()).bounds;auto scale=take(host.snapshot()).backing_scale;const auto* p=&image.rgba[(std::size_t((r.y+r.height-12)*scale)*image.width+std::size_t((r.x+30)*scale))*4];
            check(p[0]>180&&p[0]<200&&p[1]<40&&p[2]<40,"Translucent authored frame did not blend in linear light");
        }else throw std::runtime_error("Unknown native Modal motion test");
        good(host.request_close());
    }}));check(retired==made,"Modal body survived host teardown");
}
}
int main(int argc,char**argv){try{std::string name=argc>1?argv[1]:"chrome";run(name);std::cout<<"NATIVE_MODAL_MOTION_OK "<<name<<'\n';}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
