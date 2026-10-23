#include <LVRS/Navigator.h>
#include <LVRS/Widgets.h>
#include <iiGUI/iiGUI.h>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
void run(std::string name){
    gui::Host host({.size={620,480}});gui::WidgetMount mount;auto navigator=name=="default"?lvrs::Navigator::current():std::make_shared<lvrs::Navigator>();
    auto other=std::make_shared<lvrs::Navigator>();std::shared_ptr<lvrs::PageRouter> inner;
    auto page=std::make_shared<const lvrs::PageComponent>([](auto& b,auto parent,const auto& context)->core::Result<void>{auto root=b.vstack(parent,"page");if(!root)return std::unexpected(root.error());auto label=b.label(*root,"title",{.text=context.path});if(!label)return std::unexpected(label.error());return {};});
    auto failed_page=std::make_shared<const lvrs::PageComponent>([](auto&,auto,const auto&)->core::Result<void>{return core::fail(core::ErrorCode::failed,"Intentional candidate construction failure");});
    auto create=[&](std::shared_ptr<lvrs::Navigator> service,bool enabled=true){return take(lvrs::PageRouter::create({.routes={{"/",page},{"/two",page},{"/bad",failed_page}},.navigator=std::move(service),.register_as_global_navigator=enabled}));};
    auto first=create(name=="default"?nullptr:navigator),second=create(name=="isolated"?other:name=="default"?nullptr:navigator,name!="disabled");
    if(name=="nested"){
        auto nested_page=std::make_shared<const lvrs::PageComponent>([&](auto& b,auto parent,const auto&)->core::Result<void>{auto root=take(b.vstack(parent,"page"));inner=create(navigator);take(inner->mount(b,root,"nested",{280,140}));return {};});
        first=take(lvrs::PageRouter::create({.routes={{"/",nested_page},{"/two",page}},.navigator=navigator}));
    }
    good(host.run({.opened=[&]{
        mount=take(host.mount_widgets([&](auto& b)->core::Result<void>{auto root=take(b.vstack({},"root"));take(first->mount(b,root,"first",{580,180}));if(name!="nested")take(second->mount(b,root,"second",{580,180}));return {};}));
        if(name=="nested"){
            check(take(navigator->router())==inner&&take(navigator->state()).router_count==2,"Nested router did not register after its parent");
            take(navigator->go("/two"));check(take(inner->state()).navigation.current_path=="/two"&&take(first->state()).navigation.current_path=="/","Global dispatch did not target the nested page");
            take(first->go("/two"));check(take(navigator->router())==first&&take(navigator->state()).router_count==1&&!inner->state(),"Retired nested scope remained globally active");
        }else if(name=="isolated"){
            check(take(navigator->router())==first&&take(other->router())==second,"Explicit navigation domains were mixed");take(navigator->go("/two"));check(take(first->state()).navigation.current_path=="/two"&&take(second->state()).navigation.current_path=="/","Dispatch crossed an explicit domain");
        }else if(name=="disabled"){
            check(take(navigator->router())==first&&take(navigator->state()).router_count==1,"Opted-out router registered globally");
        }else if(name=="siblings"||name=="default"||name=="tracking"){
            check(take(navigator->router())==second,"Last native sibling did not become active");take(navigator->register_router(first));check(take(navigator->router())==second,"Manual re-registration lost native sibling placement");take(navigator->go("/two"));check(take(second->state()).navigation.current_path=="/two"&&take(first->state()).navigation.current_path=="/","Global command reached the wrong native sibling");
            if(name=="tracking"){
                auto tracker=take(navigator->view_state_tracker());check(take(tracker->loaded_views())==std::vector<std::string>{"/","/two"},"Compiled page history did not reach the shared tracker");
                const auto before=take(tracker->snapshot_value());check(!second->go("/bad")&&take(tracker->snapshot_value())==before&&take(second->state()).navigation.current_path=="/two","Failed page candidate mutated the published tracker");
                take(first->go("/two"));take(first->go("/"));check(take(tracker->loaded_views())==std::vector<std::string>{"/","/two"},"Inactive native page overwrote the selected tracker");
                good(navigator->unregister_router(second));check(take(tracker->loaded_views())==std::vector<std::string>{"/","/two","/"},"Native selection restoration did not synchronize its complete history");
            }
        }else throw std::runtime_error("Unknown native Navigator GUI case");
        good(host.unmount_widgets());check(!take(navigator->state()).has_router&&!take(other->state()).has_router,"Unmount retained registrations despite externally owned controllers");
        check(take(take(navigator->view_state_tracker())->state()).loaded_count==0,"Unmount retained logical tracking entries");
        good(host.request_close());
    }}));
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"siblings");std::cout<<"LVRS_NAVIGATOR_GUI_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
