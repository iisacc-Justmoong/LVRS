#include "NativeModelSourceAot.generated.h"
#include "NativeModelSourceSelectionAot.generated.h"
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
    gui::Widget find(std::string_view name){for(const auto& node:take(tree->snapshots()))if((node.id==name||node.id.ends_with("/"+std::string(name))))return take(tree->find(node.id));throw std::runtime_error("Missing generated model-source widget");}
    std::string text(std::string_view name){return take(find(name).snapshot()).text;}
};
using Default=lvui_generated::NativeModelSourceAot;using Selected=lvui_generated::NativeModelSourceSelectionAot;
void defaults(){Mounted<Default> m;check(m.text("count")=="0"&&m.text("column")=="0"&&m.text("revision")=="0"&&m.text("item-model")=="false"&&m.text("adapter-count")=="0","Scoped native source/adapter defaults not projected");}
void append(){Mounted<Selected> m;good(m.find("append").activate());check(m.text("count")=="3"&&m.text("label")=="First"&&m.text("revision")=="2","Native source event did not update bindings");}
void column(){Mounted<Selected> m;good(m.find("column").activate());check(m.text("label")=="First alternate"&&m.text("initial")=="First","Column projection lost reactivity or Static snapshot");}
void replace(){Mounted<Selected> m;good(m.find("replace").activate());check(m.text("count")=="1"&&m.text("label")=="Replacement","Source replacement did not rebind");const auto revision=m.scope->backend_0.source.revision();m.scope->backend_0.append();check(m.text("count")=="1"&&m.scope->backend_0.source.revision()==revision,"Retired source still changed the view");}
void identity(){Mounted<Selected> m;auto field=m.find("edit");good(field.set_text("Draft"));good(field.focus());const auto before=take(field.snapshot());const auto generation=take(m.host.snapshot()).content_generation;good(m.find("append").activate());good(m.find("replace").activate());const auto after=take(field.snapshot());check(before.id==after.id&&after.text=="Draft"&&after.focused&&take(m.host.snapshot()).content_generation==generation,"Source notifications rebuilt view/input state");}
void nonmutation(){Mounted<Selected> m;auto& vm=m.scope->backend_0;const auto rows=vm.primary->rows;const auto revision=vm.source.revision();for(int i=0;i<20;++i)(void)take(m.tree->snapshots());vm.alternateColumn();check(vm.primary->rows==rows&&vm.file_revision==0&&vm.source.revision()==revision+1,"A display column or snapshot modified file/model data");}
void retirement(){Mounted<Selected> m;auto provider=m.scope->backend_0.primary;auto widget=m.find("label");std::weak_ptr<typename Selected::Scope> weak=m.scope;good(m.host.clear_content());m.scope.reset();check(weak.expired()&&NativeModelSourceBackend::live==0&&!widget.snapshot(),"Scoped native source/backend survived retirement");provider->append();}
}
int main(int argc,char** argv){try{const std::string name=argc>1?argv[1]:"defaults";if(name=="defaults")defaults();else if(name=="append")append();else if(name=="column")column();else if(name=="replace")replace();else if(name=="identity")identity();else if(name=="nonmutation")nonmutation();else if(name=="retirement")retirement();else throw std::runtime_error("Unknown model-source AOT case");check(NativeModelSourceBackend::live==0,"Model-source backend leaked");std::cout<<"Native model source AOT "<<name<<" passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
