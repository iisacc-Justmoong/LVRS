#include "Materials.generated.h"
#include <iiGUI/Application.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
using View=lvui_generated::Materials;
struct State final{std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;std::shared_ptr<MaterialModel> model;bool test=false,background=false,resized=false,updated=false,closed=false;std::uint64_t frame=0;gui::Widget widget(std::string_view id){return take(view->widgets().find(id));}};
}
core::Result<void> runtime::configure_application(Application& app){
 auto p=std::make_shared<State>();p->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";if(app.arguments().size()>1&&!p->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
 std::weak_ptr<State> weak=p;auto sender=app.event_loop().sender();auto options=View::options();options.frame_loop=&app.frame_loop();
 p->host=take(gui::attach_application(app,options,{
  .opened=[weak,sender,&app]{auto p=weak.lock();if(!p)return;gui::WidgetDrawing background;for(int i=0;i<24;++i){const gui::Color color=i%2?gui::Color{.02,.08,.12,1}:gui::Color{.08,.02,.10,1};background.panels.push_back({{float(i*48),0,48,900},color,color,0});}good(p->widget("root").set_drawing(std::move(background)));
   check(bool(app.post([weak,sender]{(void)sender.post([weak]{if(auto p=weak.lock();p&&!p->closed){p->model->status.set("Background result: 42 · iiSDKCore");p->background=true;}});})),"Background admission failed");
   take(app.frame_loop().add([p](const auto& frame){p->frame=frame.frame;if(p->test&&p->background&&p->resized&&p->updated&&frame.frame>=3)good(p->widget("close").activate());},{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
   if(!p->test)return;auto editor=p->widget("editor");good(editor.set_text("Keep this edit"));good(p->widget("accent").activate());good(p->widget("density").activate());good(p->model->motion->set_reduced_motion(true));
   check(MaterialBackend::created==1&&take(editor.snapshot()).text=="Keep this edit","Material update reconstructed its backend or child");take(p->host->capture_canvas());auto info=take(p->host->graphics_info());check(info.backdrop_supported&&info.shadow_supported&&info.backdrop_blurs>0&&info.effect_image_uploads>0,"Material GPU paths or cached masks were not used");p->updated=true;good(p->host->set_content_size({1000,740}));
  },
  .resized=[weak](gui::Size size){if(auto p=weak.lock();p&&p->model){p->model->width.set(std::clamp((size.width-100)/2,240.,440.));p->resized|=size==gui::Size{1000,740};}},
  .closed=[weak]{if(auto p=weak.lock())p->closed=true;}
 }));
 if(auto r=app.attach({"material-verification",[](Application&)->core::Result<void>{return {};},[p]()->core::Result<void>{check(MaterialBackend::created==MaterialBackend::retired,"Material backend outlived its view");if(p->test){check(p->closed&&p->background&&p->updated&&p->resized&&p->frame>=3,"Material gallery milestones incomplete");std::cout<<"LVRS_NATIVE_MATERIAL_GALLERY_OK scope=1/1 frames="<<p->frame<<'\n';}return {};}});!r)return r;
 return app.attach(gui::view_attachment("materials",p->host,p->view,std::function<void(View::Scope&)>{[weak,&app](auto& scope){if(auto p=weak.lock()){p->model=scope.backend_0.model;scope.backend_0.on_close=[&app]{good(app.quit());};}}}));
}
