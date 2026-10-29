#include "NativeRegistryAot.generated.h"
#include "NativeScopedRegistryAot.generated.h"
#include <iiGUI/WidgetState.h>
#include <iostream>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> result){if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}return std::move(*result);}
void good(core::Result<void> result){if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}}
template<class View>struct Mounted final {
    std::shared_ptr<gui::detail::WidgetState> state=std::make_shared<gui::detail::WidgetState>([](runtime::Task action){action();});
    gui::Widget host=take(state->content_host({},"root",{640,480}));std::shared_ptr<typename View::Scope> scope;
    Mounted(){good(host.set_content([&](gui::WidgetBuilder& builder){return View::build(builder,{}, {},&scope);}));}
    ~Mounted(){state->close();scope.reset();}
    gui::Widget find(std::string_view name){for(const auto& node:take(state->snapshots()))if(node.id.ends_with(name))return take(state->find(node.id));throw std::runtime_error("Missing registry widget");}
    std::string text(std::string_view name){return take(find(name).snapshot()).text;}
    void click(std::string_view name){good(find(name).activate());}
};
using View=lvui_generated::NativeRegistryAot;
void actions(){Mounted<View> m;check(m.text("/name")=="Inspector"&&m.text("/type")=="StateModel"&&m.text("/models")=="1","Core model properties did not render through generated C++");m.click("/toggle");check(m.text("/busy")=="true"&&m.scope->backend_0.model->busy(),"Native registry write did not refresh generated data");m.click("/denied");check(m.text("/name")=="Inspector"&&m.text("/error")=="View has no write permission for the model","A reader changed data or lost the native error");}
void external(){Mounted<View> m;take(m.scope->backend_0.model->set_display_name("External"));check(m.text("/name")=="External","Private model descriptor notification was not observed");m.click("/release");check(!take(m.find("/toggle").snapshot()).enabled,"Ownership release left the generated action enabled");}
void replacement(){Mounted<View> m;auto old=m.scope->backend_0.model;const auto id=take(m.find("/name").snapshot()).id;m.click("/replace");check(m.text("/name")=="Replacement"&&take(m.find("/name").snapshot()).id==id,"Replacing a model rebuilt the view or left old data");take(old->set_display_name("Obsolete"));check(m.text("/name")=="Replacement","A replaced source still controlled the generated view");}
void nonmutation(){Mounted<View> m;auto& backend=m.scope->backend_0;for(int n=0;n<100;++n)(void)take(m.state->snapshots());check(backend.file_revision.get()==0&&backend.model->revision()==0&&!backend.model->busy()&&backend.registry.can_write("Editor"),"View inspection changed file, model or ownership state");}
void retirement(){Mounted<View> m;auto old=m.find("/toggle");auto model=m.scope->backend_0.model;std::weak_ptr<typename View::Scope> weak=m.scope;good(m.host.clear_content());m.scope.reset();check(weak.expired()&&!old.snapshot(),"Registry generation retained its scope or actions");take(model->set_busy(true));}
void scoped(){Mounted<lvui_generated::NativeScopedRegistryAot> m;check(m.text("/models")=="0"&&m.text("/writable")=="false","Scoped ViewModels alias was not independently constructed");}
}
int main(int argc,char** argv){try{std::string name=argc>1?argv[1]:"actions";if(name=="actions")actions();else if(name=="external")external();else if(name=="replacement")replacement();else if(name=="nonmutation")nonmutation();else if(name=="retirement")retirement();else if(name=="scoped")scoped();else throw std::runtime_error("Unknown registry AOT case");std::cout<<"Native registry AOT "<<name<<" passed\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
