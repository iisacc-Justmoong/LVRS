#include "Gallery.generated.h"
#include <iiGUI/Application.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace ii;
using namespace std::chrono_literals;
namespace {
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Editor final {
    inline static int created=0,destroyed=0;
    std::shared_ptr<gui::ColorPickerModel> model=std::make_shared<gui::ColorPickerModel>();
    std::shared_ptr<gui::ColorChangePort> receiver;
    explicit Editor(gui::WidgetColor initial){good(model->set_color(initial));good(model->accept());++created;}
    ~Editor(){++destroyed;}
};
using View=lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view=std::make_shared<View>();std::shared_ptr<gui::Host> host;
    std::shared_ptr<gui::ColorPickerModel> preview;std::weak_ptr<gui::ColorPickerModel> watched_preview,watched_model;std::weak_ptr<Editor> editor;
    gui::Widget popup,anchor,status,color,worker,frames;gui::WidgetColor baseline;
    bool test=false,applied=false,background=false,resized=false,closed=false,closing=false;int outside=0,opens=0,closes=0;std::uint64_t frame=0;
    void refresh(){if(preview&&!closed)good(color.set_text("Color: #"+take(preview->state()).hex));}
};
void edit(const std::shared_ptr<State>& s,std::string text){auto field=take(s->popup.popup_content("picker.hex"));good(field.focus());auto m=s->view->widgets();good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(m.dispatch_input({.kind=gui::WidgetInputKind::text,.text=std::move(text)}));good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));}
}
core::Result<void> runtime::configure_application(Application& app){
    auto s=std::make_shared<State>();s->test=app.arguments().size()==2&&app.arguments()[1]=="--self-test";if(app.arguments().size()>1&&!s->test)return core::fail(core::ErrorCode::invalid_argument,"Use --self-test or no arguments");
    std::weak_ptr<State> weak=s;auto options=View::options();options.frame_loop=&app.frame_loop();auto sender=app.event_loop().sender();
    s->host=take(gui::attach_application(app,options,{
        .opened=[weak,sender,&app]{auto s=weak.lock();if(!s)return;auto m=s->view->widgets();s->popup=take(m.find("editor"));s->anchor=take(m.find("open"));s->preview=take(s->anchor.color_model());s->watched_preview=s->preview;s->status=take(m.find("status"));s->color=take(m.find("color"));s->worker=take(m.find("worker"));s->frames=take(m.find("frames"));s->refresh();
            good(s->popup.set_popup_content([weak](gui::WidgetBuilder& builder)->core::Result<void>{auto s=weak.lock();if(!s)return core::fail(core::ErrorCode::closed,"Gallery retired");s->baseline=take(s->preview->state()).color;s->applied=false;auto editor=std::make_shared<Editor>(s->baseline);s->editor=editor;s->watched_model=editor->model;std::weak_ptr<Editor> owner=editor;
                editor->receiver=std::make_shared<gui::ColorChangePort>([weak,owner](const auto& change){auto s=weak.lock();auto editor=owner.lock();if(!s||!editor||s->closed||!change.color_changed)return;good(s->preview->set_color(take(editor->model->state()).color));s->refresh();});good(editor->model->observe(editor->receiver));good(builder.retain(editor));
                auto picker=lvrs::ColorPicker::create(builder,{},"picker",{.model=editor->model},{.accepted=[weak](gui::WidgetColor){if(auto s=weak.lock()){s->applied=true;good(s->popup.close_popup());}},.canceled=[weak]{if(auto s=weak.lock())good(s->popup.close_popup());}});if(!picker)return std::unexpected(picker.error());return {};
            }));
            check(bool(app.post([weak,sender]{(void)sender.post([weak]{if(auto s=weak.lock();s&&!s->closed){s->background=true;good(s->worker.set_text("Background result: 42 · iiSDKCore"));}});})),"Worker rejected");
            (void)take(app.frame_loop().add([s](const auto& f){s->frame=f.frame;if(f.frame%15==0)good(s->frames.set_text("Core display frames: "+std::to_string(f.frame)));if(s->test&&s->background&&s->resized&&f.frame>=3&&!s->closing){s->closing=true;good(take(s->view->widgets().find("close")).activate());}}, {.phase=runtime::FramePhase::end,.clock=runtime::TickClock::unscaled}));
            if(s->test){check(Editor::created==0&&!s->popup.popup_content(),"Closed editor was constructed eagerly");good(s->anchor.activate());auto stale=take(s->popup.popup_content("picker.hex"));auto old_id=take(stale.snapshot()).id;edit(s,"00E71F80");good(take(s->popup.popup_content("picker.apply")).activate());auto baseline=take(s->preview->state()).color;check(s->editor.expired()&&s->watched_model.expired()&&!stale.snapshot(),"First editor survived Apply");
                good(s->anchor.activate());check(take(take(s->popup.popup_content("picker.hex")).snapshot()).id!=old_id,"Content reused a retired activation identity");edit(s,"FF0000");good(take(s->popup.popup_content("picker.cancel")).activate());check(take(s->preview->state()).color==baseline&&s->editor.expired(),"Cancel did not restore preview/retire content");
                good(s->anchor.activate());edit(s,"11223380");auto r=take(take(m.find("outside")).snapshot()).bounds;good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+2,.y=r.y+2}));good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+2,.y=r.y+2}));check(s->outside==0&&!take(s->popup.snapshot()).opened&&take(s->preview->state()).color==baseline&&s->editor.expired(),"Modal outside click reached background or leaked edit");
                good(s->anchor.activate());check(!take(s->host->capture_canvas()).rgba.empty(),"Activation content did not render");good(s->host->set_content_size({820,720}));check(Editor::created==4,"Resize reconstructed editor");good(s->popup.close_popup());check(Editor::created==4&&Editor::destroyed==4&&s->opens==4&&s->closes==4,"Activations did not retire once");check(bool(sender.after(5s,[]{throw std::runtime_error("Activation gallery completion watchdog");})),"Watchdog rejected");}
        },.resized=[weak](gui::Size size){if(auto s=weak.lock())s->resized=s->resized||size==gui::Size{820,720};},
        .closed=[weak]{if(auto s=weak.lock()){s->closed=true;s->preview.reset();check(ActivationBackend::destroyed==0,"Parent scope retired before Core detached view");}}
    }));
    auto verify=app.attach({"verification",[](Application&)->core::Result<void>{return {};},[s]()->core::Result<void>{check(ActivationBackend::created==1&&ActivationBackend::destroyed==1&&s->watched_preview.expired()&&s->editor.expired(),"Core did not retire parent/content models");if(s->test){check(s->closed&&s->background&&s->resized&&s->frame>=3,"Activation milestones incomplete");std::cout<<"LVRS_NATIVE_ACTIVATION_GALLERY_OK content=4/4 parent=1/1 stale_handles_closed=true modal_input_blocked=true background=42 frames="<<s->frame<<'\n';}return {};}});if(!verify)return verify;
    return app.attach(gui::view_attachment("activation",s->host,s->view,std::function<void(View::Scope&)>{[weak,&app](View::Scope& scope){
        scope.backend_0.on_open=[weak]{if(auto s=weak.lock();s&&!s->closed){good(s->anchor.focus());auto r=take(s->anchor.snapshot());good(s->popup.open_for(s->anchor,0,r.size.height+8));}};
        scope.backend_0.on_opened=[weak]{if(auto s=weak.lock()){++s->opens;good(s->status.set_text("Content created: "+std::to_string(Editor::created)+" · retired: "+std::to_string(Editor::destroyed)));}};
        scope.backend_0.on_closed=[weak]{if(auto s=weak.lock();s&&!s->closed){++s->closes;check(s->editor.expired()&&!s->popup.popup_content(),"Closed callback observed owned content");if(!s->applied)good(s->preview->set_color(s->baseline));s->refresh();good(s->status.set_text("Content created: "+std::to_string(Editor::created)+" · retired: "+std::to_string(Editor::destroyed)));}};
        scope.backend_0.on_outside=[weak]{if(auto s=weak.lock()){++s->outside;good(s->status.set_text("Background action: "+std::to_string(s->outside)));}};scope.backend_0.on_close=[&app]{good(app.quit());};
    }}));
}
