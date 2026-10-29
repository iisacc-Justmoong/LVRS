#include <LVRS/ProgressBar.h>
#include <LVRS/Theme.h>
#include <LVRS/SurfaceDetail.h>
#include <iiSDKCore/Animation/Timeline.h>
#include <cmath>
namespace lvrs {
using namespace ii;
namespace {
struct Failure final{core::Error error;};
template<class T>T take(core::Result<T> value){if(!value)throw Failure{value.error()};return std::move(*value);}
void good(core::Result<void> value){if(!value)throw Failure{value.error()};}
template<class T,class F>core::Result<T> protect(F action){try{return action();}catch(const Failure& e){return std::unexpected(e.error);}catch(...){return core::fail(core::ErrorCode::failed,"ProgressBar operation failed",std::current_exception());}}
bool metric(double value){return std::isfinite(value)&&value>=0&&value<=8192;}
bool valid(const ProgressBarOptions& o){
    if(unsigned(o.size)>1||unsigned(o.shape)>1)return false;
    for(auto value:{o.width,o.large_height,o.regular_height,o.corner_radius})if(!metric(value))return false;
    if(o.height&&!metric(*o.height))return false;
    for(auto value:{o.displayed_start,o.displayed_progress})if(value&&!std::isfinite(*value))return false;
    return surface_detail::color(o.track_color)&&(!o.fill_color||surface_detail::color(*o.fill_color));
}
double bar_height(const ProgressBarOptions& o){return o.size==ProgressSize::regular?o.regular_height:o.large_height;}
double radius(ProgressShape shape,double r,double width,double height){const auto limit=std::max(0.,std::min(width,height)/2.);return shape==ProgressShape::cylinder?limit:std::min(r,limit);}
struct Display final {double start=0,progress=0;bool operator==(const Display&)const=default;};
}
struct ProgressBar::Impl final:std::enable_shared_from_this<Impl> {
    gui::Widget root;ProgressBarOptions options;gui::WidgetColor accent;
    std::shared_ptr<ProgressModel> model=std::make_shared<ProgressModel>();
    std::shared_ptr<ProgressChangePort> model_receiver;
    std::shared_ptr<Motion> defaults,motion;
    std::shared_ptr<gui::MotionChangePort> motion_receiver;
    gui::WidgetFrameHandler frame_handler;
    ProgressSnapshot computed;
    Display displayed,from,target;
    runtime::Clock::time_point start{},now_hint{};
    runtime::Clock::duration duration{};
    bool animating=false,committing=false,model_notified=false,retired=false;
    std::uint64_t revision=0,paint_revision=UINT64_MAX;
    std::optional<gui::WidgetPaintState> paint_state;
    std::shared_ptr<const gui::WidgetDrawing> drawing;
    void ready(){if(retired)throw Failure{{core::ErrorCode::closed,"ProgressBar generation retired"}};take(root.snapshot());}
    runtime::Clock::duration effective_duration()const{return options.motion_enabled?take(motion->duration(MotionTokens::release)):runtime::Clock::duration{};}
    void advance(runtime::Clock::time_point now){
        now_hint=std::max(now_hint,now);if(!animating)return;
        const auto elapsed=now_hint-start;const bool done=duration<=runtime::Clock::duration{}||elapsed>=duration;
        const double t=done?1.:std::max(0.,std::chrono::duration<double>(elapsed).count()/std::chrono::duration<double>(duration).count());
        const auto eased=animation::ease(animation::Easing::out_back,t,MotionTokens::overshoot);
        displayed=done?target:Display{from.start+(target.start-from.start)*eased,from.progress+(target.progress-from.progress)*eased};animating=!done;++revision;
    }
    core::Result<void> configure(){
        if(auto r=root.set_extent({options.width,options.height.value_or(bar_height(options))});!r)return r;
        return root.request_repaint();
    }
    core::Result<void> update_target(bool initial=false){
        return protect<void>([&]()->core::Result<void>{ready();const auto now=std::max(runtime::Clock::now(),now_hint);advance(now);
            computed=model->snapshot();const Display next{options.displayed_start.value_or(computed.fill_start),options.displayed_progress.value_or(computed.fill_progress)};
            const auto next_duration=effective_duration();
            if(initial){target=from=displayed=next;animating=false;}
            else if(next!=target){from=displayed;target=next;start=now;duration=next_duration;animating=duration>runtime::Clock::duration{}&&from!=target;if(!animating)displayed=target;}
            else if(next_duration<=runtime::Clock::duration{}){displayed=target;animating=false;}
            else if(animating&&duration!=next_duration){from=displayed;start=now;duration=next_duration;}
            ++revision;
            if(animating){if(auto r=root.request_frames();!r){displayed=target;animating=false;(void)root.set_frame_handler(frame_handler,false);(void)root.request_repaint();return r;}}
            else if(auto r=root.set_frame_handler(frame_handler,false);!r)return r;
            return root.request_repaint();
        });
    }
    std::shared_ptr<gui::MotionChangePort> prepare_motion(const std::shared_ptr<Motion>& selected){
        if(selected==motion)return motion_receiver;
        take(selected->state());
        auto receiver=std::make_shared<gui::MotionChangePort>([weak=weak_from_this(),identity=selected.get()](const auto&){if(auto p=weak.lock();p&&!p->retired&&p->motion.get()==identity){
            good(protect<void>([&]()->core::Result<void>{const auto now=std::max(runtime::Clock::now(),p->now_hint);p->advance(now);const auto duration=p->effective_duration();
                if(duration<=runtime::Clock::duration{}){p->displayed=p->target;p->animating=false;good(p->root.set_frame_handler(p->frame_handler,false));}
                else if(p->animating){p->from=p->displayed;p->start=now;p->duration=duration;good(p->root.request_frames());}
                ++p->revision;return p->root.request_repaint();}));
        }});
        good(selected->observe(receiver));return receiver;
    }
    core::Result<std::shared_ptr<const gui::WidgetDrawing>> paint(const gui::WidgetPaintState& state){
        if(paint_state==state&&paint_revision==revision)return drawing;
        auto ink=std::make_shared<gui::WidgetDrawing>();const auto width=std::max(0.,state.size.width),height=std::max(0.,state.size.height);
        if(width>0&&height>0){
            const gui::Rect track{0,0,float(width),float(height)};const auto track_color=surface_detail::linear(options.track_color);
            ink->panels.push_back({track,track_color,track_color,float(radius(options.shape,options.corner_radius,width,height))});
            const double x=width*std::clamp(displayed.start,0.,1.);const auto fill_width=std::max(0.,std::min(width-x,width*displayed.progress));
            if(fill_width>0){const auto color=surface_detail::linear(options.fill_color.value_or(accent));ink->panels.push_back({{float(x),0,float(fill_width),float(height)},color,color,float(radius(options.shape,options.corner_radius,fill_width,height))});}
            const auto border=surface_detail::linear({0,0,0,20./255});ink->panels.push_back({track,border,border,float(radius(options.shape,options.corner_radius,width,height)),float(std::min(1.,std::min(width,height)/2.))});
        }
        drawing=std::move(ink);paint_state=state;paint_revision=revision;return drawing;
    }
    void retire(){retired=true;model_receiver.reset();motion_receiver.reset();}
};
core::Result<ProgressBar> ProgressBar::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,ProgressBarOptions options){return protect<ProgressBar>([&]()->core::Result<ProgressBar>{
    if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid ProgressBar options");
    auto p=std::make_shared<Impl>();p->options=std::move(options);p->accent=Theme::accent_color(Accent::blue);p->defaults=take(b.motion_preferences());p->motion=p->options.motion?p->options.motion:p->defaults;
    take(p->model->set_configuration(p->options.values,p->options.state_keys,p->options.state_model.lock()));
    p->root=take(b.absolute_stack(parent,id,{p->options.width,p->options.height.value_or(bar_height(p->options))}));std::weak_ptr<Impl> weak=p;
    p->frame_handler=[weak](runtime::Clock::time_point now)->core::Result<bool>{auto p=weak.lock();if(!p||p->retired)return false;p->advance(now);if(auto r=p->root.request_repaint();!r)return std::unexpected(r.error());return p->animating;};
    good(p->root.set_frame_handler(p->frame_handler,false));
    auto motion=p->motion;p->motion.reset();p->motion_receiver=p->prepare_motion(motion);p->motion=std::move(motion);
    p->model_receiver=std::make_shared<ProgressChangePort>([weak](const ProgressChange& change){if(change.event!=ProgressEvent::effective)return;if(auto p=weak.lock();p&&!p->retired){p->model_notified=true;if(!p->committing)good(p->update_target());}});
    good(p->model->observe(p->model_receiver));good(p->update_target(true));
    good(p->root.set_paint_handler([weak](const gui::WidgetPaintState& state)->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{if(auto p=weak.lock();p&&!p->retired)return p->paint(state);return core::fail(core::ErrorCode::closed,"ProgressBar paint owner retired");}));
    good(b.on_retire([weak]{if(auto p=weak.lock())p->retire();}));good(b.retain(p));ProgressBar result;result.root_=p->root;result.state_=p;return result;
});}
core::Result<std::shared_ptr<ProgressBar::Impl>> ProgressBar::lock()const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto p=state_.lock();if(!p||p->retired)return core::fail(core::ErrorCode::closed,"ProgressBar generation retired");return p;}
core::Result<ProgressBarOptions> ProgressBar::options()const{return protect<ProgressBarOptions>([&]()->core::Result<ProgressBarOptions>{auto p=take(lock());auto o=p->options;o.values={p->model->minimum_value(),p->model->maximum_value(),p->model->start_value(),p->model->current_value()};o.state_keys=p->model->state_keys();o.state_model=p->model->state_model();return o;});}
core::Result<ProgressBarState> ProgressBar::state()const{return protect<ProgressBarState>([&]()->core::Result<ProgressBarState>{auto p=take(lock());return ProgressBarState{.progress=p->model->snapshot(),.using_state_model=p->model->using_state_model(),.animating=p->animating,.state_revision=p->model->state_revision(),.bar_height=bar_height(p->options),.displayed_start=p->displayed.start,.displayed_progress=p->displayed.progress,.implicit_size={100,bar_height(p->options)}};});}
core::Result<std::shared_ptr<const gui::WidgetDrawing>> ProgressBar::drawing(gui::Size size)const{return protect<std::shared_ptr<const gui::WidgetDrawing>>([&]()->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{auto p=take(lock());if(!metric(size.width)||!metric(size.height))return core::fail(core::ErrorCode::invalid_argument,"Invalid ProgressBar drawing size");return p->paint({.size=size});});}
core::Result<std::shared_ptr<ProgressModel>> ProgressBar::model()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->model;}
core::Result<void> ProgressBar::set_options(ProgressBarOptions options)const{return protect<void>([&]()->core::Result<void>{
    auto p=take(lock());if(p->committing)return core::fail(core::ErrorCode::busy,"ProgressBar options are committing");if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid ProgressBar options");
    struct Guard final{bool& value;explicit Guard(bool& v):value(v){value=true;}~Guard(){value=false;}}guard{p->committing};
    auto motion=options.motion?options.motion:p->defaults;take(motion->duration(MotionTokens::release));auto receiver=p->prepare_motion(motion);p->model_notified=false;
    auto configured=p->model->set_configuration(options.values,options.state_keys,options.state_model.lock());if(!configured&&!p->model_notified)return std::unexpected(configured.error());
    p->options=std::move(options);p->motion=std::move(motion);p->motion_receiver=std::move(receiver);
    auto layout=p->configure();if(!layout)return layout;auto updated=p->update_target();if(!updated)return updated;if(!configured)return std::unexpected(configured.error());return {};
});}
core::Result<double> ProgressBar::state_number(std::string_view key,double fallback)const{return protect<double>([&]()->core::Result<double>{auto p=take(lock());return p->model->state_number(key,fallback);});}
core::Result<double> ProgressBar::normalized_value(double value)const{return protect<double>([&]()->core::Result<double>{auto p=take(lock());return p->model->normalized_value(value);});}
core::Result<double> ProgressBar::resolved_radius(double width,double height)const{return protect<double>([&]()->core::Result<double>{auto p=take(lock());return p->model->radius_for(int(p->options.shape),p->options.corner_radius,width,height);});}
}
