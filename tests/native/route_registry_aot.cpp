#include "NativeRouteRegistryAot.generated.h"
#include <iiGUI/WidgetState.h>
#include <iostream>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> result){if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}return std::move(*result);}
void good(core::Result<void> result){if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}}
using View=lvui_generated::NativeRouteRegistryAot;
struct Mounted final {
    std::shared_ptr<gui::detail::WidgetState> state=std::make_shared<gui::detail::WidgetState>([](runtime::Task action){action();});
    gui::Widget host=take(state->content_host({},"root",{640,800}));std::shared_ptr<View::Scope> scope;
    Mounted(){good(host.set_content([&](gui::WidgetBuilder& widgets){return View::build(widgets,{}, {},&scope);}));}
    ~Mounted(){state->close();scope.reset();}
    gui::Widget find(std::string_view name){for(const auto& node:take(state->snapshots()))if(node.id.ends_with(name))return take(state->find(node.id));throw std::runtime_error("Missing route registry widget");}
    std::string text(std::string_view name){return take(find(name).snapshot()).text;}
    void click(std::string_view name){good(find(name).activate());}
};
void workflow(){Mounted m;check(m.text("/bound")=="1"&&m.text("/writable")=="false"&&m.text("/name").empty(),"Initial route did not refresh its generated bindings");auto label=m.find("/name");m.click("/edit");check(m.text("/bound")=="2"&&m.text("/writable")=="true"&&m.text("/name")=="Canvas"&&label.snapshot(),"Route publication did not update retained generated controls");m.click("/rename");check(m.text("/name")=="Renamed","Native model write did not reach the generated label");m.click("/back");check(m.text("/bound")=="1"&&m.text("/writable")=="false"&&!take(m.find("/rename").snapshot()).enabled,"Back left stale generated ownership state");}
void failed(){Mounted m;m.click("/edit");const auto generation=take(m.scope->backend_0.router->state()).content_generation;m.click("/failed");check(m.text("/name")=="Canvas"&&m.text("/writable")=="true"&&take(m.scope->backend_0.router->state()).content_generation==generation&&!m.scope->backend_0.registry->get_for_view("Broken"),"A failed page changed the generated selection or model permission");}
void retirement(){Mounted m;m.click("/edit");auto registry=m.scope->backend_0.registry;auto router=m.scope->backend_0.router;std::weak_ptr<View::Scope> weak=m.scope;good(m.host.clear_content());m.scope.reset();check(weak.expired()&&registry->views().empty()&&registry->owners().empty()&&!router->back(),"A retired generated view leaked its registry permissions or scope");}
void isolation(){Mounted a,b;a.click("/edit");a.click("/rename");check(a.text("/name")=="Renamed"&&b.text("/name").empty()&&b.text("/writable")=="false"&&b.text("/bound")=="1","Independent compiled activations shared a registry domain");}
void nonmutation(){Mounted m;auto& backend=m.scope->backend_0;const auto revision=backend.model->revision();for(int i=0;i<16;++i){m.click("/edit");m.click("/back");}check(backend.file_revision.get()==0&&backend.model->revision()==revision&&backend.model->display_name()=="Canvas","Pure route changes mutated document model data");}
}
int main(int argc,char** argv){try{const std::string name=argc>1?argv[1]:"workflow";if(name=="workflow")workflow();else if(name=="failed")failed();else if(name=="retirement")retirement();else if(name=="isolation")isolation();else if(name=="nonmutation")nonmutation();else throw std::runtime_error("Unknown route registry AOT case");std::cout<<"Native route registry AOT "<<name<<" passed\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
