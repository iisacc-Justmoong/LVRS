#include <LVRS/ProgressBar.h>
#include <LVRS/StateModel.h>
#include <iiGUI/WidgetState.h>
#include <iiSDKCore/Animation/Timeline.h>
#include <cmath>
#include <iostream>
using namespace ii;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void good(core::Result<void> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}}
template<class T>T take(core::Result<T> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}return std::move(*value);}
struct Mounted final {
    runtime::EventLoop events;
    runtime::FrameLoop frames{events};
    std::shared_ptr<lvrs::Motion> motion=std::make_shared<lvrs::Motion>();
    std::shared_ptr<gui::detail::WidgetState> tree;
    gui::Widget host;lvrs::ProgressBar bar;
    explicit Mounted(lvrs::ProgressBarOptions options={},bool animate=false){
        gui::detail::WidgetFrameHooks hooks;if(animate){hooks.frames=[this]()->core::Result<runtime::FrameLoop*>{return &frames;};hooks.preferences=motion;}
        tree=std::make_shared<gui::detail::WidgetState>([](runtime::Task task){task();},gui::WidgetAppearance{},std::move(hooks));
        host=take(tree->content_host({},"root",{300,200}));options.motion_enabled=animate;
        good(host.set_content([&](gui::WidgetBuilder& b){auto parent=take(b.vstack({},"layout",{.spacing=0,.alignment=gui::StackAlignment::leading}));bar=take(lvrs::ProgressBar::create(b,parent,"progress",options));return core::Result<void>{};}));
    }
    ~Mounted(){tree->close();}
};
void defaults(){Mounted m;auto state=take(m.bar.state());auto o=take(m.bar.options());check(state.bar_height==6&&state.implicit_size==gui::Size{100,6}&&o.corner_radius==100&&state.progress.normalized_current==0,"ProgressBar defaults differ");}
void values(){Mounted m;auto o=take(m.bar.options());o.values={10,110,30,80};good(m.bar.set_options(o));auto s=take(m.bar.state());check(s.progress.normalized_start==.2&&s.progress.normalized_current==.7&&s.displayed_start==.2&&std::abs(s.displayed_progress-.5)<1e-12,"ProgressBar lost model normalization");o.values={100,0,80,20};good(m.bar.set_options(o));check(take(m.bar.state()).progress.normalized_current==.8,"ProgressBar rejected a descending range");}
void source(){auto source=std::make_shared<lvrs::StateModel>();take(source->set_value("currentValue",core::Value(75)));Mounted m({.state_model=source});check(take(m.bar.state()).using_state_model&&take(m.bar.state()).displayed_progress==.75,"ProgressBar did not consume StateSource");take(source->set_value("currentValue",core::Value(25)));check(take(m.bar.state()).displayed_progress==.25,"ProgressBar source update did not repaint state");source.reset();check(!take(m.bar.state()).using_state_model&&take(m.bar.state()).displayed_progress==0,"ProgressBar retained a destroyed state source");}
void size(){Mounted m;auto o=take(m.bar.options());o.size=lvrs::ProgressSize::regular;o.regular_height=4;o.large_height=9;good(m.bar.set_options(o));check(take(m.bar.state()).bar_height==4&&take(m.bar.root().snapshot()).size.height==4,"ProgressBar regular height differs");o.height=12;good(m.bar.set_options(o));check(take(m.bar.state()).bar_height==4&&take(m.bar.root().snapshot()).size.height==12,"Explicit size replaced the implicit bar-height contract");}
void identity(){Mounted m;auto id=take(m.bar.root().snapshot()).id;auto model=take(m.bar.model());auto o=take(m.bar.options());o.width=240;o.fill_color=gui::WidgetColor{1,0,0,1};good(m.bar.set_options(o));check(take(m.bar.root().snapshot()).id==id&&take(m.bar.model())==model&&take(m.bar.state()).implicit_size.width==100,"ProgressBar update rebuilt the widget or model");}
void invalid(){Mounted m;auto before=take(m.bar.options());auto o=before;o.width=-1;check(!m.bar.set_options(o)&&take(m.bar.options()).width==before.width,"Invalid ProgressBar options changed a live component");}
void manual(){Mounted m;auto o=take(m.bar.options());o.values.current=75;o.displayed_start=.1;o.displayed_progress=.2;good(m.bar.set_options(o));auto s=take(m.bar.state());check(s.progress.normalized_current==.75&&s.displayed_start==.1&&s.displayed_progress==.2,"Manual display values overwrote the numerical model");o.displayed_start.reset();o.displayed_progress.reset();good(m.bar.set_options(o));check(take(m.bar.state()).displayed_progress==.75,"ProgressBar did not restore automatic display binding");}
void retirement(){Mounted m;auto old=m.bar;good(m.host.clear_content());check(!old.state()&&!old.set_options({}),"ProgressBar kept a retired component alive");}
void paint(){Mounted m;for(auto size:{gui::Size{240,6},gui::Size{.25,.5}}){auto native=take(m.bar.drawing(size));gui::CanvasFrame frame{.viewport={0,0,640,480},.overlays={{.panels=native->panels}}};check(gui::valid_canvas(frame),"Default rounded ProgressBar produced invalid native geometry");check(native->panels.front().radius<=std::min(size.width,size.height)/2&&native->panels.back().stroke_width<=std::min(size.width,size.height)/2,"ProgressBar paint did not apply the renderer's radius/stroke bounds");}auto o=take(m.bar.options());o.values.current=50;o.width=200;o.height=8;o.shape=lvrs::ProgressShape::cylinder;good(m.bar.set_options(o));auto ink=take(m.bar.drawing({200,8}));check(ink&&ink->panels.size()==3,"ProgressBar did not supply native paint");check(ink->panels[0].bounds.width==200&&ink->panels[1].bounds.width==100&&ink->panels[1].radius==4,"ProgressBar paint ignored allocation, range or cylinder geometry");const auto& outline=ink->panels[2];check(outline.stroke_width==1&&outline.top==outline.bottom&&std::abs(outline.top.alpha-20.f/255)<1e-6,"ProgressBar outline used a fill or a gradient");check(take(m.bar.drawing({200,8}))==ink,"Unchanged ProgressBar paint was not cached");check(!m.bar.drawing({-1,8}),"ProgressBar accepted invalid paint geometry");}
void motion(){
    Mounted m({},true);auto o=take(m.bar.options());o.values.current=100;good(m.bar.set_options(o));
    check(take(m.bar.state()).animating,"ProgressBar did not admit native frames");
    // Establish a known unscaled frame timestamp before the measured retarget.
    // Scheduling between set_options() and Clock::now() is not animation time.
    const auto anchor=runtime::Clock::now()+1h;(void)m.frames.step(anchor);
    check(!take(m.bar.state()).animating&&take(m.bar.state()).displayed_progress==1,"ProgressBar did not finish the clock-anchor transition");
    o.motion_enabled=false;o.values.current=0;good(m.bar.set_options(o));
    o.motion_enabled=true;o.values.current=100;good(m.bar.set_options(o));
    (void)m.frames.step(anchor+180ms);auto mid=take(m.bar.state());
    const double expected=animation::ease(animation::Easing::out_back,.5,lvrs::MotionTokens::overshoot);
    check(mid.animating&&std::abs(mid.displayed_progress-expected)<1e-6,"ProgressBar did not follow the original OutBack curve");
    good(m.motion->set_reduced_motion(true));check(!take(m.bar.state()).animating&&take(m.bar.state()).displayed_progress==1,"Live reduced motion left a stale ProgressBar");
}
void retarget(){Mounted m({},true);auto o=take(m.bar.options());o.values.current=100;good(m.bar.set_options(o));auto now=runtime::Clock::now();(void)m.frames.step(now+120ms);const auto before=take(m.bar.state()).displayed_progress;o.values.current=20;good(m.bar.set_options(o));check(std::abs(take(m.bar.state()).displayed_progress-before)<1e-6,"Retargeting ProgressBar jumped to its old target");m.frames.set_paused(true);good(m.frames.set_time_scale(0));(void)m.frames.step(now+600ms);check(!take(m.bar.state()).animating&&take(m.bar.state()).displayed_progress==.2,"Game pause froze unscaled ProgressBar motion");}
void admission(){Mounted m;auto o=take(m.bar.options());o.motion_enabled=true;o.values.current=80;check(!m.bar.set_options(o),"ProgressBar hid an unavailable frame source");check(!take(m.bar.state()).animating&&take(m.bar.state()).displayed_progress==.8,"Frame admission failure left a frozen intermediate progress value");}
void sources(){struct Owner {lvrs::StateModel state;};auto owner=std::make_shared<Owner>();auto source=lvrs::progress_source(owner,owner->state);check(source.lock().get()==&owner->state,"Scoped StateSource was copied");owner.reset();check(source.expired(),"ProgressBar source retained its scope");check(lvrs::progress_size(1)==lvrs::ProgressSize::regular&&lvrs::progress_shape(0)==lvrs::ProgressShape::round_rect,"Native enum conversion changed original constants");bool rejected=false;try{(void)lvrs::progress_size(2);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid dynamic ProgressBar enum was accepted");}
}
int main(int argc,char** argv){try{std::string name=argc>1?argv[1]:"defaults";if(name=="defaults")defaults();else if(name=="values")values();else if(name=="source")source();else if(name=="size")size();else if(name=="identity")identity();else if(name=="invalid")invalid();else if(name=="manual")manual();else if(name=="retirement")retirement();else if(name=="paint")paint();else if(name=="motion")motion();else if(name=="retarget")retarget();else if(name=="admission")admission();else if(name=="sources")sources();else throw std::runtime_error("Unknown progress bar case");std::cout<<"Native ProgressBar "<<name<<" passed\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
