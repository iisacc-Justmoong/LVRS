#include "NativeHistoryAot.generated.h"
#include "NativeScopedHistoryAot.generated.h"
#include <iiGUI/WidgetState.h>
#include <iostream>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> result){if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}return std::move(*result);}
void good(core::Result<void> result){if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}}
template<class View>struct Mounted final {
    std::shared_ptr<gui::detail::WidgetState> state=std::make_shared<gui::detail::WidgetState>([](runtime::Task action){action();});
    gui::Widget host=take(state->content_host({},"root",{640,480}));
    std::shared_ptr<typename View::Scope> scope;
    Mounted(){good(host.set_content([&](gui::WidgetBuilder& builder){return View::build(builder,{}, {},&scope);}));}
    ~Mounted(){state->close();scope.reset();}
    gui::Widget find(std::string_view suffix){for(const auto& node:take(state->snapshots()))if(node.id.ends_with(suffix))return take(state->find(node.id));throw std::runtime_error("Missing history view control");}
    std::string text(std::string_view suffix){return take(find(suffix).snapshot()).text;}
    void click(std::string_view suffix){good(find(suffix).activate());}
};
using View=lvui_generated::NativeHistoryAot;
void buttons(){Mounted<View> m;m.click("/remember");m.click("/remember");check(m.text("/current")=="2"&&m.text("/undoDepth")=="2","Native history actions did not update generated labels");m.click("/undo");check(m.text("/current")=="1"&&m.text("/redoDepth")=="1"&&take(m.find("/redo").snapshot()).enabled,"Undo did not enable the generated redo action");m.click("/redo");check(m.text("/current")=="2"&&m.text("/redoDepth")=="0","Generated redo action did not consume the native transfer");}
void external(){Mounted<View> m;auto& history=m.scope->backend_0.history;take(history.push_snapshot(core::Value(5)));take(history.push_snapshot(core::Value(6)));check(m.text("/undoDepth")=="2"&&take(m.find("/undo").snapshot()).enabled,"Private native history observation was not collected by LVUI");take(history.set_limit(1));check(m.text("/undoDepth")=="1","A trimmed history limit left a generated label stale");}
void nonmutation(){Mounted<View> m;m.click("/remember");const auto depth=m.scope->backend_0.history.undo_depth();for(int n=0;n<100;++n)(void)take(m.state->snapshots());check(m.scope->backend_0.history.undo_depth()==depth&&m.scope->backend_0.current.get()==1&&m.scope->backend_0.file_revision.get()==0,"View inspection changed history, preview or file state");}
void retirement(){Mounted<View> m;auto old=m.find("/remember");auto graph=m.scope->native_lifetime.lock();std::weak_ptr<typename View::Scope> owner=m.scope;good(m.host.clear_content());check(!old.snapshot()&&graph&&!graph->available(),"Retired history actions retained their active graph");take(m.scope->backend_0.history.push_snapshot(core::Value(8)));m.scope.reset();check(owner.expired(),"A retired native history binding leaked its view scope");}
void scoped(){Mounted<lvui_generated::NativeScopedHistoryAot> m;take(m.scope->backend_0.push_snapshot(core::Value(1)));check(m.text("/undo")=="1"&&m.text("/canUndo")=="true","Direct scoped history import lost native observation");}
}
int main(int argc,char** argv){try{std::string name=argc>1?argv[1]:"buttons";if(name=="buttons")buttons();else if(name=="external")external();else if(name=="nonmutation")nonmutation();else if(name=="retirement")retirement();else if(name=="scoped")scoped();else throw std::runtime_error("Unknown history AOT case");std::cout<<"Native history AOT "<<name<<" passed\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
