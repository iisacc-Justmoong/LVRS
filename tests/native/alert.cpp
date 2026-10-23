#include <LVRS/Widgets.h>
#include <iiGUI/iiGUI.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r){if(r.error().cause)std::rethrow_exception(r.error().cause);throw std::runtime_error(r.error().message);}}
void geometry(){gui::Host host({.size={1000,800},.minimum_size={160,200}});lvrs::Alert alert;good(host.run({.opened=[&]{auto m=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));alert=take(lvrs::Alert::create(b,root,"alert",{.message="First line\nSecond line"}));return {};},lvrs::Theme().appearance()));good(alert.set_motion_enabled(false));good(alert.open());auto r=take(alert.root().snapshot()).bounds;check(r.width==500&&r.height==379&&take(alert.resolved_button_count())==2,"Two-action geometry differs from original 500x379");auto p=take(take(alert.action_button(1)).snapshot()),s=take(take(alert.action_button(2)).snapshot());check(p.size==gui::Size{219,56}&&s.size==p.size&&s.bounds.x<p.bounds.x&&s.bounds.x==r.x+24,"Two-action widths/order differ");good(alert.set_button_count(3));r=take(alert.root().snapshot()).bounds;check(r.height==479,"Three-action geometry differs from original 500x479");auto a=take(take(alert.action_button(1)).snapshot()),b=take(take(alert.action_button(2)).snapshot()),c=take(take(alert.action_button(3)).snapshot());check(a.size==gui::Size{452,56}&&b.size==a.size&&c.size==gui::Size{452,44}&&a.bounds.y+68==b.bounds.y&&b.bounds.y+68==c.bounds.y,"Vertical action sizes/spacing differ");good(alert.set_text("Alert Dialog","One line"));check(take(alert.root().snapshot()).bounds.height==466,"Copy did not shrink card");good(alert.set_text("",""));check(take(alert.root().snapshot()).bounds.height==385,"Empty copy retained text gaps");good(alert.set_show_icon(false));check(take(alert.root().snapshot()).bounds.height==299,"Hidden icon retained space");good(host.set_content_size({250,700}));check(take(alert.root().snapshot()).bounds.width==202,"Alert did not shrink below minWidth");good(host.request_close());}}));check(!alert.open(),"Retired Alert remained callable");}
void actions(){gui::Host host;std::string order;lvrs::Alert alert;std::vector<std::string> sources;good(host.run({.opened=[&]{auto m=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));auto method=[&](int i){return [&,i](const lvrs::ButtonEvent& e){check(e.trigger=="clicked"&&bool(e.source.snapshot()),"Typed button event lost source/trigger");sources.push_back(take(e.source.snapshot()).id);order+=char('0'+i);};};alert=take(lvrs::Alert::create(b,root,"alert",{.secondary_text="",.show_icon=false},{.button1_method=method(1),.button2_method=method(2),.button3_method=method(3),.primary_clicked=[&]{order+='a';},.secondary_clicked=[&]{order+='b';},.tertiary_clicked=[&]{order+='c';}}));return {};},lvrs::Theme().appearance()));good(alert.set_motion_enabled(false));good(alert.open());good(take(alert.action_button(1)).activate());check(take(alert.root().snapshot()).opened,"Action closed Alert automatically");good(alert.set_button_count(2));for(int i=1;i<=2;++i)good(take(alert.action_button(i)).activate());good(alert.set_button_count(3));for(int i=1;i<=3;++i)good(take(alert.action_button(i)).activate());check(order=="1a1a2b1a2b3c"&&sources.size()==6,"Six action positions changed method/click ordering");good(alert.set_action_enabled(2,false));good(take(alert.action_button(2)).activate());check(order=="1a1a2b1a2b3c","Disabled action executed");good(alert.set_action_method(1,[&](const auto&){order+='x';good(alert.set_action_method(1,[&](const auto&){order+='y';}));}));good(take(alert.action_button(1)).activate());good(take(alert.action_button(1)).activate());check(order.ends_with("xaya"),"Live method replacement invalidated active invocation");good(alert.set_action_method(1,[&](const auto&){good(alert.close());order+='z';}));good(take(alert.action_button(1)).activate());check(order.ends_with("za")&&!take(alert.root().snapshot()).opened,"Method close suppressed retained clicked signal");good(host.request_close());}}));}
void lifetime(){gui::Host host;lvrs::Alert alert;int dismissed=0;good(host.run({.opened=[&]{auto m=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));alert=take(lvrs::Alert::create(b,root,"alert",{}, {.dismissed=[&]{++dismissed;}}));return {};},lvrs::Theme().appearance()));auto count=take(m.snapshot()).size();good(alert.set_motion_enabled(false));good(alert.open());auto stable=take(alert.action_button(1));auto id=take(stable.snapshot()).id;for(int i=0;i<100;++i){good(alert.close());check(bool(stable.snapshot())&&!take(stable.snapshot()).visible,"Closing destroyed static Alert actions");good(alert.set_motion_enabled(false));good(alert.open());check(take(take(alert.action_button(1)).snapshot()).id==id&&take(m.snapshot()).size()==count,"Reopen reconstructed Alert widgets");}good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=1,.y=1}));good(m.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=1,.y=1}));good(m.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::escape}));check(take(alert.root().snapshot()).opened&&dismissed==0,"Default Alert dismissed outside or on Escape");check(take(alert.dismiss())&&!take(alert.dismiss())&&dismissed==1,"Explicit dismissal was not once-only");good(host.unmount_widgets());check(!stable.activate(),"Retired static action survived unmount");good(host.request_close());}}));}
void mutations(){gui::Host host;lvrs::Alert alert;int calls=0;good(host.run({.opened=[&]{auto m=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.vstack({},"root"));alert=take(lvrs::Alert::create(b,root,"alert"));return {};},lvrs::Theme().appearance()));good(alert.set_motion_enabled(false));good(alert.open());auto old=take(alert.action_button(1));good(old.focus());good(alert.set_button_count(3));check(take(take(alert.action_button(1)).snapshot()).focused,"Variant switch lost action focus");good(alert.set_button_count(1));check(take(alert.resolved_button_count())==2,"Only 2/3 are explicit variants");good(alert.set_action_text(2,""));check(take(alert.resolved_button_count())==1,"Text presence did not update auto count");good(alert.set_action_method(1,[&](const lvrs::ButtonEvent&){++calls;good(host.unmount_widgets());}));good(take(alert.action_button(1)).activate());check(calls==1&&!alert.root().snapshot(),"Owner removal did not retire action scope");good(host.request_close());}}));}
void assets(){for(auto [icon,name]:{std::pair{lvrs::AlertIcon::adjustments,"alert-adjustments.svg"},std::pair{lvrs::AlertIcon::file_text,"alert-file-text.svg"}}){std::ifstream file(std::string(ALERT_ASSET_ROOT)+"/"+name,std::ios::binary);std::string data{std::istreambuf_iterator<char>(file),{}};check(!data.empty()&&take(lvrs::alert_icon_source(icon))==data,"Original icon bytes were modified or missing");check(bool(gui::svg_image(data,128,128)),"Embedded Alert icon failed native decoding");}check(!lvrs::alert_icon_source(lvrs::AlertIcon(99)),"Invalid icon enum accepted");}
void rendering(){gui::Host host({.size={900,700}});lvrs::Alert alert;good(host.run({.opened=[&]{auto m=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{auto root=take(b.absolute_stack({},"root",{900,700}));std::vector<std::uint8_t> pixels(900*700*4);for(unsigned y=0;y<700;++y)for(unsigned x=0;x<900;++x){auto p=&pixels[(y*900+x)*4];p[0]=p[1]=p[2]=x/16%2?255:0;p[3]=255;}auto image=take(gui::rgba_image(900,700,std::move(pixels)));take(b.image(root,"background",{.image=image,.width=900,.height=700}));alert=take(lvrs::Alert::create(b,root,"alert",{.message="First line\nSecond line"}));return {};},lvrs::Theme().appearance()));good(alert.set_motion_enabled(false));good(alert.open());auto bounds=take(alert.root().snapshot()).bounds;auto image=take(host.capture_canvas());auto scale=take(host.snapshot()).backing_scale;auto p=[&](const gui::PixelBuffer& im,double x,double y){return im.rgba[(std::size_t(y*scale)*im.width+std::size_t(x*scale))*4];};auto x=bounds.x+8,y=bounds.y+120;int blur=std::abs(int(p(image,x,y))-p(image,x+16,y));auto stats=take(host.graphics_info());check(stats.backdrop_blurs>0&&blur<35,"Alert did not frost actual background content");good(alert.set_text("Another title","First line\nSecond line"));take(host.capture_canvas());check(take(host.graphics_info()).backdrop_blurs==stats.backdrop_blurs,"Overlay-only copy update reblurred unchanged content");good(alert.set_glass_enabled(false));auto sharp=take(host.capture_canvas());check(std::abs(int(p(sharp,x,y))-p(sharp,x+16,y))>blur+25,"glassEnabled false failed to retain translucent sharp tint");good(alert.set_primary_color({0,.9,.1,1}));good(alert.set_button_count(3));auto pixels=take(host.capture_canvas());check(!pixels.rgba.empty(),"Live primary/variant did not render");good(host.request_close());}}));}
void icon_fit() {
    gui::Host host({.size={1000,800}});
    lvrs::Alert alert;
    lvrs::Modal modal;
    good(host.run({.opened=[&] {
        std::vector<std::uint8_t> pixels(80*40*4);
        for(std::size_t i=0;i<pixels.size();i+=4) {pixels[i]=255;pixels[i+3]=255;}
        auto image=take(gui::rgba_image(80,40,std::move(pixels)));
        auto mount=take(host.mount_widgets([&](gui::WidgetBuilder& builder)->core::Result<void> {
            auto root=take(builder.vstack({},"root"));
            alert=take(lvrs::Alert::create(builder,root,"alert",{.image_source=image}));
            modal=take(lvrs::Modal::create(builder,root,"modal",{.icon_source=image,.motion_enabled=false}));
            return {};
        },lvrs::Theme().appearance()));
        auto verify=[&](gui::Widget widget,double inset) {
            auto bounds=take(widget.snapshot()).bounds;
            auto canvas=take(host.capture_canvas());
            const auto scale=take(host.snapshot()).backing_scale;
            auto pixel=[&](double x,double y) {return &canvas.rgba[(std::size_t(y*scale)*canvas.width+std::size_t(x*scale))*4];};
            const auto x=bounds.x+bounds.width/2;
            auto center=pixel(x,bounds.y+bounds.height/2),letterbox=pixel(x,bounds.y+inset);
            check(center[0]>245&&center[1]<5&&center[2]<5,"Custom icon center missing");
            check(letterbox[0]<120,"Non-square icon stretched instead of preserving its aspect ratio");
        };
        good(alert.set_motion_enabled(false));good(alert.open());
        verify(take(mount.find("alert.icon")),16);
        good(alert.close());
        good(modal.open());
        verify(take(modal.root().popup_content("modal.icon")),12);
        good(host.request_close());
    }}));
}

