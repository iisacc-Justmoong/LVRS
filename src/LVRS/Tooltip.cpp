#include "Tooltip.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <iiSDKCore/Runtime/TaskScope.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <thread>
namespace lvrs {
using namespace ii;
namespace {
struct Failure final{core::Error error;};
template<class T>T take(core::Result<T> r){if(!r)throw Failure{r.error()};return std::move(*r);}
void good(core::Result<void> r){if(!r)throw Failure{r.error()};}
template<class T,class F>core::Result<T> protect(F&& f){try{return f();}catch(const Failure& e){return std::unexpected(e.error);}catch(const std::bad_alloc&){return core::fail(core::ErrorCode::full,"Tooltip allocation failed");}catch(...){return core::fail(core::ErrorCode::failed,"Tooltip operation failed",std::current_exception());}}
bool metric(double n){return std::isfinite(n)&&n>=0&&n<=8192;}
bool text(std::string_view s){return s.size()<=65536&&core::valid_utf8(s)&&s.find('\0')==s.npos;}
bool valid(const TooltipOptions& o){
    for(auto n:{o.maximum_width,o.maximum_height,o.content_padding,o.edge_margin,o.corner_radius,o.tail_length,o.tail_width})if(!metric(n))return false;
    for(auto t:{o.delay,o.timeout,o.hide_delay,o.animation_duration})if(t<decltype(t)::zero()||t>std::chrono::hours(24))return false;
    for(auto c:{o.surface_color,o.border_color,o.primary_color})if(c)for(auto n:{c->red,c->green,c->blue,c->alpha})if(!std::isfinite(n)||n<0||n>1)return false;
    if(o.anchor_point&&(!std::isfinite(o.anchor_point->x)||!std::isfinite(o.anchor_point->y)||std::abs(o.anchor_point->x)>32767||std::abs(o.anchor_point->y)>32767))return false;
    if(o.available_rect){auto r=*o.available_rect;if(!std::isfinite(r.x)||!std::isfinite(r.y)||std::abs(r.x)>32767||std::abs(r.y)>32767||!std::isfinite(r.width)||!std::isfinite(r.height)||r.width<0||r.height<0||r.width>32767||r.height>32767)return false;}
    return text(o.text)&&text(o.accessibility_name)&&unsigned(o.preferred_placement)<=4&&unsigned(o.backdrop)<=2&&o.maximum_width>o.content_padding*2&&o.maximum_height>o.content_padding*2&&std::isfinite(o.surface_opacity)&&o.surface_opacity>=0&&o.surface_opacity<=1;
}
gui::Rect intersect(gui::Rect a,gui::Rect b){const auto r=std::min(a.x+a.width,b.x+b.width),d=std::min(a.y+a.height,b.y+b.height);a.x=std::max(a.x,b.x);a.y=std::max(a.y,b.y);a.width=std::max(0.f,r-a.x);a.height=std::max(0.f,d-a.y);return a;}
template<class T>bool same_owner(const std::weak_ptr<T>& a,const std::weak_ptr<T>& b){return !a.owner_before(b)&&!b.owner_before(a);}
}
struct Tooltip::Impl final:std::enable_shared_from_this<Impl>{
    enum Timer:std::size_t{show,hide,expiry,guard};
    TooltipOptions options;TooltipEvents events;TooltipState value;
    gui::Widget root,chrome,viewport,content,label;
    PanelMaterial material;
    std::shared_ptr<gui::WidgetInteractionPort> target_port,root_port,content_port;
    gui::WidgetInteraction target_state,root_state;
    runtime::EventLoop::Sender loop;
    runtime::TaskScope lifetime;
    std::array<runtime::TaskToken,4> timers;
    std::array<std::uint64_t,4> generations{};
    std::array<bool,4> scheduled{};
    std::thread::id owner=std::this_thread::get_id();
    std::atomic_bool wrong_dispatch=false;
    bool custom=false,refreshing=false,suppressed=false,saw_target=false,style_dirty=true;
    ~Impl(){for(auto& token:timers)(void)token.cancel();lifetime.request_close();(void)lifetime.close();for(auto p:{target_port,root_port,content_port})if(p)(void)p->close();}
    void ready(){if(owner!=std::this_thread::get_id()||wrong_dispatch.load())throw Failure{{core::ErrorCode::wrong_executor,"Tooltip requires its owner thread"}};take(root.placement());}
    void cancel(Timer t){(void)timers[t].cancel();timers[t]={};scheduled[t]=false;++generations[t];if(t==show)value.pending=false;}
    void schedule(Timer t,std::chrono::milliseconds duration){
        cancel(t);if(duration<=duration.zero()){fire(t);return;}
        const auto generation=generations[t];auto submitted=lifetime.at(loop,runtime::Clock::now()+duration,[weak=weak_from_this(),t,generation]{if(auto p=weak.lock()){
            if(p->owner!=std::this_thread::get_id()){p->wrong_dispatch.store(true);return;}
            if(p->generations[t]!=generation||!p->root.placement())return;
            p->scheduled[t]=false;p->timers[t]={};auto result=protect<void>([&]()->core::Result<void>{p->fire(t);return {};});if(!result)p->value.last_error=result.error().message;
        }});
        if(!submitted)throw Failure{{core::ErrorCode::full,"Core rejected the owned Tooltip deadline"}};
        timers[t]=submitted.token;scheduled[t]=true;if(t==show)value.pending=true;
    }
    TooltipPoint anchor()const{
        const auto point=options.anchor_point.value_or(TooltipPoint{target_state.bounds.width/2,target_state.bounds.height/2});
        return options.target.is_null()?options.anchor_point.value_or(TooltipPoint{}):TooltipPoint{target_state.bounds.x+point.x,target_state.bounds.y+point.y};
    }
    bool wanted()const{const auto point=anchor();return !options.target.is_null()&&target_state.contains(point.x,point.y)&&(target_state.hovered||target_state.pressed||target_state.focused);}
    gui::PopoverOptions surface()const{
        auto transition=modal_transition(options.motion,options.motion_enabled);transition.enter_duration=options.animation_duration;transition.exit_duration=std::min(options.animation_duration,MotionTokens::exit);
        transition.origin=value.layout.placement==TooltipPlacement::above?gui::Size{.5,1}:value.layout.placement==TooltipPlacement::below?gui::Size{.5,0}:value.layout.placement==TooltipPlacement::left_side?gui::Size{1,.5}:gui::Size{0,.5};
        auto input=value.layout.body;input.x-=value.layout.bounds.x;input.y-=value.layout.bounds.y;
        return {.width=value.layout.bounds.width,.height=value.layout.bounds.height,.padding=0,.margins=0,.corner_radius=0,.surface_opacity=0,.surface_color=gui::WidgetColor{0,0,0,0},.focus_on_open=false,.accessibility_name=options.accessibility_name.empty()?options.text:options.accessibility_name,.opaque_base=false,.transition=transition,.bounds=value.layout.bounds,.input_bounds=input,.preserve_input=true};
    }
    bool refresh(){
        if(refreshing)return true;ready();refreshing=true;struct Reset{bool& b;~Reset(){b=false;}}reset{refreshing};
        root_state=take(root.interaction());
        if(!options.target.is_null()){
            auto current=options.target.interaction();if(!current){cancel(show);if(value.opened)good(root.close_popup());return false;}target_state=*current;
            auto p=anchor();if(!target_state.contains(p.x,p.y)){cancel(show);if(value.opened)good(root.close_popup());return false;}
        }
        const gui::Rect window{0,0,float(root_state.surface.width),float(root_state.surface.height)};
        const auto available=options.available_rect?intersect(*options.available_rect,window):window;
        auto content_size=take(content.measure(std::max(1.,options.maximum_width-options.content_padding*2)));
        auto next=take(layout_tooltip({anchor(),available,content_size,options.preferred_placement,options.maximum_width,options.maximum_height,options.content_padding,options.edge_margin,options.tail_length}));
        if(next.body.width<=options.content_padding*2||next.body.height<=options.content_padding*2||next.bounds.width<1||next.bounds.height<1||next.bounds.width>8192||next.bounds.height>8192){cancel(show);if(value.opened)good(root.close_popup());value.last_error="Tooltip has no drawable content region";return false;}
        // Placement can choose a narrower side. Re-measure real native wrapping
        // at that width once, rather than estimating rows from character counts.
        content_size=take(content.measure(next.body.width-options.content_padding*2));
        next=take(layout_tooltip({anchor(),available,content_size,options.preferred_placement,options.maximum_width,options.maximum_height,options.content_padding,options.edge_margin,options.tail_length}));
        const auto changed=next!=value.layout;value.layout=next;value.content_size=content_size;
        if(changed||style_dirty){
        auto material_options=take(material.options());material_options.width=next.bounds.width;material_options.height=next.bounds.height;material_options.outline_path=take(tooltip_outline(next,options.corner_radius,options.tail_width));
        material_options.color=options.surface_color;material_options.border_color=options.border_color;material_options.primary_color=options.primary_color;material_options.tint_opacity=options.surface_opacity;material_options.backdrop=options.backdrop;material_options.radius=options.corner_radius;material_options.motion_enabled=options.motion_enabled;material_options.motion=options.motion;
        good(material.set_options(std::move(material_options)));good(material.root().set_frame({0,0,next.bounds.width,next.bounds.height}));
        const auto body=next.body;good(viewport.set_frame({float(body.x-next.bounds.x+options.content_padding),float(body.y-next.bounds.y+options.content_padding),float(body.width-options.content_padding*2),float(body.height-options.content_padding*2)}));
        good(viewport.set_scroll_options({.width=body.width-options.content_padding*2,.height=body.height-options.content_padding*2,.enabled=options.scroll_content}));
        good(root.set_popover_options(surface()));style_dirty=false;
        }return true;
    }
    void synchronize(){
        if(!options.automatic)return;const bool need=wanted();
        if(scheduled[guard]){if(need)saw_target=true;else if(saw_target)suppressed=false;return;}
        if(need||(value.opened&&root_state.hovered)){
            cancel(hide);if(need&&!value.automatic_latched&&!suppressed){value.automatic_latched=true;request_open();}
        }else{
            value.automatic_latched=false;suppressed=false;
            if(value.pending)cancel(show);else if(value.opened&&!scheduled[hide])schedule(hide,options.hide_delay);
        }
    }
    void request_open(){if(value.opened||value.pending)return;if(!refresh())return;value.last_error.clear();schedule(show,options.delay);}
    void dismiss(){
        cancel(show);cancel(hide);cancel(expiry);
        if(value.opened)good(root.close_popup());
        else if(options.automatic){suppressed=true;value.automatic_latched=true;saw_target=false;schedule(guard,std::max(std::chrono::milliseconds(32),options.hide_delay));}
    }
    void fire(Timer t){
        if(t==show){value.pending=false;if(!refresh())return;good(viewport.set_scroll_offset(0));good(root.open_at(0,0));}
        else if(t==guard){if(!wanted())suppressed=false;synchronize();}
        else dismiss();
    }
    void opened(){value.opened=true;value.pending=false;if(!refresh())return;if(options.timeout>options.timeout.zero())schedule(expiry,options.timeout);if(events.opened)events.opened();}
    void closed(){
        value.opened=false;cancel(show);cancel(hide);cancel(expiry);
        if(options.automatic){suppressed=true;value.automatic_latched=true;saw_target=false;schedule(guard,std::max(std::chrono::milliseconds(32),options.hide_delay));}
        if(events.closed)events.closed();
    }
    template<class F>void receive(F&& action){auto result=protect<void>([&]()->core::Result<void>{action();return {};});if(!result)value.last_error=result.error().message;}
    void bind_target(gui::Widget target){
        if(target.is_null()){if(target_port)(void)target_port->close();target_port.reset();target_state={};return;}
        const auto source=take(target.placement()),destination=take(root.placement());
        if(!same_owner(source.domain,destination.domain))throw Failure{{core::ErrorCode::invalid_argument,"Tooltip target must share its native widget domain"}};
        auto weak=weak_from_this();auto receiver=std::make_shared<gui::WidgetInteractionPort>([weak](const auto& state){if(auto p=weak.lock()){p->target_state=state;p->receive([&]{if(!state.alive){p->cancel(show);if(p->value.opened)good(p->root.close_popup());return;}if(p->value.opened||p->value.pending)p->refresh();p->synchronize();});}});
        good(target.observe_interaction(receiver));if(target_port)(void)target_port->close();target_port=std::move(receiver);target_state=take(target.interaction());
    }
    bool same_target(gui::Widget target)const{
        if(target.is_null()||options.target.is_null())return target.is_null()&&options.target.is_null();
        auto before=options.target.placement(),after=target.placement();return before&&after&&same_owner(before->scope,after->scope)&&before->parent==after->parent&&before->sibling==after->sibling;
    }
    void apply(TooltipOptions next){
        if(!valid(next))throw Failure{{core::ErrorCode::invalid_argument,"Invalid Tooltip options"}};
        const bool retarget=!same_target(next.target),automatic_changed=options.automatic!=next.automatic;
        const bool pending=value.pending,delay_changed=options.delay!=next.delay,timeout_changed=options.timeout!=next.timeout;
        if(retarget)bind_target(next.target); // Validate before disturbing the old bubble.
        if(retarget){cancel(show);cancel(hide);cancel(expiry);if(value.opened)good(root.close_popup());cancel(guard);}
        else if(delay_changed&&pending)cancel(show);
        options=std::move(next);style_dirty=true;
        if(retarget||automatic_changed){suppressed=false;value.automatic_latched=false;cancel(guard);}
        if(!custom)good(label.set_text(options.text));
        if(automatic_changed&&!options.automatic){cancel(show);cancel(hide);if(value.opened)good(root.close_popup());}
        good(root.set_accessibility_name(options.accessibility_name.empty()?options.text:options.accessibility_name));
        refresh();
        if(value.opened&&timeout_changed){cancel(expiry);if(options.timeout>options.timeout.zero())schedule(expiry,options.timeout);}
        if(pending&&!value.pending&&!retarget&&!(automatic_changed&&!options.automatic))request_open();
        synchronize();
    }
};
core::Result<Tooltip> Tooltip::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,TooltipOptions options,TooltipEvents events,TooltipContentFactory content){
    return protect<Tooltip>([&]()->core::Result<Tooltip>{
        if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid Tooltip options");
        auto p=std::make_shared<Impl>();p->events=std::move(events);p->options=options;p->custom=bool(content);p->loop=take(b.event_loop()).get().sender();auto weak=std::weak_ptr<Impl>(p);
        p->value.layout={TooltipPlacement::above,{},{0,0,2,2},{0,0,2,2}};
        p->root=take(b.popover(parent,id,p->surface(),{.opened=[weak]{if(auto s=weak.lock())s->opened();},.closed=[weak]{if(auto s=weak.lock())s->closed();}}));
        const auto prefix=std::string(id);p->chrome=take(b.absolute_stack(p->root,prefix+".chrome",{2,2}));
        p->material=take(PanelMaterial::create(b,p->chrome,prefix+".surface",{.density=MaterialDensity::glass25,.width=2,.height=2}));
        p->viewport=take(b.scroll_view(p->chrome,prefix+".viewport",{.width=1,.height=1}));
        p->content=take(b.vstack(p->viewport,prefix+".content",{.spacing=0,.alignment=gui::StackAlignment::leading,.intrinsic_width=true}));
        if(content)good(content(b,p->content));else p->label=take(b.label(p->content,prefix+".label",{.text=options.text,.font_size=13,.selectable=false,.role=gui::TextRole::body,.line_height=16}));
        good(p->root.set_input_handler([](const auto&)->core::Result<bool>{return false;}));
        p->root_port=std::make_shared<gui::WidgetInteractionPort>([weak](const auto& state){if(auto s=weak.lock()){const auto size=s->root_state.surface;s->root_state=state;s->receive([&]{if(!state.alive){for(auto t:{Impl::show,Impl::hide,Impl::expiry,Impl::guard})s->cancel(t);return;}if(size!=state.surface&&(s->value.opened||s->value.pending))s->refresh();s->synchronize();});}});
        p->content_port=std::make_shared<gui::WidgetInteractionPort>([weak,last=gui::Size{}](const auto& state)mutable{if(auto s=weak.lock()){if(state.alive&&last!=state.implicit_size){last=state.implicit_size;s->receive([&]{if(s->value.opened||s->value.pending)s->refresh();});}}});
        good(p->root.observe_interaction(p->root_port));good(p->content.observe_interaction(p->content_port));p->bind_target(options.target);good(b.retain(p));
        Tooltip result;result.root_=p->root;result.viewport_=p->viewport;result.content_=p->content;result.state_=p;return result;
    });
}
template<class T,class F>core::Result<T> Tooltip::with_state(F&& f)const{return protect<T>([&]()->core::Result<T>{auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"Tooltip retired");p->ready();return f(*p);});}
core::Result<std::chrono::milliseconds> tooltip_milliseconds(double value){if(!std::isfinite(value)||value<0||value>86400000)return core::fail(core::ErrorCode::invalid_argument,"Tooltip milliseconds must be finite and within one day");return std::chrono::milliseconds{std::int64_t(value)};}
core::Result<TooltipState> Tooltip::state()const{return with_state<TooltipState>([](auto& p){return p.value;});}
core::Result<TooltipOptions> Tooltip::options()const{return with_state<TooltipOptions>([](auto& p){return p.options;});}
core::Result<void> Tooltip::set_options(TooltipOptions o)const{return with_state<void>([&](auto& p)->core::Result<void>{p.apply(std::move(o));return {};});}
core::Result<void> Tooltip::set_target(gui::Widget target,std::optional<TooltipPoint> point)const{return with_state<void>([&](auto& p)->core::Result<void>{auto o=p.options;o.target=target;o.anchor_point=point;p.apply(std::move(o));return {};});}
core::Result<void> Tooltip::open()const{return with_state<void>([](auto& p)->core::Result<void>{p.request_open();return {};});}
core::Result<void> Tooltip::open_at(gui::Widget target,TooltipPoint point)const{return with_state<void>([&](auto& p)->core::Result<void>{auto o=p.options;o.target=target;o.anchor_point=point;p.apply(std::move(o));p.request_open();return {};});}
core::Result<void> Tooltip::close()const{return with_state<void>([](auto& p)->core::Result<void>{p.dismiss();return {};});}
core::Result<void> Tooltip::refresh()const{return with_state<void>([](auto& p)->core::Result<void>{p.refresh();return {};});}
} // namespace lvrs
