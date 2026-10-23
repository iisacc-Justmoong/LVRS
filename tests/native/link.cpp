#include <LVRS/Link.h>
#include <LVRS/PageRouter.h>
#include <iiGUI/Bindings.h>
#include <iiGUI/iiGUI.h>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
struct Target final:lvrs::NavigationTarget {
    lvrs::NavigationStackModel history;std::function<void()> on_go;bool fail=false;int calls=0;
    Target(){take(history.apply_path_operation("/"));}
    core::Result<lvrs::NavigationTargetState> state()const override{auto value=history.state();if(!value)return std::unexpected(value.error());return lvrs::NavigationTargetState{.navigation=*value,.mounted=true,.can_go_back=value->depth>1};}
    core::Result<bool> go(std::string_view path,core::Value params,lvrs::NavigationOperation operation)override{++calls;if(on_go)on_go();if(fail)return core::fail(core::ErrorCode::failed,"Rejected route");return history.apply_path_operation(path,std::move(params),operation);}
    core::Result<bool> go_to(lvrs::PageComponentRef page,core::Value params,lvrs::NavigationOperation operation)override{++calls;return history.apply_component_operation(std::move(page),std::move(params),operation);}
    core::Result<bool> back()override{return history.pop();}
    core::Result<bool> pop_to_root()override{return history.pop_to_root();}
    core::Result<void> observe(std::weak_ptr<lvrs::NavigationTargetPort>)override{return {};}
};
void run(std::string name){
    gui::Host host({.size={640,440}});gui::WidgetMount mount;lvrs::Link link;gui::Widget child;
    auto target=std::make_shared<Target>(),fallback=std::make_shared<Target>();auto navigator=std::make_shared<lvrs::Navigator>();take(navigator->register_router(fallback));
    int clicks=0,methods=0,failures=0;core::Observable<std::string> href{"/bound"};core::Observable<bool> replace{false};core::Observable<gui::WidgetColor> tint{gui::WidgetColor{.2,.4,.8,1}};
    good(host.run({.opened=[&]{
        mount=take(host.mount_widgets([&](gui::WidgetBuilder& builder)->core::Result<void>{auto root=take(builder.vstack({},"root",{.alignment=gui::StackAlignment::leading}));
            lvrs::LinkOptions options{.text="Open report",.href="/reports/42",.params=core::Value{core::Value::Object{{"typed",core::Value{42}}}},.router=target,.navigator=navigator};
            if(name=="global")options.router={};
            if(name=="methods")options.href="";
            lvrs::LinkEvents events{.method=[&](const auto& event){check(event.trigger=="clicked"&&bool(event.source.snapshot()),"Button method lost its typed source");++methods;},.methods={[&](const auto&){++methods;},[&](const auto&){++methods;}},.clicked=[&]{++clicks;},.navigation_failed=[&](const auto&){++failures;}};
            link=take(lvrs::Link::create(builder,root,"link",options,events));
            if(name=="children"||name=="input"){auto row=take(builder.hstack(link.root(),"content",{.spacing=6}));take(builder.label(row,"label",{.text="Custom Link"}));child=take(builder.button(row,"child",{.text="Child"},[&]{++clicks;}));}
            if(name=="bindings"){
                auto bindings=std::make_shared<gui::BindingScope>();good(builder.retain(bindings));
                good(gui::bind_member(*bindings,link.root(),href,[link=link](gui::Widget,const std::string& value){return link.set_href(value);}));
                good(gui::bind_member(*bindings,link.root(),replace,[link=link](gui::Widget,bool value){return link.set_replace(value);}));
                good(gui::bind_member(*bindings,link.root(),tint,[link=link](gui::Widget,gui::WidgetColor value){return link.set_link_color(value);}));
                auto plain=target;check(gui::binding_value(plain)==target,"Plain shared_ptr was treated as Observable::get");
            }
            return {};}));
        const auto pointer=[&](gui::Widget widget){auto r=take(widget.snapshot()).bounds;good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+3,.y=r.y+3}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+3,.y=r.y+3}));};
        if(name=="geometry"){
            const auto snapshot=take(link.root().snapshot());check(snapshot.kind==gui::WidgetKind::action_surface&&snapshot.action_role==gui::ActionRole::link&&snapshot.implicit_size.height==36&&snapshot.font_size==12,"Link lost its own action composition or authored metrics");take(host.capture_canvas());
        }else if(name=="route"){
            good(link.root().activate());auto entry=take(target->history.current_entry_descriptor());check(entry.path=="/reports/42"&&take(entry.params.find("typed")).get()==core::Value{42}&&target->calls==1,"Typed explicit route dispatch failed");
        }else if(name=="alias"){
            good(link.set_to("/alias"));check(take(link.options()).href=="/alias"&&take(link.trigger_navigation()),"to did not alias href");good(link.set_href(""));check(!take(link.trigger_navigation())&&target->calls==1,"Empty href was dispatched");
        }else if(name=="replace"){
            take(link.trigger_navigation());good(link.set_replace(true));good(link.set_href("/replacement"));take(link.trigger_navigation());check(take(target->history.state()).depth==2&&take(target->history.state()).current_path=="/replacement","Link replace pushed another entry");
        }else if(name=="component"){
            auto page=std::make_shared<const lvrs::PageComponent>(lvrs::PageFactory{});good(link.set_target_component(page));good(link.set_href(""));take(link.trigger_navigation());check(take(target->history.current_entry_descriptor()).component==page,"Component did not take precedence over empty href");
        }else if(name=="global"){
            take(link.trigger_navigation());check(fallback->calls==1&&target->calls==0,"Link did not resolve the shared Navigator");
        }else if(name=="expired"){
            target.reset();take(link.trigger_navigation());check(fallback->calls==1,"Expired explicit router did not fall back to Navigator");
        }else if(name=="children"){
            check(take(link.root().snapshot()).custom_content&&take(link.root().snapshot()).bounds.height>=take(child.snapshot()).bounds.height+20,"Link child geometry was not remeasured after early construction snapshots");auto before=take(host.capture_canvas());good(link.set_text("hidden text"));auto after=take(host.capture_canvas());check(before.rgba==after.rgba,"Link fallback painted over custom native children");
        }else if(name=="input"){
            pointer(child);check(target->calls==0&&clicks==1,"Link intercepted a child control click");pointer(link.root());check(target->calls==1,"Link padding did not navigate");
        }else if(name=="states"){
            good(link.set_colors(gui::WidgetColor{.2,.4,.8,1},gui::WidgetColor{.3,.5,.9,1},gui::WidgetColor{.1,.2,.4,1},gui::WidgetColor{1,1,1,.6}));good(link.set_underline(true));good(link.set_text("Latest bound text"));good(link.set_enabled(false));good(link.set_link_color(gui::WidgetColor{.4,.6,.8,1}));check(!take(link.root().snapshot()).enabled&&take(link.root().snapshot()).text=="Latest bound text"&&take(link.root().snapshot()).text_underline,"Restyling overwrote latest text/enabled bindings");good(link.set_tone(gui::ButtonTone::disabled));good(link.set_enabled(true));check(!take(link.root().snapshot()).enabled,"Disabled tone lost its interaction gate");good(link.set_tone(gui::ButtonTone::normal));check(take(link.root().snapshot()).enabled,"Tone did not restore the requested enabled value");
        }else if(name=="bindings"){
            check(take(link.options()).href=="/bound","Initial Core binding was not applied");href.set("/live");replace.set(true);tint.set(gui::WidgetColor{.4,.6,.8,1});take(link.trigger_navigation());check(take(target->history.state()).current_path=="/live"&&take(target->history.state()).depth==1,"Live link input binding did not affect dispatch");good(host.unmount_widgets());href.set("/after-retirement");check(!link.options(),"Retired binding retained a live Link");
        }else if(name=="validation"){
            check(!link.set_href(std::string("/bad\0path",9))&&take(link.options()).href=="/reports/42","Invalid href partially changed Link");check(!link.set_params(core::Value{std::string(lvrs::NavigationLimits::entry_bytes,'x')}),"Oversized Link parameters accepted");check(!link.set_colors(gui::WidgetColor{2,0,0,1}),"Invalid Link color accepted");
        }else if(name=="methods"){
            good(link.root().activate());check(methods==3&&clicks==1&&target->calls==0,"No-op navigation suppressed typed methods or clicked");good(link.set_method([&](const auto&){good(host.unmount_widgets());}));good(link.root().activate());check(methods==3&&clicks==1&&!link.root().snapshot(),"Retired Link admitted subsequent method callbacks");
        }else if(name=="failure"){
            target->fail=true;good(link.root().activate());check(failures==1&&take(target->history.state()).depth==1,"Navigation error did not reach the failure callback");
        }else if(name=="reentrant"){
            target->on_go=[&]{auto result=link.trigger_navigation();check(!result&&result.error().code==core::ErrorCode::busy,"Link admitted recursive navigation");};take(link.trigger_navigation());check(target->calls==1,"Recursive Link duplicated navigation");
        }else if(name=="retirement"){
            target->on_go=[&]{good(host.unmount_widgets());};take(link.trigger_navigation());check(!link.root().snapshot(),"Navigation target could not retire Link owner");
        }else throw std::runtime_error("Unknown Link case");
        good(host.request_close());
    }}));
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"route");std::cout<<"LVRS_LINK_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
