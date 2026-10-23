#include <LVRS/Widgets.h>
#include <iiGUI/iiGUI.h>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
struct Scope final {int& retired;std::function<void()> on_destroy;explicit Scope(int& count):retired(count){}~Scope(){++retired;if(on_destroy)on_destroy();}};
void run(std::string name){
    gui::Host host({.size={620,480}});gui::WidgetMount mount;gui::Widget slot,action;
    int created=0,retired=0,clicked=0,notifications=0,failed=0;std::weak_ptr<Scope> active_scope;std::shared_ptr<lvrs::PageRouter> router;
    auto page=std::make_shared<const lvrs::PageComponent>([&](gui::WidgetBuilder& b,gui::Widget parent,const lvrs::NavigationEntry& entry)->core::Result<void>{
        ++created;auto scope=std::make_shared<Scope>(retired);active_scope=scope;good(b.retain(scope));
        if(name=="reentrant"){auto rejected=router->go("/two");check(!rejected&&rejected.error().code==core::ErrorCode::busy,"Page factory admitted recursive navigation");}
        auto root=take(b.vstack(parent,"page",{.spacing=6,.padding=10,.alignment=gui::StackAlignment::stretch}));take(b.label(root,"title",{.text=entry.path.empty()?"Component":entry.path}));
        action=take(b.button(root,"action",{.text="Native action"},[&]{++clicked;}));return {};
    });
    auto broken=std::make_shared<const lvrs::PageComponent>([](gui::WidgetBuilder& b,gui::Widget parent,const auto&)->core::Result<void>{auto root=b.label(parent,"broken",{.text="Rejected"});if(!root)return std::unexpected(root.error());return core::fail(core::ErrorCode::failed,"Rejected page");});
    auto throwing=std::make_shared<const lvrs::PageComponent>([](gui::WidgetBuilder&,gui::Widget,const auto&)->core::Result<void>{throw std::runtime_error("Page constructor exception");});
    auto options=lvrs::PageRouterOptions{.routes={{"/",page},{"/two",page},{"/reports/[id]",page},{"/broken",broken},{"/throw",throwing}}};if(name=="fallback")options.not_found_component=page;
    router=take(lvrs::PageRouter::create(std::move(options)));
    auto port=std::make_shared<lvrs::PageNavigationPort>([&](const auto& change){++notifications;auto state=take(router->state());check(state.navigation.revision==change.navigation.revision&&state.content_generation==change.content_generation,"Receiver saw inconsistent page and history");check(bool(router->current_page_item()),"Receiver saw no current page");if(name=="observation"){check(!router->back(),"Notification admitted recursive mutation");if(change.navigation.depth==2)throw std::runtime_error("Receiver failure");}});
    auto failure=std::make_shared<lvrs::PageNavigationFailurePort>([&](const auto&){++failed;check(bool(router->state()),"Failure receiver cannot inspect retained state");});
    good(router->observe(port));good(router->observe_failures(failure));
    good(host.run({.opened=[&]{
        mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));lvrs::Widgets controls(b);slot=take(controls.page_router(root,"router",router,{580,400}));return {};}));
        auto old=action;const auto before=take(router->state());check(before.navigation.depth==1&&notifications==1,"Initial page was not committed once");
        if(name=="lifetime"){
            take(router->go("/two"));check(!old.snapshot()&&created==2&&retired==1&&take(router->state()).navigation.depth==2,"Forward page did not retire its old scope");
            const auto second=action;take(router->back());check(!second.snapshot()&&created==3&&retired==2&&take(router->state()).navigation.depth==1,"Back did not rebuild an independent page generation");
            good(host.unmount_widgets());check(retired==3&&!router->go("/two"),"Unmount retained a page or admitted navigation");
        }else if(name=="parameters"){
            take(router->go("reports/42/",core::Value{core::Value::Object{{"id",core::Value{43}},{"enabled",core::Value{false}}}}));
            auto state=take(router->state());check(state.navigation.current_path=="/reports/42","Router lost normalized path");auto params=take(state.navigation.current_params.get<core::Value::Object>());check(take(params.get().at("id").get<std::int64_t>()).get()==43&&!take(params.get().at("enabled").get<bool>()).get(),"Explicit typed parameters did not override captures");
            const auto builds=created;check(!router->go("/two",core::Value{7})&&created==builds,"Non-object route parameters constructed a page");
        }else if(name=="components"){
            take(router->go_to(page,core::Value{7}));check(take(router->state()).navigation.current_path.empty()&&take(router->path()).back().component==page,"Typed compiled component lost identity");
            take(router->replace_with(page,core::Value{8}));check(take(router->state()).navigation.depth==2,"Component replace pushed history");
            const auto builds=created;check(!take(router->replace_with(page,core::Value{8}))&&created==builds,"Unchanged replace reconstructed the page");
            take(router->set_root_component(page));check(take(router->state()).navigation.depth==1&&!take(router->back()),"Component root retained old history");
        }else if(name=="atomic"){
            for(auto path:{"/missing","/broken","/throw"}){auto rejected=router->go(path);check(!rejected&&old.snapshot()&&take(router->state()).navigation.revision==before.navigation.revision&&take(router->state()).content_generation==before.content_generation,"Failed page changed visible generation or history");}
            check(failed==3&&notifications==1,"Failure and success ports were mixed");take(router->go("/two"));
        }else if(name=="observation"){
            auto result=router->go("/two");check(!result&&result.error().cause&&take(router->state()).navigation.current_path=="/two"&&!old.snapshot(),"Receiver failure rolled back committed navigation");
        }else if(name=="reentrant"){
            take(router->go("/two"));check(created==2&&take(router->state()).navigation.depth==2,"Constructor recursion changed history");
        }else if(name=="fallback"){
            take(router->go("/missing"));check(take(router->state()).navigation.current_path=="/missing"&&take(action.snapshot()).text=="Native action","Fallback page did not preserve missing route");
        }else if(name=="input"){
            auto r=take(action.snapshot()).bounds;good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+5,.y=r.y+5}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+5,.y=r.y+5}));good(action.focus());good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));check(clicked==1,"Page action activated before Enter release");good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_up,.key=gui::WidgetKey::enter}));check(clicked==2,"Page input did not use the existing native surface");
        }else if(name=="geometry"){
            auto root=take(take(router->current_page_item()).snapshot());auto bounds=take(slot.snapshot()).bounds;check(root.bounds==bounds&&root.parent_id=="router"&&root.popup_id.empty(),"Page became an independent popup or lost viewport ownership");
            good(slot.set_extent({500,300}));check(take(take(router->current_page_item()).snapshot()).bounds==take(slot.snapshot()).bounds,"Page root did not follow resize");take(host.capture_canvas());
        }else if(name=="ownership"){
            bool rejected=false;std::thread worker([&]{auto r=router->state();rejected=!r&&r.error().code==core::ErrorCode::wrong_executor;});worker.join();check(rejected,"Router accepted another thread");
        }else if(name=="limits"){
            auto rejected=router->go("/two",core::Value{core::Value::Object{{"large",core::Value{std::string(lvrs::NavigationLimits::entry_bytes,'x')}}}});check(!rejected&&created==1&&old.snapshot(),"Oversized parameters ran the page constructor");
            rejected=router->go("/two",{},static_cast<lvrs::NavigationOperation>(99));check(!rejected&&created==1,"Invalid operation ran the page constructor");
            check(!router->go_to({})&&created==1,"Null compiled component ran a page constructor");
        }else if(name=="retirement"){
            auto retiring=std::make_shared<const lvrs::PageComponent>([&](gui::WidgetBuilder& b,gui::Widget,const auto&)->core::Result<void>{take(b.label({},"title",{.text="retire"}));good(host.unmount_widgets());return {};});
            auto result=router->go_to(retiring);check(!result&&result.error().code==core::ErrorCode::closed&&!old.snapshot(),"Retired host published an orphan page");
        }else if(name=="destructor_retirement"){
            auto scope=active_scope.lock();scope->on_destroy=[&]{good(host.unmount_widgets());};scope.reset();auto result=router->go("/two");check(!result&&result.error().code==core::ErrorCode::closed&&!old.snapshot(),"Old page destructor left an orphan generation");
        }else throw std::runtime_error("Unknown page router case");
        good(host.request_close());
    }}));
    check(retired==created,"Page scopes remained alive after host shutdown");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"lifetime");std::cout<<"LVRS_PAGE_ROUTER_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
