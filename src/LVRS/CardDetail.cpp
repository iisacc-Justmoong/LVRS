#include "CardDetail.h"
#include "SurfaceDetail.h"
#include "Icons.h"
#include "Theme.h"
#include <iiGUI/Text.h>
#include <algorithm>
#include <cmath>
namespace lvrs::card_detail {
using namespace ii;
namespace {
constexpr gui::WidgetColor bg03{.083941631,.083941631,.086313099,1},solid{.1139791608,.1166428775,.1166428775,1},alt{.1230252981,.1252803057,.1275353134,1},interactive{.1411764771,.1411764771,.1411764771,1},input{.1555851549,.157072559,.1585599482,1},raised{.1935568452,.1978072077,.1999323964,1};
constexpr const char* names[]{"File","FilePreview","Folder","Project","Device","Model","Member","Link"};
constexpr const char* icons[]{"image","image","folder","project","device","model","member","link"};
constexpr const char* actions[]{"","","Open","View","Manage","Use","Profile","Visit"};
bool image_card(const CardOptions& o){return o.type==CardType::file||o.type==CardType::file_preview;}
bool progress_card(const CardOptions& o){return o.type==CardType::project||o.type==CardType::device;}
bool menu(const CardOptions& o){return o.type!=CardType::file_preview&&o.show_menu.value_or(!image_card(o));}
bool action(const CardOptions& o){return !image_card(o)&&o.show_action.value_or(true);}
bool description(const CardOptions& o){return o.show_description&&!o.description.empty();}
bool metrics(const CardOptions& o){return o.type!=CardType::link&&(progress_card(o)||!o.rows.empty());}
double title_height(const CardOptions& o){return 15+(description(o)?16:0);}
double metrics_height(const CardOptions& o){const auto count=o.rows.size();return (progress_card(o)?34:0)+count*22+(progress_card(o)?count*12:std::max(0,int(count)-1)*2);}
double information_height(const CardOptions& o,double inset){
    double main=title_height(o)+1;unsigned parts=2;if(o.type==CardType::link){main+=61;++parts;}if(metrics(o)){main+=metrics_height(o);++parts;}if(o.type!=CardType::link&&!o.summary.empty()){main+=12;++parts;}
    return inset*2+36+22+20+main+(parts-1)*12;
}
gui::Rect box(double x,double y,double w,double h){return {float(x),float(y),float(std::max(0.,w)),float(std::max(0.,h))};}
void panel(gui::WidgetDrawing& d,gui::Rect r,gui::WidgetColor c,double radius,double opacity=1){if(r.width<=0||r.height<=0)return;c.alpha*=opacity;const auto ink=surface_detail::linear(c);d.panels.push_back({r,ink,ink,float(std::min(radius,double(std::min(r.width,r.height)/2)))});}
void text(gui::WidgetDrawing& d,const Prepared& p,gui::Rect r,std::string_view value,double pixels,double line,unsigned weight,gui::WidgetColor color,double opacity,bool elide=true,gui::TextAlignment alignment=gui::TextAlignment::left){if(r.width<=0||r.height<=0||value.empty())return;color.alpha*=opacity;d.texts.push_back({.bounds=r,.text=std::string(value),.color=surface_detail::linear(color),.font_size=float(pixels),.font_weight=weight,.alignment=alignment,.font_family=p.font,.line_height=float(line),.elide_right=elide,.clip=r});}
void image(gui::WidgetDrawing& d,gui::Rect bounds,std::shared_ptr<const gui::RasterImage> pixels,double opacity){if(bounds.width>0&&bounds.height>0&&pixels)d.images.push_back({.bounds=bounds,.image=std::move(pixels),.opacity=float(opacity)});}
gui::Rect cover(gui::Size bounds,const gui::RasterImage& image){const auto source=double(image.width())/image.height(),destination=bounds.width/bounds.height;gui::Rect uv{0,0,1,1};if(source>destination){uv.width=float(destination/source);uv.x=(1-uv.width)/2;}else{uv.height=float(source/destination);uv.y=(1-uv.height)/2;}return uv;}
}
core::Result<Prepared> prepare(const CardOptions& o,const gui::WidgetAppearance& appearance){
    if(unsigned(o.type)>7||unsigned(o.size)>2||unsigned(o.detail)>1||unsigned(o.display_state)>3||!surface_detail::metric(o.width)||!surface_detail::metric(o.height)||!std::isfinite(o.progress)||o.rows.size()>64)return core::fail(core::ErrorCode::invalid_argument,"Invalid Card type, size, geometry, progress or row count");
    std::size_t bytes=0;const auto admit=[&](std::string_view s){if(!surface_detail::text(s)||s.size()>65536-bytes)return false;bytes+=s.size();return true;};
    for(const auto* s:{&o.title,&o.description,&o.details,&o.metadata,&o.status_text,&o.summary,&o.footnote,&o.domain,&o.preview_title,&o.icon_name,&o.menu_accessibility_name,&o.progress_text})if(!admit(*s))return core::fail(core::ErrorCode::invalid_argument,"Card text exceeds its bounded UTF-8 contract");
    std::size_t accessible_bytes=0;unsigned accessible_fields=0;
    for(const auto* value:{&o.description,&o.details,&o.metadata,&o.status_text})if(!value->empty()){accessible_bytes+=value->size()+(accessible_fields?2:0);++accessible_fields;}
    if(accessible_bytes>65536)return core::fail(core::ErrorCode::invalid_argument,"Card accessibility description exceeds its UTF-8 byte budget");
    for(const auto& s:{o.action_text,o.progress_label})if(s&&!admit(*s))return core::fail(core::ErrorCode::invalid_argument,"Invalid Card action/progress label");
    for(const auto& row:o.rows)for(const auto* s:{&row.label,&row.value,&row.icon_name})if(!admit(*s))return core::fail(core::ErrorCode::invalid_argument,"Card row text exceeds its budget");
    for(auto color:{o.progress_color,o.status_color})if(color&&!surface_detail::color(*color))return core::fail(core::ErrorCode::invalid_argument,"Invalid Card color");
    Prepared p;p.primary=appearance.primary;p.font=appearance.font_family;p.action_text=o.action_text.value_or(actions[unsigned(o.type)]);p.progress_label=o.progress_label.value_or(o.type==CardType::device?"Storage":"Milestones");
    auto icon=o.icon?core::Result<std::shared_ptr<const gui::RasterImage>>{o.icon}:!o.icon_name.empty()?Icons::image(o.icon_name):card_icon(icons[unsigned(o.type)]);if(!icon)return std::unexpected(icon.error());p.icon=*icon;
    for(auto [name,target]:{std::pair{"image",&p.placeholder},std::pair{"check",&p.check},std::pair{"more",&p.more},std::pair{"globe",&p.globe}}){auto value=card_icon(name);if(!value)return std::unexpected(value.error());*target=*value;}
    static auto transparent=gui::rgba_image(1,1,std::vector<std::uint8_t>(4));if(!transparent)return std::unexpected(transparent.error());p.transparent=*transparent;static auto white=gui::rgba_image(1,1,std::vector<std::uint8_t>(4,255));if(!white)return std::unexpected(white.error());p.white=*white;
    gui::TextRun measure{.bounds={0,0,8192,8192},.text=p.action_text,.font_size=13,.font_weight=500,.font_family=p.font,.line_height=13};auto width=gui::measure_text_layout(measure);if(!width)return std::unexpected(width.error());p.action_width=std::ceil(width->width)+16;measure.text=o.progress_text;measure.font_size=12;measure.font_weight=600;measure.line_height=12;width=gui::measure_text_layout(measure);if(!width)return std::unexpected(width.error());p.progress_value_width=width->width;
    for(std::size_t i=0;i<o.rows.size();++i){const auto& row=o.rows[i];auto pixels=row.icon;if(!pixels&&!row.icon_name.empty()){auto decoded=Icons::image(row.icon_name);if(!decoded)return std::unexpected(decoded.error());pixels=*decoded;}else if(!pixels&&o.type==CardType::folder){const char* names[]{"folder","image","text"};auto decoded=card_icon(names[std::min(i,std::size_t(2))]);if(!decoded)return std::unexpected(decoded.error());pixels=*decoded;}p.row_icons.push_back(std::move(pixels));measure.text=row.value;measure.font_size=12;measure.font_weight=600;measure.line_height=12;width=gui::measure_text_layout(measure);if(!width)return std::unexpected(width.error());p.value_widths.push_back(width->width);}
    return p;
}
CardState resolve(const CardOptions& o,const Prepared& p,const gui::WidgetPaintState& actual,bool preview_custom){
    CardState state;state.image_card=image_card(o);state.preview_only=o.type==CardType::file_preview;state.progress_card=progress_card(o);state.variant_name=names[unsigned(o.type)];
    state.effective_selected=!state.preview_only&&(o.display_state==CardDisplayState::automatic?actual.checked:o.display_state==CardDisplayState::selected);
    state.effective_hovered=!state.preview_only&&(o.display_state==CardDisplayState::automatic?(actual.enabled&&(actual.hovered||actual.focused)):o.display_state==CardDisplayState::hover);
    state.border_width=state.preview_only?0:state.effective_selected?2:1;state.content_inset=(o.type==CardType::file&&o.size==CardSize::small?12:18)+state.border_width;
    state.border=state.effective_selected?p.primary:state.image_card&&state.effective_hovered?gui::WidgetColor{1,1,1,.9}:input;state.surface=state.image_card?bg03:state.effective_selected?alt:state.effective_hovered?interactive:solid;
    state.implicit_size=state.preview_only?gui::Size{480,320}:o.type==CardType::file?o.size==CardSize::small?gui::Size{140,160}:o.size==CardSize::large?gui::Size{360,280}:gui::Size{210,240}:gui::Size{256,std::max(280.,information_height(o,state.content_inset))};
    state.preview_status=state.image_card&&(preview_custom||o.preview_source)?gui::ImageStatus::ready:gui::ImageStatus::empty;return state;
}
Layout layout(const CardOptions& o,const Prepared& p,const CardState& state,gui::Size size){
    Layout l;l.width=size.width;l.height=size.height;l.inset=state.content_inset;
    const auto w=size.width,h=size.height,inset=l.inset,inner=std::max(0.,w-2*inset);
    l.icon=box(inset+9,inset+9,18,18);l.menu=state.image_card?box(std::max(0.,w-40),12,28,28):box(std::max(0.,w-inset-22),inset+7,22,22);
    l.action=box(std::max(0.,w-inset-std::min(p.action_width,inner*.5)),h-inset-22,std::min(p.action_width,inner*.5),22);
    if(state.image_card){l.title_height=o.size==CardSize::small?17:19;l.detail_height=o.size==CardSize::small?32:o.size==CardSize::large?36:54;l.caption_height=l.title_height+6+14+(o.detail==CardDetail::detailed?l.detail_height+6:0);l.title_y=h-inset-l.caption_height;l.detail_y=l.title_y+l.title_height+6;l.metadata_y=l.title_y+l.title_height+6+(o.detail==CardDetail::detailed?l.detail_height+6:0);}
    else{l.preview_y=inset+46;l.title_y=l.preview_y+(o.type==CardType::link?73:0);l.progress_y=l.title_y+title_height(o)+12;l.row_y=l.progress_y+(state.progress_card?46:0);l.summary_y=h-inset-22-10-12;}
    return l;
}
std::shared_ptr<const gui::WidgetDrawing> background(const CardOptions& o,const Prepared& p,const CardState& state,const Layout& l,bool preview_custom,bool icon_custom,bool enabled){
    auto d=std::make_shared<gui::WidgetDrawing>();d->images_before_text=true;if(l.width<=0||l.height<=0)return d;const auto opacity=enabled?1.:.4;const auto radius=state.preview_only?0:std::min({12.,l.width/2,l.height/2});panel(*d,box(0,0,l.width,l.height),state.surface,radius);
    if(state.image_card){if(!preview_custom&&o.preview_source){gui::ImageRun run{.bounds=box(0,0,l.width,l.height),.image=o.preview_source,.opacity=float(opacity),.source=cover({l.width,l.height},*o.preview_source),.radius=float(radius)};d->images.push_back(std::move(run));}else if(!preview_custom)image(*d,box((l.width-36)/2,(l.height-36)/2,36,36),p.placeholder,opacity);}
    else{panel(*d,box(l.inset,l.inset,36,36),bg03,6,opacity);if(!icon_custom)image(*d,l.icon,p.icon,opacity);if(o.type==CardType::link)panel(*d,box(l.inset,l.preview_y,std::max(0.,l.width-2*l.inset),61),bg03,6,opacity);}
    return d;
}
std::shared_ptr<const gui::WidgetDrawing> foreground(const CardOptions& o,const Prepared& p,const CardState& state,const Layout& l,bool enabled){
    auto d=std::make_shared<gui::WidgetDrawing>();d->images_before_text=true;if(l.width<=0||l.height<=0||state.preview_only)return d;
    const auto opacity=enabled?1.:.4,w=l.width,h=l.height,inset=l.inset,inner=std::max(0.,w-2*inset);const auto radius=std::min({12.,w/2,h/2});
    gui::ImageRun frame{.bounds=box(0,0,w,h),.image=p.transparent,.opacity=float(opacity),.radius=float(radius),.border_width=float(std::min(state.border_width,std::min(w,h)/2)),.border=surface_detail::linear(state.border)};
    if(state.image_card)frame.gradient=gui::ImageGradient{.height=float(std::min(h,l.caption_height+inset+64)),.middle_position=.36f,.middle={0,0,0,214.f/255},.bottom={0,0,0,245.f/255}};
    if(state.image_card)d->images.push_back(std::move(frame));else{const auto border=surface_detail::linear(state.border);d->panels.push_back({box(0,0,w,h),border,border,float(radius),float(std::min(state.border_width,std::min(w,h)/2))});}
    if(state.image_card){
        text(*d,p,box(inset,l.title_y,inner,l.title_height),o.title,o.size==CardSize::small?13:15,l.title_height,600,{1,1,1,.9},opacity);
        if(o.detail==CardDetail::detailed)text(*d,p,box(inset,l.detail_y,inner,l.detail_height),o.details,o.size==CardSize::small?12:13,o.size==CardSize::small?16:18,o.size==CardSize::small?600:500,{1,1,1,.8},opacity,false);
        text(*d,p,box(inset,l.metadata_y,inner,14),o.metadata,11,14,600,{1,1,1,.8},opacity);
        if(state.effective_selected){d->images.push_back({.bounds=box(12,12,24,24),.image=p.white,.tint=surface_detail::linear(p.primary),.opacity=float(opacity),.radius=12});image(*d,box(15,15,18,18),p.check,opacity);}
        return d;
    }
    if(state.effective_selected)image(*d,box(inset+44,inset+9,18,18),p.check,opacity);
    if(o.show_status&&!o.status_text.empty()){const auto right=menu(o)?l.menu.x-8:w-inset;const auto max_width=std::max(0.,inner-(state.effective_selected?104:78));const auto left=std::max(inset+(state.effective_selected?70:44),double(right)-max_width);const auto color=o.status_color.value_or(o.type==CardType::device||o.type==CardType::model||o.type==CardType::member?Theme::accent_color(Accent::green):gui::WidgetColor{1,1,1,.6});text(*d,p,box(left,inset+12,std::max(0.,right-left),12),o.status_text,12,12,600,color,opacity,true,gui::TextAlignment::right);}
    if(o.type==CardType::link){image(*d,box(inset+12,l.preview_y+12,18,18),p.globe,opacity);text(*d,p,box(inset+36,l.preview_y+15,std::max(0.,inner-48),12),o.domain,12,12,600,{1,1,1,.6},opacity);text(*d,p,box(inset+12,l.preview_y+36,std::max(0.,inner-24),13),o.preview_title,13,13,500,{1,1,1,.8},opacity);}
    text(*d,p,box(inset,l.title_y,inner,15),o.title,15,15,600,{1,1,1,.9},opacity);
    if(description(o))text(*d,p,box(inset,l.title_y+19,inner,12),o.description,12,12,600,{1,1,1,.6},opacity);
    const auto row=[&](double y,std::string_view label,std::string_view value,double value_width,std::shared_ptr<const gui::RasterImage> icon){const auto vw=std::min(value_width,inner*.5),ix=icon?24.:0;text(*d,p,box(w-inset-vw,y+5,vw,12),value,12,12,600,{1,1,1,.6},opacity,true,gui::TextAlignment::right);if(icon)image(*d,box(inset,y+2,18,18),std::move(icon),opacity);text(*d,p,box(inset+ix,y+4.5,std::max(0.,inner-vw-6-ix),13),label,13,13,500,{1,1,1,.8},opacity);};
    if(state.progress_card){row(l.progress_y,p.progress_label,o.progress_text,p.progress_value_width,{});panel(*d,box(inset,l.progress_y+30,inner,4),raised,2,opacity);panel(*d,box(inset,l.progress_y+30,inner*std::clamp(o.progress,0.,1.),4),o.progress_color.value_or(p.primary),2,opacity);}
    if(o.type!=CardType::link)for(std::size_t i=0;i<o.rows.size();++i)row(l.row_y+i*(state.progress_card?34:24),o.rows[i].label,o.rows[i].value,p.value_widths[i],p.row_icons[i]);
    if(o.type!=CardType::link&&!o.summary.empty())text(*d,p,box(inset,l.summary_y,inner,12),o.summary,12,12,600,{1,1,1,.6},opacity);
    text(*d,p,box(inset,h-inset-17,std::max(0.,inner-(action(o)?l.action.width+8:0)),12),o.footnote,12,12,600,{1,1,1,.6},opacity);
    return d;
}
}