void shadow() {
    gui::Host host({.size={1000,800}});lvrs::Alert alert;
    good(host.run({.opened=[&] {
        auto appearance=lvrs::Theme().appearance();appearance.window={1,1,1,1};
        auto mount=take(host.mount_widgets([&](gui::WidgetBuilder& builder)->core::Result<void> {
            auto root=take(builder.vstack({},"root"));
            alert=take(lvrs::Alert::create(builder,root,"alert",{.title="",.message="",.secondary_text="",.backdrop_color={0,0,0,0},.card_color={1,1,1,1},.glass_enabled=false,.show_icon=false}));
            return {};
        },appearance));
        good(alert.set_motion_enabled(false));good(alert.open());auto bounds=take(alert.root().snapshot()).bounds;
        auto image=take(host.capture_canvas());const auto scale=take(host.snapshot()).backing_scale;
        auto pixel=[&](const gui::PixelBuffer& p,double x,double y){return p.rgba[(std::size_t(y*scale)*p.width+std::size_t(x*scale))*4];};
        const auto x=bounds.x+bounds.width/2,below=bounds.y+bounds.height+8;
        check(pixel(image,x,below)+6<pixel(image,x,bounds.y-8),"Authored downward shadow missing from native Alert");
        auto count=take(host.graphics_info()).shadow_blurs;check(count>0,"Alert used no real GPU shadow");
        good(alert.set_primary_color({0,1,0,1}));take(host.capture_canvas());
        check(take(host.graphics_info()).shadow_blurs==count,"Action palette change rebuilt shadow mask");
        good(alert.set_shadow_enabled(false));auto flat=take(host.capture_canvas());
        check(pixel(flat,x,below)>252&&take(host.graphics_info()).shadow_texture_bytes==0,"Disabling Alert shadow retained dark pixels or storage");
        good(host.request_close());
    }}));
}

}
int main(int argc,char** argv){try{std::string n=argc>1?argv[1]:"geometry";if(n=="geometry")geometry();else if(n=="actions")actions();else if(n=="lifetime")lifetime();else if(n=="mutations")mutations();else if(n=="assets")assets();else if(n=="rendering")rendering();else if(n=="icon_fit")icon_fit();else if(n=="shadow")shadow();else throw std::runtime_error("Unknown Alert case");std::cout<<"LVRS_NATIVE_ALERT_OK "<<n<<'\n';}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
