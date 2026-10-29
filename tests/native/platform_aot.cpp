#include "NativePlatformAot.generated.h"
#include "NativePlatformSelectionAot.generated.h"
#include <iiGUI/WidgetState.h>
#include <iostream>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
template<class View>struct Mounted final {
    std::shared_ptr<gui::detail::WidgetState> tree=std::make_shared<gui::detail::WidgetState>([](runtime::Task task){task();});
    gui::Widget host=take(tree->content_host({},"host",{640,480}));std::shared_ptr<typename View::Scope> scope;
    Mounted(){good(host.set_content([&](gui::WidgetBuilder& b){return View::build(b,{}, {},&scope);}));}
    ~Mounted(){tree->close();scope.reset();}
    gui::Widget find(std::string_view name){for(const auto& node:take(tree->snapshots()))if((node.id==name||node.id.ends_with("/"+std::string(name))))return take(tree->find(node.id));throw std::runtime_error("Missing generated platform widget");}
    std::string text(std::string_view name){return take(find(name).snapshot()).text;}
};
using Default=lvui_generated::NativePlatformAot;using Selected=lvui_generated::NativePlatformSelectionAot;
void current(){Mounted<Default> m;check(m.text("canonical")==m.scope->backend_0.canonicalOs()&&m.text("architecture")==m.scope->backend_0.arch(),"Scoped native platform values did not reach the AOT view");}
void metadata(){Mounted<Default> m;check(m.text("alias")=="macos"&&m.text("target")=="ios"&&m.text("width")=="948"&&m.text("profile-order")=="wasm","Typed metadata/list results were not projected through Core Value");check(take(m.find("navigation").snapshot()).implicit_size.width==216,"Runtime profile metadata did not reach native layout");}
void initial(){Mounted<Selected> m;check(m.text("target")=="macos"&&m.text("backend")=="metal"&&m.text("width")=="1040","Selected immutable platform context was not read");}
void switching(){Mounted<Selected> m;good(m.find("phone").activate());check(m.text("target")=="ios"&&m.text("width")=="948"&&m.text("initial")=="macos","A context replacement lost native reactivity or the Static snapshot");good(m.find("mac").activate());check(m.text("target")=="macos"&&m.text("width")=="1040","The platform context could not switch back");}
void identity(){Mounted<Selected> m;auto field=m.find("edit");good(field.set_text("Draft"));good(field.focus());const auto before=take(field.snapshot());const auto generation=take(m.host.snapshot()).content_generation;m.scope->backend_0.selectPhone();const auto after=take(field.snapshot());check(before.id==after.id&&after.text=="Draft"&&after.focused&&take(m.host.snapshot()).content_generation==generation,"A platform profile update rebuilt the native view");}
void nonmutation(){Mounted<Selected> m;auto mac=m.scope->backend_0.mac;const auto before=mac->runtimeProfiles();for(int i=0;i<10;++i)(void)take(m.tree->snapshots());m.scope->backend_0.selectPhone();check(m.scope->backend_0.file_revision==0&&mac->runtimeProfiles()==before,"A view context selection changed file or source platform data");}
void retirement(){Mounted<Selected> m;auto old=m.find("target");std::weak_ptr<typename Selected::Scope> weak=m.scope;good(m.host.clear_content());m.scope.reset();check(weak.expired()&&NativePlatformBackend::live==0&&!old.snapshot(),"Native platform imports or bindings survived retirement");}
}
int main(int argc,char** argv){try{const std::string name=argc>1?argv[1]:"current";if(name=="current")current();else if(name=="metadata")metadata();else if(name=="initial")initial();else if(name=="switching")switching();else if(name=="identity")identity();else if(name=="nonmutation")nonmutation();else if(name=="retirement")retirement();else throw std::runtime_error("Unknown platform AOT case");check(NativePlatformBackend::live==0,"Platform backend leaked");std::cout<<"Native platform AOT "<<name<<" passed\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
