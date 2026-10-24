#include <LVRS/Avatar.h>
#include <LVRS/Card.h>
#include <LVRS/Widgets.h>
#include <iiGUI/Bindings.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}return std::move(*r);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
bool same(gui::Widget a,gui::Widget b){return take(a.snapshot()).id==take(b.snapshot()).id;}
bool near(double a,double b){return std::abs(a-b)<.01;}
std::shared_ptr<const gui::RasterImage> photo(){return take(gui::rgba_image(4,2,{255,0,0,255,0,255,0,255,0,0,255,255,255,255,0,255,255,0,0,255,0,255,0,255,0,0,255,255,255,255,0,255}));}
void run(const std::string& name){
 if(name=="assets"){for(auto icon:{"image","folder","project","device","model","member","link","more","check","globe","text"}){auto a=take(lvrs::card_icon(icon)),b=take(lvrs::card_icon(icon));check(a==b&&a->width()==64,"Card assets did not use bounded immutable cache");}check(!lvrs::card_icon("missing")&&!lvrs::card_icon("image",0)&&!lvrs::card_icon("image",513),"Invalid Card asset accepted");return;}
 gui::Host host({.size={700,600}});gui::WidgetMount mount;lvrs::Card card;lvrs::Avatar avatar;gui::Widget child,factory_root;unsigned clicked=0,selected=0,menu=0,action=0,built=0;std::vector<std::string> order;auto image=photo();
 core::Observable<std::string> title{"Initial"};core::Observable<std::shared_ptr<const gui::RasterImage>> source;auto bindings=std::make_shared<gui::BindingScope>();
 good(host.run({.opened=[&]{
   mount=take(host.mount_widgets([&](auto& b)->core::Result<void>{auto root=take(b.vstack({},"root",{.alignment=gui::StackAlignment::leading}));good(b.retain(bindings));
    if(name.starts_with("avatar_")){lvrs::AvatarOptions o;if(name=="avatar_circle"){o.width=100;o.height=40;o.source=image;}avatar=take(lvrs::Avatar::create(b,root,"avatar",o));if(name=="avatar_binding")good(gui::bind_member(*bindings,avatar.root(),source,[avatar](auto,const auto& value){return avatar.set_source(value);}));}
    else{lvrs::CardOptions o;o.title="Initial";o.metadata="PNG · 4 × 2";if(name=="information"||name=="rows"||name=="progress"||name=="actions"||name=="icon_factory")o.type=lvrs::CardType::project;
     if(name=="source"||name=="pixels"||name=="resize")o.preview_source=image;
     if(name=="preview")o.type=lvrs::CardType::file_preview;
     lvrs::CardContent content;
     if(name=="factory"||name=="factory_failure")content.preview=[&](auto& b)->core::Result<void>{++built;auto parent=take(b.vstack({},"previewRoot"));factory_root=parent;child=take(b.input_field(parent,"editor",{.text="Retained",.width=120}));return {};};
     if(name=="icon_factory")content.icon=[&](auto& b)->core::Result<void>{++built;child=take(b.label({},"customIcon",{.text="I"}));return {};};
     card=take(lvrs::Card::create(b,root,"card",o,{.clicked=[&]{++clicked;order.push_back("click");},.menu_requested=[&]{++menu;},.action_triggered=[&]{++action;},.selected_changed=[&](bool){++selected;order.push_back("selected");if(name=="reentrant")good(host.unmount_widgets());}},content));
     if(name=="inline"||name=="binding")child=take(b.input_field(card.root(),"inline",{.text="Keep",.width=100}));
     if(name=="binding")good(gui::bind_member(*bindings,card.root(),title,[card](auto,const auto& value){return card.set_title(value);}));
    }return {};
   }));
   if(name=="avatar_sizes"){for(auto [size,extent]:{std::pair{lvrs::AvatarSize::small,24.},std::pair{lvrs::AvatarSize::medium,40.},std::pair{lvrs::AvatarSize::large,64.}}){auto o=take(avatar.options());o.size=size;good(avatar.set_options(o));check(take(avatar.root().snapshot()).implicit_size==gui::Size{extent,extent},"Avatar authored implicit size changed");}}
   else if(name=="avatar_source"||name=="avatar_binding"){check(take(avatar.status())==lvrs::ImageStatus::empty&&take(avatar.progress())==0,"Empty avatar status changed");if(name=="avatar_binding")source.set(image);else good(avatar.set_source(image));check(take(avatar.status())==lvrs::ImageStatus::ready&&take(avatar.progress())==1&&take(avatar.options()).source==image,"Avatar immutable source not retained");good(avatar.set_source({}));check(take(avatar.status())==lvrs::ImageStatus::empty,"Avatar clear retained stale status");}
   else if(name=="avatar_circle"){auto o=take(avatar.options());check(o.width==100&&o.height==40&&o.source==image&&take(avatar.root().snapshot()).bounds.width==100,"Avatar did not use native centered circular cover");take(host.capture_canvas());}
   else if(name=="avatar_invalid"){auto o=take(avatar.options());o.width=-1;check(!avatar.set_options(o)&&take(avatar.options()).width==0,"Invalid Avatar options partially committed");o=take(avatar.options());o.background.alpha=2;check(!avatar.set_options(o),"Invalid Avatar color accepted");}
   else if(name=="avatar_retirement"){good(host.unmount_widgets());check(!avatar.status()&&!avatar.set_source(image),"Retired Avatar remained writable");}
   else if(name=="sizes"){for(auto [size,extent]:{std::pair{lvrs::CardSize::small,gui::Size{140,160}},std::pair{lvrs::CardSize::medium,gui::Size{210,240}},std::pair{lvrs::CardSize::large,gui::Size{360,280}}}){auto o=take(card.options());o.size=size;good(card.set_options(o));check(take(card.root().snapshot()).implicit_size==extent,"File Card authored dimensions changed");}}
   else if(name=="types"||name=="information"){for(unsigned type=0;type<8;++type){auto o=take(card.options());o.type=lvrs::CardType(type);good(card.set_options(o));auto state=take(card.state());check(state.image_card==(type<2)&&state.progress_card==(type==3||type==4)&&state.preview_only==(type==1)&&!state.variant_name.empty(),"Card type classification lost an authored variant");take(host.capture_canvas());}}
   else if(name=="selection"){good(card.root().activate());check(take(card.options()).selected&&order==std::vector<std::string>{"selected","click"},"Card check state/callback order changed");good(card.set_selected(false));check(!take(card.options()).selected&&selected==2&&clicked==1,"Programmatic selection clicked the card");}
   else if(name=="states"){auto o=take(card.options());o.display_state=lvrs::CardDisplayState::selected;good(card.set_options(o));check(take(card.state()).effective_selected&&take(card.state()).border_width==2&&!take(card.options()).selected,"Display-state override changed model selection");o.display_state=lvrs::CardDisplayState::hover;good(card.set_options(o));check(take(card.state()).effective_hovered&&!take(card.state()).effective_selected,"Card hover override lost authored appearance");}
   else if(name=="disabled"){auto o=take(card.options());o.enabled=false;good(card.set_options(o));good(card.root().activate());check(clicked==0&&selected==0&&!take(card.root().snapshot()).enabled,"Disabled Card dispatched activation");}
   else if(name=="preview"){good(card.set_selected(true));check(!take(card.state()).effective_selected&&take(card.state()).border_width==0&&!take(card.root().action_options()).tab_focusable,"FilePreview retained selection or keyboard focus");}
   else if(name=="actions"){good(take(mount.find("card/menu")).activate());good(take(mount.find("card/action")).activate());check(menu==1&&action==1&&clicked==0&&!take(card.options()).selected,"Internal card actions toggled/activated their parent");}
   else if(name=="source"){check(take(card.state()).preview_status==gui::ImageStatus::ready,"Card prepared preview is not ready");good(card.set_preview_source({}));check(take(card.state()).preview_status==gui::ImageStatus::empty,"Card clear retained stale image status");}
   else if(name=="inline"||name=="binding"){check(take(child.snapshot()).parent_id=="card/inline"&&same(take(card.preview_item()),card.preview_root()),"Card inline content did not use native preview slot");auto retained=child;if(name=="binding")title.set("Updated");else good(card.set_title("Updated"));check(take(card.options()).title=="Updated"&&take(retained.snapshot()).text=="Keep","Card data update reconstructed its child");}
   else if(name=="factory"||name=="icon_factory"){auto retained=child;good(card.set_title("Updated"));check(built==1&&retained.snapshot().has_value(),"Card data update rebuilt its custom component");if(name=="factory")check(same(take(card.preview_item()),factory_root),"Card custom preview item lost its identity");}
   else if(name=="factory_failure"){auto retained=child;auto failed=card.set_preview_content([](auto&)->core::Result<void>{return core::fail(core::ErrorCode::failed,"Intentional media failure");});check(!failed&&retained.snapshot().has_value()&&same(take(card.preview_item()),factory_root),"Failed Card component replaced committed content");failed=card.set_preview_content([](auto&)->core::Result<void>{throw std::runtime_error("Intentional media throw");});check(!failed&&failed.error().cause&&retained.snapshot().has_value(),"Card factory exception escaped its Result boundary");}
   else if(name=="rows"){std::vector<lvrs::CardRow> rows{{"Folders","42"},{"Files","1024"}};good(card.set_rows(rows));auto before=take(card.root().snapshot()).bounds;rows.resize(64,{"Value","123"});good(card.set_rows(rows));check(take(card.root().snapshot()).bounds.height>before.height,"Card rows did not grow their implicit body");rows.resize(65);check(!card.set_rows(rows)&&take(card.options()).rows.size()==64,"Card row budget partially committed");}
   else if(name=="progress"){auto o=take(card.options());o.progress=2;o.progress_text="200%";good(card.set_options(o));take(host.capture_canvas());o.progress=-1;good(card.set_options(o));take(host.capture_canvas());o.progress=std::numeric_limits<double>::quiet_NaN();check(!card.set_options(o)&&take(card.options()).progress==-1,"NaN progress partially committed");}
   else if(name=="invalid"){auto o=take(card.options());o.icon_name="unknown-native-media-icon";check(!card.set_options(o)&&take(card.options()).icon_name.empty(),"Bad icon partially committed");o=take(card.options());o.type=lvrs::CardType(9);check(!card.set_options(o),"Unknown Card type accepted");check(!card.set_title(std::string("a\0b",3))&&!card.set_title(std::string(65537,'x')),"Unbounded or NUL Card title accepted");lvrs::CardOptions oversized;oversized.menu_accessibility_name="";oversized.description=std::string(65535,'x');oversized.details="x";check(!card.set_options(oversized)&&take(card.options()).title=="Initial","Accessibility separator budget partially committed options");}
   else if(name=="metadata"){auto o=take(card.options());o.description="Details";o.details="RAW";o.status_text="Ready";good(card.set_options(o));check(take(card.root().snapshot()).accessibility_description=="Details. RAW. PNG · 4 × 2. Ready","Card accessibility metadata lost its authored fields");}
   else if(name=="pixels"){auto before=take(host.capture_canvas());good(card.set_selected(true));auto after=take(host.capture_canvas());check(before.rgba!=after.rgba,"Card selected border/check did not reach native pixels");}
   else if(name=="resize"){auto o=take(card.options());o.width=320;o.height=180;good(card.set_options(o));auto root=take(card.root().snapshot()).bounds,base=take(take(mount.find("card/surface")).snapshot()).bounds;check(near(root.width,320)&&near(base.width,320)&&near(base.height,180),"Card painter extent did not follow native layout");take(host.capture_canvas());}
   else if(name=="reentrant"){good(card.root().activate());check(selected==1&&clicked==0&&!card.options(),"Card click ran after selection callback retired its scope");}
   else if(name=="retirement"){good(host.unmount_widgets());check(!card.options()&&!card.set_selected(true),"Card survived its native generation");}
   else throw std::runtime_error("Unknown native media case");
   good(host.request_close());
 }}));
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"types");std::cout<<"LVRS_NATIVE_MEDIA_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
