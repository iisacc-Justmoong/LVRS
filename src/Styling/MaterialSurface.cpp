#include <LVRS/MaterialSurface.h>
#include <LVRS/MaterialDetail.h>
#include <LVRS/SurfaceDetail.h>
#include <LVRS/Motion.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace lvrs {
using namespace ii;
namespace {
gui::WidgetColor mix(gui::WidgetColor a,gui::WidgetColor b,double t){return {a.red+(b.red-a.red)*t,a.green+(b.green-a.green)*t,a.blue+(b.blue-a.blue)*t,a.alpha+(b.alpha-a.alpha)*t};}
}
struct MaterialSurface::Impl final {
    gui::Widget root;MaterialOptions options;MaterialVariant variant=MaterialVariant::surface;gui::WidgetAppearance appearance;material_detail::Prepared prepared;
    std::shared_ptr<gui::MotionPreferences> defaults,model;std::shared_ptr<gui::MotionChangePort> receiver;gui::WidgetFrameHandler frame_callback;
    material_detail::Colors displayed,from,target;runtime::Clock::time_point start{},now_hint{};runtime::Clock::duration duration{};bool animating=false,committing=false;std::uint64_t revision=1;
    material_detail::RasterCache rasters;std::optional<gui::WidgetPaintState> paint_state;std::uint64_t paint_revision=0;std::shared_ptr<const gui::WidgetDrawing> drawing;
    ~Impl(){if(receiver)(void)receiver->close();}
    core::Result<void> bind_model(std::shared_ptr<gui::MotionPreferences> next){
        if(next==model)return {};auto state=next->state();if(!state)return std::unexpected(state.error());std::weak_ptr<Impl> weak=shared;
        auto port=std::make_shared<gui::MotionChangePort>([weak](const gui::MotionState&){if(auto p=weak.lock()){auto time=p->model->duration(MotionTokens::color);if(!time){surface_detail::require(std::unexpected(time.error()));return;}p->duration=p->options.motion_enabled?*time:runtime::Clock::duration{};p->advance(runtime::Clock::now());if(!p->animating)surface_detail::require(p->root.set_frame_handler(p->frame_callback,false));surface_detail::require(p->root.request_repaint());}});
        if(auto r=next->observe(port);!r)return r;if(receiver)(void)receiver->close();model=std::move(next);receiver=std::move(port);return {};
    }
    std::weak_ptr<Impl> shared;
    void advance(runtime::Clock::time_point now){now_hint=std::max(now_hint,now);now=now_hint;if(!animating)return;const auto elapsed=now-start;const auto t=duration<=runtime::Clock::duration{}?1.:std::clamp(std::chrono::duration<double>(elapsed).count()/std::chrono::duration<double>(duration).count(),0.,1.);const auto eased=1-std::pow(1-t,3);displayed=t>=1?target:material_detail::Colors{mix(from.tint,target.tint,eased),mix(from.primary,target.primary,eased),mix(from.base,target.base,eased)};animating=t<1;++revision;}
    core::Result<void> configure(){auto extent=prepared.state.implicit_size;if(options.width)extent.width=options.width;if(options.height)extent.height=options.height;if(auto r=root.set_extent(extent);!r)return r;if(auto r=root.set_enabled(options.enabled);!r)return r;return root.request_repaint();}
    core::Result<std::shared_ptr<const gui::WidgetDrawing>> paint(const gui::WidgetPaintState& state){if(paint_state==state&&paint_revision==revision)return drawing;auto result=material_detail::paint(options,prepared,displayed,state,rasters);if(!result)return result;drawing=*result;paint_state=state;paint_revision=revision;return drawing;}
};
core::Result<MaterialSurface> MaterialSurface::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,MaterialOptions options,MaterialContentFactory content){return create_variant(b,parent,id,MaterialVariant::surface,std::move(options),std::move(content));}
core::Result<MaterialSurface> MaterialSurface::create_variant(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,MaterialVariant variant,MaterialOptions options,MaterialContentFactory content){
    auto appearance=b.appearance();if(!appearance)return std::unexpected(appearance.error());auto prepared=material_detail::prepare(options,variant,*appearance);if(!prepared)return std::unexpected(prepared.error());auto defaults=b.motion_preferences();if(!defaults)return std::unexpected(defaults.error());auto p=std::make_shared<Impl>();p->shared=p;p->options=std::move(options);p->variant=variant;p->appearance=*appearance;p->prepared=std::move(*prepared);p->displayed=p->target=p->prepared.target;p->defaults=*defaults;std::weak_ptr<Impl> weak=p;
    auto root=b.absolute_stack(parent,id,p->prepared.state.implicit_size);if(!root)return std::unexpected(root.error());p->root=*root;
    p->frame_callback=[weak](runtime::Clock::time_point now)->core::Result<bool>{auto p=weak.lock();if(!p)return false;p->advance(now);if(auto r=p->root.request_repaint();!r)return std::unexpected(r.error());return p->animating;};
    if(auto r=root->set_frame_handler(p->frame_callback,false);!r)return std::unexpected(r.error());if(auto r=p->bind_model(p->options.motion?p->options.motion:p->defaults);!r)return std::unexpected(r.error());
    if(auto r=root->set_paint_handler([weak](const auto& state)->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{auto p=weak.lock();if(!p)return core::fail(core::ErrorCode::closed,"Material paint owner retired");return p->paint(state);});!r)return std::unexpected(r.error());
    if(auto r=b.retain(p);!r)return std::unexpected(r.error());if(auto r=p->configure();!r)return std::unexpected(r.error());if(content){try{if(auto r=content(b,*root);!r)return std::unexpected(r.error());}catch(...){return core::fail(core::ErrorCode::failed,"Material content factory failed",std::current_exception());}}
    if(!root->snapshot())return core::fail(core::ErrorCode::closed,"Material content retired its scope");MaterialSurface result;result.root_=*root;result.state_=p;return result;
}
core::Result<std::shared_ptr<MaterialSurface::Impl>> MaterialSurface::lock()const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"Material scope retired");return p;}
core::Result<MaterialOptions> MaterialSurface::options()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options;}
core::Result<MaterialState> MaterialSurface::state()const{auto p=lock();if(!p)return std::unexpected(p.error());auto s=(*p)->prepared.state;s.displayed_tint=(*p)->displayed.tint;s.displayed_primary=(*p)->displayed.primary;s.displayed_base=(*p)->displayed.base;s.animating=(*p)->animating;s.mask_rasterizations=(*p)->rasters.masks;s.gradient_rasterizations=(*p)->rasters.gradients;s.border_rasterizations=(*p)->rasters.strokes;auto root=root_.snapshot();if(!root)return std::unexpected(root.error());s.capture_requested=(*p)->options.backdrop!=MaterialBackdrop::none&&root->visible&&root->bounds.width>0&&root->bounds.height>0;return s;}
core::Result<void> MaterialSurface::set_options(MaterialOptions options)const{
    auto owner=lock();if(!owner)return std::unexpected(owner.error());auto p=*owner;if(p->committing)return core::fail(core::ErrorCode::busy,"Material options are committing");
    auto prepared=material_detail::prepare(options,p->variant,p->appearance);if(!prepared)return std::unexpected(prepared.error());auto model=options.motion?options.motion:p->defaults;auto time=model->duration(MotionTokens::color);if(!time)return std::unexpected(time.error());
    const auto now=std::max(runtime::Clock::now(),p->now_hint);p->advance(now);const bool changed=prepared->target!=p->target;const auto duration=options.motion_enabled?*time:runtime::Clock::duration{};
    const bool needs_frames=duration>runtime::Clock::duration{}&&(changed?p->displayed!=prepared->target:p->animating);
    // Reserve the shared Core source before committing new options. An exhausted
    // source must not leave a new color permanently waiting for frames.
    if(needs_frames){if(auto r=root_.request_frames();!r)return r;}
    if(auto r=p->bind_model(model);!r){if(needs_frames)(void)root_.set_frame_handler(p->frame_callback,p->animating);return r;}
    struct Guard{bool& value;explicit Guard(bool& v):value(v){value=true;}~Guard(){value=false;}}guard{p->committing};p->options=std::move(options);p->prepared=std::move(*prepared);
    if(changed){p->from=p->displayed;p->target=p->prepared.target;p->start=now;p->duration=duration;p->animating=p->from!=p->target&&duration>runtime::Clock::duration{};if(!p->animating)p->displayed=p->target;}
    else if(duration<=runtime::Clock::duration{}){p->animating=false;p->displayed=p->target;}
    else if(p->animating){p->duration=duration;p->advance(now);}
    ++p->revision;if(auto r=p->configure();!r)return r;if(!p->animating)return root_.set_frame_handler(p->frame_callback,false);return {};
}
core::Result<void> MaterialSurface::set_primary_color(gui::WidgetColor color)const{auto o=options();if(!o)return std::unexpected(o.error());o->primary_color=color;return set_options(std::move(*o));}
core::Result<void> MaterialSurface::set_color(gui::WidgetColor color)const{auto o=options();if(!o)return std::unexpected(o.error());o->color=color;return set_options(std::move(*o));}
}
