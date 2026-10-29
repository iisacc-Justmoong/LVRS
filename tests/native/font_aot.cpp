#include "NativeFontAot.generated.h"
#include "NativeFontSelectionAot.generated.h"
#include <iiGUI/WidgetState.h>
#include <iostream>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
template<class View>struct Mounted final {
    std::shared_ptr<gui::detail::WidgetState> tree=std::make_shared<gui::detail::WidgetState>([](runtime::Task task){task();},gui::WidgetAppearance{},gui::detail::WidgetFrameHooks{.window_geometry=[](const auto&)->core::Result<void>{return {};}});
    std::shared_ptr<typename View::Scope> scope;
    Mounted(){good(tree->construct([&](gui::WidgetBuilder& b){return View::build(b,{}, {},&scope);}));good(tree->commit_window_properties());}
    ~Mounted(){tree->close();scope.reset();}
    gui::Widget find(std::string_view name){for(const auto& item:take(tree->snapshots()))if((item.id==name||item.id.ends_with("/"+std::string(name))))return take(tree->find(item.id));throw std::runtime_error("Missing native font widget");}
    std::string text(std::string_view name){return take(find(name).snapshot()).text;}
};
using Default=lvui_generated::NativeFontAot;using Selected=lvui_generated::NativeFontSelectionAot;
void defaults(){Mounted<Default> m;check(m.text("preferred")=="Pretendard"&&m.text("weight")=="600"&&m.text("style")=="SemiBold"&&m.text("compliant")=="true","Native font policy queries not projected");check(take(m.tree->appearance()).font_family==m.scope->backend_0.effectiveFamily(),"Default Window font binding did not reach the native appearance");}
void initial(){Mounted<Selected> m;check(m.text("effective")=="System"&&m.text("available")=="false"&&m.text("initial")=="System","Injected font environment ignored");}
void install(){Mounted<Selected> m;good(m.find("install").activate());check(m.text("effective")=="Pretendard"&&m.text("available")=="true"&&m.text("initial")=="System"&&take(m.tree->appearance()).font_family=="Pretendard","Font metadata did not update the existing Window or Static snapshot changed");}
void warning(){Mounted<Selected> m;good(m.find("enforce").activate());check(m.text("warning").find("unavailable")!=std::string::npos,"C++ font failure warning not displayed");good(m.find("install").activate());good(m.find("enforce").activate());check(m.text("warning").empty(),"A successful font apply did not clear its warning");}
void identity(){Mounted<Selected> m;auto field=m.find("edit");good(field.set_text("Retained"));good(field.focus());const auto before=take(field.snapshot());const auto nodes=take(m.tree->snapshots()).size();m.scope->backend_0.install();const auto after=take(field.snapshot());check(before.id==after.id&&after.text=="Retained"&&after.focused&&take(m.tree->snapshots()).size()==nodes&&m.scope->backend_0.file_revision==0,"Window typography rebuilt scope/input or changed file state");}
void retirement(){Mounted<Selected> m;auto old=m.find("effective");std::weak_ptr<typename Selected::Scope> weak=m.scope;m.tree->close();m.scope.reset();check(weak.expired()&&NativeFontBackend::live==0&&!old.snapshot(),"Font imports/bindings survived scope retirement");}
}
int main(int argc,char** argv){try{const std::string name=argc>1?argv[1]:"defaults";if(name=="defaults")defaults();else if(name=="initial")initial();else if(name=="install")install();else if(name=="warning")warning();else if(name=="identity")identity();else if(name=="retirement")retirement();else throw std::runtime_error("Unknown native font AOT case");check(NativeFontBackend::live==0,"Font backend leaked");std::cout<<"Native font AOT "<<name<<" passed\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
