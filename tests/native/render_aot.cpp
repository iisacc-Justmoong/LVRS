#include "NativeRenderAot.generated.h"
#include "NativeRenderSelectionAot.generated.h"
#include <iiGUI/WidgetState.h>
#include <iostream>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
template<class V>struct Mounted final{
 std::shared_ptr<gui::detail::WidgetState> tree=std::make_shared<gui::detail::WidgetState>([](runtime::Task task){task();});std::shared_ptr<typename V::Scope> scope;
 Mounted(const lvui::native::BackendFactories& factories={}){good(tree->construct([&](gui::WidgetBuilder& b){return V::build(b,{}, {},&scope,factories);}));}
 ~Mounted(){tree->close();scope.reset();}
 gui::Widget find(std::string_view name){for(const auto& row:take(tree->snapshots()))if((row.id==name||row.id.ends_with("/"+std::string(name))))return take(tree->find(row.id));throw std::runtime_error("Missing render widget");}
 std::string text(std::string_view name){return take(find(name).snapshot()).text;}
 void click(std::string_view name){good(find(name).activate());}
};
using Default=lvui_generated::NativeRenderAot;using Selected=lvui_generated::NativeRenderSelectionAot;
void defaults(){Mounted<Default> m;check(m.text("active")=="false"&&m.text("frames")=="0"&&m.text("capacity")=="240"&&m.text("threshold")=="20"&&m.text("fps")=="0","Default monitor not projected");}
void frames(){Mounted<Selected> m;m.click("sample8");check(m.text("frames")=="9"&&m.text("snapshot")=="9"&&m.text("p95")=="16"&&m.text("initial")=="0"&&m.text("fixedSnapshot")=="0"&&m.text("derivedSnapshot")=="9","Native frame/stat/snapshot dependencies or Static value wrong");}
void stop_resume(){Mounted<Selected> m;m.click("sample");m.click("sample");m.click("pause");m.click("sample");check(m.text("active")=="false"&&m.text("frames")=="2","Paused view continued counting");m.click("resume");m.click("sample");check(m.text("active")=="true"&&m.text("frames")=="3","Resumed source did not reach view");}
void reset(){Mounted<Selected> m;m.click("sample8");m.click("reset");check(m.text("frames")=="0"&&m.text("samples")=="0"&&m.text("active")=="true","Reset rebuilt scope or disconnected source");}
void replacement(){Mounted<Selected> m;m.click("sample8");auto old=m.scope->backend_0.source;m.click("replace");check(m.text("frames")=="0"&&!old->has_observers(),"Old source still projects");m.click("sample");check(m.text("frames")=="1","New source not projected");}
void closure(){Mounted<Selected> m;m.click("sample8");m.click("close");check(m.text("active")=="false"&&m.text("frames")=="9","Closure changed completed statistics");}
void identity(){Mounted<Selected> m;auto field=m.find("edit");good(field.set_text("Retained"));good(field.focus());auto id=take(field.snapshot()).id;auto nodes=take(m.tree->snapshots()).size();m.click("sample8");check(take(field.snapshot()).id==id&&take(field.snapshot()).text=="Retained"&&take(field.snapshot()).focused&&take(m.tree->snapshots()).size()==nodes&&m.scope->backend_0.file_revision==0,"Stats changed input/focus/nodes or file revision");}
void retirement(){Mounted<Selected> m;auto source=m.scope->backend_0.source;auto old=m.find("frames");std::weak_ptr<Selected::Scope> scope=m.scope;m.tree->close();m.scope.reset();check(scope.expired()&&NativeRenderBackend::live==0&&!source->has_observers()&&!old.snapshot(),"Render source/import survived activation");}
void constructors(){auto source=std::make_shared<gui::PresentationStream>();auto constructors=lvui::native::BackendFactories{}.with<lvrs::RenderMonitor>([source]{return lvrs::RenderMonitor(source);});Mounted<Default> m(constructors);check(m.text("active")=="true","Frame source constructor policy not applied");good(source->queued(runtime::Clock::time_point{},gui::GraphicsApi::metal));check(m.text("frames")=="1","Constructed native monitor not projected");}
}
int main(int argc,char** argv){try{const std::string name=argc>1?argv[1]:"defaults";if(name=="defaults")defaults();else if(name=="frames")frames();else if(name=="stop_resume")stop_resume();else if(name=="reset")reset();else if(name=="replacement")replacement();else if(name=="closure")closure();else if(name=="identity")identity();else if(name=="retirement")retirement();else if(name=="constructors")constructors();else throw std::runtime_error("Unknown render AOT case");check(NativeRenderBackend::live==0,"Render backend leaked");std::cout<<"Native render AOT "<<name<<" passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
