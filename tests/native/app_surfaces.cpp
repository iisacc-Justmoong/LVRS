#include <LVRS/AppCard.h>
#include <LVRS/AppHeader.h>
#include <LVRS/Widgets.h>
#include <iiGUI/Bindings.h>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> result){if(!result)throw std::runtime_error(result.error().message);return std::move(*result);}
void good(core::Result<void> result){if(!result)throw std::runtime_error(result.error().message);}
bool near(double a,double b){return std::abs(a-b)<.01;}
void run(const std::string& name){
    gui::Host host({.size={720,600}});gui::WidgetMount mount;lvrs::AppCard card;lvrs::AppHeader header;gui::Widget child;unsigned built=0,clicked=0;
    if(name=="factory_failure"){
        good(host.run({.opened=[&]{
            mount=take(host.mount_widgets([](auto& b)->core::Result<void>{auto root=b.vstack({},"stable");if(!root)return std::unexpected(root.error());return {};}));auto stable=take(mount.find("stable"));
            auto failed=host.mount_widgets([](auto& b)->core::Result<void>{auto card=lvrs::AppCard::create(b,{},"bad",{},[](auto&,auto)->core::Result<void>{return core::fail(core::ErrorCode::failed,"Intentional body failure");});if(!card)return std::unexpected(card.error());return {};});
            check(!failed&&stable.snapshot().has_value(),"Failed card body replaced the committed view");
            failed=host.mount_widgets([](auto& b)->core::Result<void>{auto header=lvrs::AppHeader::create(b,{},"bad",{},{},[](auto&,auto)->core::Result<void>{throw std::runtime_error("Intentional action factory failure");});if(!header)return std::unexpected(header.error());return {};});
            check(!failed&&failed.error().cause&&stable.snapshot().has_value(),"Failed header actions escaped or replaced the committed view");good(host.request_close());
        }}));return;
    }
    core::Observable<std::string> title{"Initial title"};core::Observable<bool> compact{false};auto bindings=std::make_shared<gui::BindingScope>();
    good(host.run({.opened=[&]{
        mount=take(host.mount_widgets([&](auto& b)->core::Result<void>{
            auto root=take(b.vstack({},"root",{.spacing=0,.alignment=gui::StackAlignment::leading}));good(b.retain(bindings));
            if(name.starts_with("card_")){
                lvrs::AppCardOptions options;
                if(name!="card_default")options.title="Summary";
                if(name=="card_header")options.subtitle="Details";
                if(name=="card_geometry"||name=="card_shape"){options.width=320;options.height=220;}
                if(name=="card_shape")options.background={1,0,0,1};
                if(name=="card_factory")card=take(lvrs::AppCard::create(b,root,"card",options,[&](auto& b,auto content)->core::Result<void>{child=take(b.input_field(content,"editor",{.text="Retained",.width=120}));++built;return {};}));
                else card=take(lvrs::AppCard::create(b,root,"card",options));
                if(name=="card_children"||name=="card_geometry"||name=="card_retirement"){
                    child=take(b.input_field(card.root(),"editor",{.text="Retained",.width=120}));++built;
                }
                if(name=="card_bindings")good(gui::bind_member(*bindings,card.root(),title,[card](auto,const auto& value){return card.set_title(value);}));
                if(name=="card_legacy"){
                    lvrs::Widgets controls(b);auto legacy=take(controls.app_card(root,"legacy",{.width=300,.height=180}));auto label=take(b.label(legacy,"legacyChild",{.text="Content"}));check(take(label.snapshot()).parent_id=="legacy/content","Legacy factory bypassed the default content slot");
                }
            }else{
                lvrs::AppHeaderOptions options{.title="Dashboard",.subtitle=name=="header_default"?"Overview":"",.menu_visible=true};
                lvrs::AppHeaderActionFactory factory;if(name=="header_factory")factory=[&](auto& b,auto actions)->core::Result<void>{child=take(b.input_field(actions,"action",{.text="Retained",.width=90}));++built;return {};};
                header=take(lvrs::AppHeader::create(b,root,"header",options,{.menu_clicked=[&]{++clicked;if(name=="header_retirement")good(host.unmount_widgets());}},factory));
                if(name!="header_factory"){child=take(b.input_field(header.root(),"action",{.text="Retained",.width=90}));++built;}
                if(name=="header_bindings")good(gui::bind_member(*bindings,header.root(),compact,[header](auto,bool value){return header.set_compact(value);}));
            }
            return {};
        }));
        const auto find=[&](std::string_view id){return take(mount.find(id));};
        if(name=="card_default"){
            auto snapshot=take(card.root().snapshot());check(near(snapshot.implicit_size.width,280)&&near(snapshot.implicit_size.height,37)&&!take(find("card/header").snapshot()).visible,"Default AppCard still uses fixed stub geometry");
        }else if(name=="card_header"){
            check(near(take(card.root().snapshot()).implicit_size.height,91)&&near(take(find("card/title").snapshot()).font_size,17),"AppCard header metrics changed");good(card.set_subtitle(""));check(near(take(card.root().snapshot()).implicit_size.height,75)&&!take(find("card/subtitle").snapshot()).visible,"Subtitle removal left a layout gap");good(card.set_title(""));check(near(take(card.root().snapshot()).implicit_size.height,37),"Empty header was not removed");
        }else if(name=="card_children"||name=="card_geometry"||name=="card_factory"){
            check(take(child.snapshot()).parent_id=="card/content"&&built==1,"AppCard child scope did not use its body slot");auto old=child;good(card.set_title("Changed"));good(card.set_subtitle("New subtitle"));good(card.set_background({.2,.1,.3,1}));check(take(old.snapshot()).text=="Retained"&&built==1,"AppCard update reconstructed user content");
            if(name=="card_geometry"){const auto r=take(card.root().snapshot()).bounds,body=take(card.content_root().snapshot()).bounds;check(near(r.width,320)&&near(r.height,220)&&near(body.x,r.x+18)&&body.y>r.y+18,"AppCard did not inset and resize its content slot");}
        }else if(name=="card_shape"){
            auto before=take(host.capture_canvas());good(card.set_shape(lvrs::AppCardShape::cylinder));auto after=take(host.capture_canvas());check(take(card.options()).shape==lvrs::AppCardShape::cylinder&&before.rgba!=after.rgba,"Cylinder shape replacement did not change native pixels");
        }else if(name=="card_bindings"){
            title.set("Live title");check(take(find("card/title").snapshot()).text=="Live title","Core title change did not reach the retained card");
        }else if(name=="card_validation"){
            auto options=take(card.options());options.padding=-1;check(!card.set_options(options)&&take(card.options()).padding==18,"Invalid AppCard options partially committed");check(!card.set_title(std::string("x\0y",3))&&!card.set_title(std::string(65537,'x')),"Invalid AppCard title accepted");
        }else if(name=="card_retirement"){
            good(host.unmount_widgets());check(!card.options()&&!child.snapshot(),"AppCard retained a retired child generation");
        }else if(name=="card_legacy"){
            check(take(find("legacy").snapshot()).id=="legacy","Legacy factory did not retain its outer widget id");
        }else if(name=="header_factory"){
            auto retained=child;good(header.set_title("Updated"));good(header.set_compact(true));check(built==1&&take(retained.snapshot()).text=="Retained"&&take(retained.snapshot()).parent_id=="header/actions","Action factory reran during header updates");
        }else if(name=="header_default"){
            auto root=take(header.root().snapshot());check(near(root.implicit_size.height,68)&&near(take(find("header/title").snapshot()).font_size,22)&&near(take(find("header/menu").snapshot()).font_size,12),"AppHeader typography or implicit height changed");
            check(take(child.snapshot()).parent_id=="header/actions","AppHeader actions bypassed the default slot");
        }else if(name=="header_compact"||name=="header_bindings"){
            const auto before=take(find("header/menu").snapshot()).bounds;auto retained=child;
            if(name=="header_bindings")compact.set(true);else good(header.set_compact(true));
            auto after=take(find("header/menu").snapshot()).bounds;check(near(before.height,32)&&near(after.height,28)&&after.width<before.width&&take(retained.snapshot()).text=="Retained"&&built==1,"Compact update lost menu geometry or user content identity");
        }else if(name=="header_menu"||name=="header_retirement"){
            good(find("header/menu").activate());check(clicked==1,"Menu callback was not dispatched");if(name=="header_retirement")check(!header.options(),"Menu callback did not retire the header safely");else{good(header.set_menu_clicked([&]{clicked+=10;}));good(find("header/menu").activate());check(clicked==11,"Live menu callback replacement was lost");good(header.set_menu_visible(false));check(!take(find("header/menu").snapshot()).visible,"Menu visibility did not update");}
        }else if(name=="header_disabled"){
            good(header.set_enabled(false));good(find("header/menu").activate());check(!take(child.snapshot()).enabled&&clicked==0,"Disabled header exposed active actions");good(header.set_enabled(true));check(take(child.snapshot()).enabled,"Header re-enable lost requested action state");
        }else if(name=="header_validation"){
            auto options=take(header.options());options.content_horizontal_padding=std::numeric_limits<double>::quiet_NaN();check(!header.set_options(options)&&!take(header.options()).content_horizontal_padding,"Invalid AppHeader geometry partially committed");
        }else if(name=="header_geometry"){
            auto options=take(header.options());options.width=600;options.height=100;options.content_horizontal_padding=24;options.content_vertical_padding=8;good(header.set_options(options));const auto root=take(header.root().snapshot()).bounds,menu=take(find("header/menu").snapshot()).bounds,action=take(child.snapshot()).bounds;
            check(near(menu.x,root.x+24)&&near(action.x+action.width,root.x+root.width-24)&&near(root.height,100),"Header padding or trailing actions did not follow its explicit frame");
        }else throw std::runtime_error("Unknown app surface case");
        good(host.request_close());
    }}));
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"card_default");std::cout<<"LVRS_APP_SURFACES_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
