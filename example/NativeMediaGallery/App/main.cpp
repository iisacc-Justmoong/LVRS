#include "Media.generated.h"
#include <iiGUI/Application.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
using View=lvui_generated::Media;
struct State final {std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;std::shared_ptr<MediaModel> model;bool test=false,closed=false,background=false,resized=false,updated=false;std::uint64_t frames=0;gui::Widget widget(std::string_view id){return take(view->widgets().find(id));}};
}
core::Result<void> runtime::configure_application(Application& app){
 auto p=std::make_shared<State>();p->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";
 if(app.arguments().size()>1&&!p->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
 std::weak_ptr<State> weak=p;auto sender=app.event_loop().sender();auto options=View::options();options.frame_loop=&app.frame_loop();
 p->host=take(gui::attach_application(app,options,{
  .opened=[weak,sender,&app]{auto p=weak.lock();if(!p)return;
   check(bool(app.post([weak,sender]{std::vector<std::uint8_t> pixels(128*64*4);for(unsigned y=0;y<64;++y)for(unsigned x=0;x<128;++x){auto at=(y*128+x)*4;pixels[at]=std::uint8_t(x*2);pixels[at+1]=std::uint8_t(y*4);pixels[at+2]=180;pixels[at+3]=255;}auto image=take(gui::rgba_image(128,64,std::move(pixels)));(void)sender.post([weak,image]{if(auto p=weak.lock();p&&!p->closed){p->model->image.set(image);p->model->status.set("Background image: ready · iiSDKCore");p->background=true;}});})),"Background image admission failed");
   take(app.frame_loop().add([p](const auto& frame){p->frames=frame.frame;if(p->test&&p->background&&p->updated&&p->resized&&frame.frame>=3)good(p->widget("close").activate());},{.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
   if(!p->test)return;
   auto retained=p->widget("retained");good(retained.set_text("Keep this edit"));good(p->widget("file").activate());good(p->widget("update").activate());good(p->widget("folder/menu").activate());good(p->widget("folder/action").activate());
   check(p->model->selected.get()&&p->model->menu_events==1&&p->model->action_events==1&&take(retained.snapshot()).text=="Keep this edit"&&MediaBackend::created==1,"Media update rebuilt backend/child or lost independent actions");
   p->model->preview.set(std::make_shared<const gui::WidgetFactory>([](auto& b)->core::Result<void>{auto label=b.label({},"customPreview",{.text="Reactive native factory"});if(!label)return std::unexpected(label.error());return {};}));
   p->model->icon.set(std::make_shared<const gui::WidgetFactory>([](auto& b)->core::Result<void>{auto label=b.label({},"customIcon",{.text="M"});if(!label)return std::unexpected(label.error());return {};}));
   check(p->widget("custom").snapshot().has_value()&&take(retained.snapshot()).parent_id=="inline/inline","Media retained slots lost native identity");
   take(p->host->capture_canvas());p->updated=true;good(p->host->set_content_size({1200,960}));
  },
  .resized=[weak](gui::Size size){if(auto p=weak.lock();p&&p->model){p->model->preview_width.set(std::clamp(size.width-904.,128.,320.));p->resized|=size==gui::Size{1200,960};}},
  .closed=[weak]{if(auto p=weak.lock())p->closed=true;}
 }));
 if(auto result=app.attach({"media-verification",[](Application&)->core::Result<void>{return {};},[p]()->core::Result<void>{check(MediaBackend::created==MediaBackend::retired,"Media backend outlived view retirement");if(p->test){check(p->closed&&p->background&&p->updated&&p->resized&&p->frames>=3,"Media gallery milestones incomplete");std::cout<<"LVRS_NATIVE_MEDIA_GALLERY_OK scope=1/1 frames="<<p->frames<<'\n';}return {};}});!result)return result;
 return app.attach(gui::view_attachment("media",p->host,p->view,std::function<void(View::Scope&)>{[weak,&app](auto& scope){if(auto p=weak.lock()){p->model=scope.backend_0.model;scope.backend_0.on_close=[&app]{good(app.quit());};}}}));
}
