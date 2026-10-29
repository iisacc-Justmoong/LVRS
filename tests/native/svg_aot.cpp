#include "NativeSvgAot.generated.h"
#include "NativeSvgSelectionAot.generated.h"
#include <iiGUI/WidgetState.h>
#include <algorithm>
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
    gui::Widget find(std::string_view name){for(const auto& value:take(tree->snapshots()))if((value.id==name||value.id.ends_with("/"+std::string(name))))return take(tree->find(value.id));throw std::runtime_error("Missing SVG widget");}
    std::string text(std::string_view name){return take(find(name).snapshot()).text;}
    gui::CanvasFrame frame(){return take(tree->drawing_snapshot({640,480}));}
    std::shared_ptr<const gui::RasterImage> red(){const auto f=frame();for(const auto& image:f.images)if(image.image&&image.image->width()==54&&!image.image->rgba().empty()&&image.image->rgba()[0]==255&&image.image->rgba()[3]==255)return image.image;return {};}
};
using Default=lvui_generated::NativeSvgAot;using Selected=lvui_generated::NativeSvgSelectionAot;
void defaults(){Mounted<Default> m;const auto frame=m.frame();check(m.text("vector")=="true"&&m.text("minimum")=="3"&&m.text("maximum")=="4"&&m.text("capacity")=="256"&&m.text("error").empty()&&!frame.images.empty(),"Native SVG defaults or embedded image did not reach the view");}
void pending(){Mounted<Selected> m;check(m.text("error")=="SVG request pending"&&m.scope->backend_0.source->requests==1&&!m.red(),"AOT initial asynchronous source was not coalesced or empty");}
void completion(){Mounted<Selected> m;good(m.find("complete").activate());auto image=m.red();check(image&&image==m.red()&&m.text("error").empty()&&m.text("revision")=="1"&&m.text("initial")=="0","AOT completion lost native raster identity, revision or Static snapshot");}
void failure(){Mounted<Selected> m;m.scope->backend_0.fail();check(m.text("error")=="Fixture SVG failure"&&!m.red(),"Source error did not reach AOT label");good(m.find("retarget").activate());good(m.find("complete").activate());check(m.red()!=nullptr&&m.text("error").empty(),"Retargeted SVG did not recover");}
void geometry(){Mounted<Selected> m;(void)m.frame();const auto id=take(m.find("preview").snapshot()).id;good(m.find("resize").activate());(void)m.frame();const auto after=take(m.find("preview").snapshot());check(after.id==id&&after.bounds.width==120&&after.bounds.height==72,"Image dimensions rebuilt the node or failed to update");m.scope->backend_0.width.set(-1);(void)m.frame();check(take(m.find("preview").snapshot()).bounds.width==120,"Invalid Image update replaced its valid options");m.scope->backend_0.width.set(80);(void)m.frame();check(take(m.find("preview").snapshot()).bounds.width==80,"Image options did not recover after invalid geometry");}
void batch(){Mounted<Selected> m;good(m.find("complete").activate());m.scope->backend_0.padding.set(40);m.scope->backend_0.width.set(128);m.scope->backend_0.height.set(128);const auto image=m.red();const auto frame=m.frame();check(image&&std::ranges::any_of(frame.images,[&](const auto& draw){return draw.image==image&&draw.bounds.width==48&&draw.bounds.height==48;}),"Dependent Image inputs lost the requested padding after invalid geometry");}
void identity(){Mounted<Selected> m;auto field=m.find("edit"),preview=m.find("preview");good(field.set_text("Retained"));good(field.focus());const auto nodes=take(m.tree->snapshots()).size();const auto id=take(preview.snapshot()).id;good(m.find("complete").activate());check(m.red()!=nullptr&&take(preview.snapshot()).id==id&&take(field.snapshot()).text=="Retained"&&take(field.snapshot()).focused&&take(m.tree->snapshots()).size()==nodes&&m.scope->backend_0.file_revision==0,"SVG completion rebuilt input/focus or mutated file state");}
void retirement(){Mounted<Selected> m;auto request=m.scope->backend_0.source->pending;auto old=m.find("preview");std::weak_ptr<Selected::Scope> weak=m.scope;m.tree->close();m.scope.reset();check(weak.expired()&&NativeSvgBackend::live==0&&request->cancelled&&!old.snapshot(),"SVG request, import or view outlived its activation");}
}
int main(int argc,char** argv){try{const std::string name=argc>1?argv[1]:"defaults";if(name=="defaults")defaults();else if(name=="pending")pending();else if(name=="completion")completion();else if(name=="failure")failure();else if(name=="geometry")geometry();else if(name=="batch")batch();else if(name=="identity")identity();else if(name=="retirement")retirement();else throw std::runtime_error("Unknown SVG AOT case");check(NativeSvgBackend::live==0,"SVG backend leaked");std::cout<<"Native SVG AOT "<<name<<" passed\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
