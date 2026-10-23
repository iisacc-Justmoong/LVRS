#include <LVRS/Widgets.h>
#include <iiGUI/iiGUI.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}return std::move(*r);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
void layout(){
    gui::Host host({.size={1000,700}});lvrs::Modal modal;gui::WidgetMount mount;
    good(host.run({.opened=[&]{mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));modal=take(lvrs::Modal::create(b,root,"modal",{.title="Native dialog",.message="Fallback description",.icon_name="projectStructure",.secondary_text="Later",.tertiary_text="More"}));return {};},lvrs::Theme().appearance()));
        check(!modal.content_item()&&take(modal.action_visible(3)),"Closed/content/action state invalid");good(modal.open());auto popup=modal.root();auto r=take(popup.snapshot()).bounds;check(r.width==760&&std::abs(r.x-120)<1&&std::abs(r.y-((700-r.height)/2-16))<1&&r.height>=210,"Original Modal geometry changed");
        auto icon=take(take(popup.popup_content("modal.icon")).snapshot()),title=take(take(popup.popup_content("modal.title")).snapshot()),desc=take(take(popup.popup_content("modal.description")).snapshot());
        check(icon.size==gui::Size{72,72}&&title.text=="Native dialog"&&desc.text=="Fallback description"&&title.bounds.x>=icon.bounds.x+88-1,"Header/icon/description contract invalid");
        auto a=take(take(popup.popup_content("modal.tertiary")).snapshot()),b=take(take(popup.popup_content("modal.secondary")).snapshot()),c=take(take(popup.popup_content("modal.primary")).snapshot());
        check(a.bounds.x<b.bounds.x&&b.bounds.x<c.bounds.x&&c.size.height==20&&std::abs(c.bounds.x+c.bounds.width-(r.x+r.width-20))<1,"Actions are not trailing tertiary/secondary/primary at original size");
        good(host.set_content_size({600,700}));check(take(popup.snapshot()).bounds.width==552,"Modal width did not follow viewport");check(take(modal.contains_frame_point(300,350))&&!take(modal.contains_frame_point(0,0)),"Frame hit test invalid");good(host.request_close());
    }}));check(!modal.open(),"Retired Modal handle remained live");
}
void actions(){
    gui::Host host;lvrs::Modal modal;gui::Widget locked,empty;int primary=0,secondary=0,canceled=0,closed=0;
    good(host.run({.opened=[&]{auto mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));modal=take(lvrs::Modal::create(b,root,"modal",{.button_count=9,.primary_text="",.secondary_enabled=false},{.primary_clicked=[&]{++primary;},.secondary_clicked=[&]{++secondary;},.canceled=[&]{++canceled;},.closed=[&]{++closed;}}));locked=take(lvrs::Widgets(b).modal(root,"locked",{.primary_text="",.dismiss_on_background=false}));empty=take(lvrs::Widgets(b).modal(root,"empty",{.show_icon=false,.primary_text=""}));return {};},lvrs::Theme().appearance()));
        good(modal.open());check(take(modal.action_visible(3))&&!take(modal.action_visible(4)),"Explicit action count not clamped");check(take(take(modal.root().popup_content("modal.primary")).snapshot()).text=="OK","Primary fallback lost");
        good(take(modal.root().popup_content("modal.primary")).activate());check(primary==1&&take(modal.root().snapshot()).opened,"Primary action auto-closed Modal");check(!take(modal.trigger_action(2))&&secondary==0,"Disabled action fired");good(modal.close());check(closed==1&&canceled==0,"Close emitted cancel");good(modal.open());check(take(modal.cancel())&&!take(modal.cancel())&&closed==2&&canceled==1,"Cancel idempotence failed");
        good(locked.open_at(0,0));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=1,.y=1}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=1,.y=1}));check(take(locked.snapshot()).opened,"dismissOnBackground false ignored");good(locked.close_popup());good(empty.open_at(0,0));check(!empty.popup_content("empty.icon")&&!empty.popup_content("empty.primary")&&take(empty.snapshot()).bounds.height==210,"Empty native Modal did not preserve minimum/no-actions policy");good(host.request_close());
    }}));
}
struct Lifetime final {int& count;explicit Lifetime(int& n):count(n){}~Lifetime(){++count;}};
void lifetime(){
    gui::Host host;lvrs::Modal modal;int made=0,retired=0;gui::Widget old;std::string first;
    good(host.run({.opened=[&]{auto mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));modal=take(lvrs::Modal::create(b,root,"modal",{.show_icon=false,.primary_text=""},{},[&](gui::WidgetBuilder& c,gui::Widget parent)->core::Result<void>{++made;good(c.retain(std::make_shared<Lifetime>(retired)));take(c.input_field(parent,"editor",{.text="Fresh"}));return {};}));return {};},lvrs::Theme().appearance()));
        for(int i=0;i<30;++i){good(modal.open());check(made==i+1&&retired==i,"Content lifetime not tied to open");auto now=take(modal.root().popup_content("editor"));auto id=take(now.snapshot()).id;if(!i){first=id;old=now;}else check(id!=first&&!old.snapshot(),"Retired content reused old generation");check(bool(modal.content_item()),"Active content item missing");good(now.set_text("Edited"));good(host.set_content_size({640.0+i,500}));check(made==i+1,"Resize reconstructed content");good(modal.close());check(retired==i+1&&!modal.content_item(),"Close did not retire content before return");}
        good(host.request_close());
    }}));
}
void rendering(){
    gui::Host host({.size={800,600}});lvrs::Modal modal;
    good(host.run({.opened=[&]{auto mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));take(b.button(root,"behind",{.text="Blocked"}));modal=take(lvrs::Modal::create(b,root,"modal",{.title="Native Modal",.frame_color={.2,.4,.6,1},.backdrop_color={0,0,0,.5},.motion_enabled=false}));check(!lvrs::Modal::create(b,root,"bad",{.minimum_width=1000,.maximum_width=500}),"Invalid Modal range accepted");return {};},lvrs::Theme().appearance()));
        good(modal.open());auto r=take(modal.root().snapshot()).bounds;auto image=take(host.capture_canvas());const auto scale=take(host.snapshot()).backing_scale;const auto x=std::uint32_t((r.x+30)*scale),y=std::uint32_t((r.y+r.height-25)*scale);auto p=&image.rgba[(std::size_t(y)*image.width+x)*4];check(std::abs(int(p[0])-51)<4&&std::abs(int(p[1])-102)<4&&std::abs(int(p[2])-153)<4,"Modal surface did not reach actual GPU pixels");check(take(take(mount.find("behind")).snapshot()).input_blocked,"Modal did not block background");good(host.request_close());
    }}));
}
}
int main(int argc,char** argv){try{std::string name=argc>1?argv[1]:"layout";if(name=="layout")layout();else if(name=="actions")actions();else if(name=="lifetime")lifetime();else if(name=="rendering")rendering();else throw std::runtime_error("Unknown Modal test");std::cout<<"LVRS_NATIVE_MODAL_OK "<<name<<'\n';}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
