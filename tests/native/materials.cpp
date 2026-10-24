#include <LVRS/MaterialSurface.h>
#include <LVRS/PanelMaterial.h>
#include <LVRS/WindowMaterial.h>
#include <iiGUI/Bindings.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace ii;
using namespace std::chrono_literals;
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
bool near(double a,double b){return std::abs(a-b)<.001;}
void run(std::string name){runtime::EventLoop events;runtime::FrameLoop frames(events,{.capacity=name=="invalid"?1u:128u});gui::Host host({.size={600,440},.event_loop=&events,.frame_loop=&frames});gui::WidgetMount mount;lvrs::MaterialSurface material;gui::Widget child;std::vector<lvrs::MaterialSurface> extra;unsigned built=0;auto motion=std::make_shared<gui::MotionPreferences>();auto bindings=std::make_shared<gui::BindingScope>();core::Observable<gui::WidgetColor> accent{gui::WidgetColor{1,0,0,1}};
 good(host.run({.opened=[&]{
  mount=take(host.mount_widgets([&](auto& b)->core::Result<void>{auto root=take(b.vstack({},"root",{.padding=40,.alignment=gui::StackAlignment::leading}));good(b.retain(bindings));lvrs::MaterialOptions o;o.motion=motion;if(name!="surface_default"&&name!="panel_default"&&name!="window_default"){o.width=320;o.height=240;}
   if(name=="outline"||name=="cache"||name=="resize")o.outline_path="M 20 0 H 300 Q 320 0 320 20 V 200 L 180 200 L 160 230 L 140 200 H 20 Q 0 200 0 180 V 20 Q 0 0 20 0 Z";
   if(name=="capture")o.backdrop=lvrs::MaterialBackdrop::previous_layers;
   if(name=="window_default")material=take(lvrs::WindowMaterial::create(b,root,"material",o));else if(name=="surface_default")material=take(lvrs::MaterialSurface::create(b,root,"material",o));else material=take(lvrs::PanelMaterial::create(b,root,"material",o,[&](auto& b,auto parent)->core::Result<void>{++built;child=take(b.input_field(parent,"editor",{.text="Keep",.width=150}));return {};}));
   if(name=="factory"){extra.push_back(take(lvrs::PanelMaterial::create(b,root,"second",o)));extra.push_back(take(lvrs::PanelMaterial::create(b,root,"third",o)));}
   if(name=="bindings")good(gui::bind_member(*bindings,material.root(),accent,[material](auto,const auto& color){return material.set_primary_color(color);}));return {};
  }));
  if(name=="surface_default"){auto s=take(material.state());check(s.variant==lvrs::MaterialVariant::surface&&s.implicit_size==gui::Size{}&&near(s.tint_opacity,.25)&&s.blur_radius==16,"Base material authored defaults changed");}
  else if(name=="panel_default"){auto s=take(material.state());check(s.implicit_size==gui::Size{380,204}&&near(s.radius,12)&&near(s.inner_highlight_opacity,.14)&&near(s.shadow_opacity,.22),"Panel material authored defaults changed");}
  else if(name=="window_default"){auto s=take(material.state());check(s.variant==lvrs::MaterialVariant::window&&s.density==lvrs::MaterialDensity::dense75&&s.implicit_size==gui::Size{500,272}&&s.radius==16&&s.blur_radius==64&&near(s.tint_opacity,.5)&&s.shadow_radius==48,"Window material authored defaults changed");}
  else if(name=="density"){auto o=take(material.options());o.density=lvrs::MaterialDensity::dense75;good(material.set_options(o));auto s=take(material.state());check(near(s.tint_opacity,.75)&&s.blur_radius==64&&near(s.border.alpha,.12),"Density did not update derived material properties");}
  else if(name=="overrides"){auto o=take(material.options());o.density=lvrs::MaterialDensity::dense75;o.tint_opacity=.2;o.blur_radius=12;o.inner_highlight_opacity=.3;good(material.set_options(o));auto s=take(material.state());check(near(s.tint_opacity,.2)&&s.blur_radius==12&&near(s.inner_highlight_opacity,.3),"Explicit material overrides were replaced by density");}
  else if(name=="retained"||name=="factory"){auto old=child;std::size_t slots=0;if(name=="factory"){auto root=take(mount.find("root"));good(root.set_frame_handler([](auto)->core::Result<bool>{return false;},true));(void)frames.step(runtime::Clock::now());slots=frames.stats().active_ticks;}
   good(material.set_primary_color({0,1,0,1}));auto o=take(material.options());o.radius=30;good(material.set_options(o));check(built==1&&take(old.snapshot()).text=="Keep","Material update reconstructed ordinary content");
   if(name=="factory"){for(auto& item:extra)good(item.set_primary_color({1,0,0,1}));check(frames.stats().active_ticks==slots,"Materials allocated a Core frame slot per control");(void)frames.step(runtime::Clock::now()+500ms);check(!take(material.state()).animating,"Shared frame source did not finish material tracks");}}
  else if(name=="outline"){take(host.capture_canvas());auto s=take(material.state());check(s.mask_rasterizations==1&&s.gradient_rasterizations==1&&s.border_rasterizations==1,"Custom material path did not produce reusable mask/radials/border");}
  else if(name=="cache"){good(motion->set_reduced_motion(true));take(host.capture_canvas());auto first=take(material.state());auto uploads=take(host.graphics_info()).effect_image_uploads;good(material.set_primary_color({0,1,0,1}));take(host.capture_canvas());auto second=take(material.state());check(first.mask_rasterizations==second.mask_rasterizations&&first.gradient_rasterizations==second.gradient_rasterizations&&take(host.graphics_info()).effect_image_uploads==uploads,"Accent RGB-only update regenerated mask/gradient textures");}
  else if(name=="resize"){take(host.capture_canvas());auto before=take(material.state());auto o=take(material.options());o.width=360;good(material.set_options(o));take(host.capture_canvas());check(take(material.state()).mask_rasterizations==before.mask_rasterizations+1&&take(material.root().snapshot()).bounds.width==360,"Material resize did not update its raster geometry");}
  else if(name=="capture"){check(take(material.state()).capture_requested,"Explicit material capture domain not requested");take(host.capture_canvas());}
  else if(name=="standalone"){take(host.capture_canvas());check(!take(material.state()).capture_requested,"Standalone material captured siblings implicitly");}
  else if(name=="blur"){auto o=take(material.options());o.blur_enabled=false;good(material.set_options(o));check(take(material.state()).blur_radius==0&&take(material.state()).capture_padding==16,"Disabled blur changed original capture padding");}
  else if(name=="motion"||name=="reduced"||name=="disabled_motion"){if(name=="motion"){good(motion->set_reduced_motion(true));good(material.set_primary_color({0,0,0,1}));good(motion->set_reduced_motion(false));}if(name=="reduced")good(motion->set_reduced_motion(true));auto o=take(material.options());if(name=="disabled_motion")o.motion_enabled=false;o.primary_color=gui::WidgetColor{0,1,0,1};good(material.set_options(o));check(take(material.state()).animating==(name=="motion"),"Material motion preference admission changed");if(name=="motion"){auto now=runtime::Clock::now();(void)frames.step(now+65ms);auto mid=take(material.state());check(mid.animating&&mid.displayed_primary.green>.8&&mid.displayed_primary.green<.95,"Material color did not follow Core OutCubic frames");good(material.set_primary_color({1,0,0,1}));good(frames.set_time_scale(0));frames.set_paused(true);(void)frames.step(std::max(runtime::Clock::now(),now+65ms)+220ms);check(!take(material.state()).animating&&take(material.state()).displayed_primary.red==1&&take(material.state()).displayed_primary.green==0,"Game pause froze unscaled material motion or final color drifted");good(material.set_primary_color({0,1,0,1}));good(motion->set_reduced_motion(true));check(!take(material.state()).animating&&take(material.state()).displayed_primary.green==1,"Live reduced motion did not settle material color");}}
  else if(name=="bindings"){good(motion->set_reduced_motion(true));accent.set({0,0,1,1});check(near(take(material.state()).displayed_primary.blue,1)&&built==1,"Core color binding reconstructed or failed to update material");}
  else if(name=="invalid"){auto o=take(material.options());o.tint_opacity=2;check(!material.set_options(o)&&!take(material.options()).tint_opacity,"Invalid opacity partially committed");o=take(material.options());o.blur_radius=std::numeric_limits<double>::quiet_NaN();check(!material.set_options(o),"NaN material radius accepted");o=take(material.options());o.primary_color=gui::WidgetColor{2,0,0,1};check(!material.set_options(o),"Invalid primary color accepted");o=take(material.options());o.outline_path=std::string("a\0b",3);check(!material.set_options(o),"NUL outline accepted");auto admission=material.set_primary_color({1,.25,.5,1});check(!admission&&!take(material.options()).primary_color&&!take(material.state()).animating,"Exhausted frame source partially committed material color");}
  else if(name=="retirement"){good(host.unmount_widgets());check(!material.state()&&!material.set_color({1,1,1,1})&&!child.snapshot(),"Material owner survived its view generation");good(motion->set_reduced_motion(true));}
  else throw std::runtime_error("Unknown native material case");
  good(host.request_close());
 }}));
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"panel_default");std::cout<<"LVRS_NATIVE_MATERIALS_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
