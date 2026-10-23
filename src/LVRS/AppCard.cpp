#include "AppCard.h"
#include "SurfaceDetail.h"
namespace lvrs {
using namespace ii;
namespace {
bool valid(const AppCardOptions& o){return surface_detail::metric(o.width)&&surface_detail::metric(o.height)&&surface_detail::metric(o.padding)&&surface_detail::metric(o.spacing)&&surface_detail::metric(o.corner_radius)&&unsigned(o.shape)<=1&&surface_detail::text(o.title)&&surface_detail::text(o.subtitle)&&surface_detail::color(o.background)&&surface_detail::color(o.title_color)&&surface_detail::color(o.subtitle_color);}
}
struct AppCard::Impl final {
    AppCardOptions options;
    gui::Widget root,header,title,subtitle,separator,content;
    core::Result<void> paint(gui::Rect bounds)const{return surface_detail::paint(root,bounds,options.background,options.corner_radius,options.shape==AppCardShape::cylinder);}
    core::Result<void> configure(){
        const auto& o=options;const bool has_header=!o.title.empty()||!o.subtitle.empty();
        if(auto r=root.set_stack_options({.spacing=o.spacing,.padding=o.padding,.alignment=gui::StackAlignment::stretch,.minimum_width=280,.intrinsic_width=true});!r)return r;
        if(auto r=root.set_extent({o.width,o.height});!r)return r;
        if(auto r=title.set_label_options({.text=o.title,.font_size=17,.role=gui::TextRole::header,.color=o.title_color,.line_height=17,.elide_right=true});!r)return r;
        if(auto r=subtitle.set_label_options({.text=o.subtitle,.font_size=12,.role=gui::TextRole::description,.color=o.subtitle_color,.line_height=12,.elide_right=true});!r)return r;
        if(auto r=subtitle.set_visible(!o.subtitle.empty());!r)return r;
        if(auto r=header.set_visible(has_header);!r)return r;
        if(auto r=separator.set_visible(has_header);!r)return r;
        if(auto r=root.set_enabled(o.enabled);!r)return r;
        auto bounds=root.snapshot();if(!bounds)return std::unexpected(bounds.error());return paint(bounds->bounds);
    }
};
core::Result<AppCard> AppCard::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,AppCardOptions options,AppCardContentFactory content){
    if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid AppCard options");
    auto p=std::make_shared<Impl>();p->options=std::move(options);
    const auto named=[&](std::string_view part){return std::string(id)+"/"+std::string(part);};
    auto root=b.vstack(parent,id,{.spacing=p->options.spacing,.padding=p->options.padding,.alignment=gui::StackAlignment::stretch,.minimum_width=280,.intrinsic_width=true});if(!root)return std::unexpected(root.error());p->root=*root;
    auto header=b.vstack(*root,named("header"),{.spacing=4,.alignment=gui::StackAlignment::stretch});if(!header)return std::unexpected(header.error());p->header=*header;
    auto title=b.label(*header,named("title"));if(!title)return std::unexpected(title.error());p->title=*title;
    auto subtitle=b.label(*header,named("subtitle"));if(!subtitle)return std::unexpected(subtitle.error());p->subtitle=*subtitle;
    auto separator=b.vstack(*root,named("separator"));if(!separator)return std::unexpected(separator.error());p->separator=*separator;
    if(auto result=separator->set_extent({0,1});!result)return std::unexpected(result.error());
    if(auto result=separator->set_layout_handler([widget=*separator](gui::Rect bounds){surface_detail::require(surface_detail::paint(widget,bounds,{.1139791608,.1166428775,.1166428775,1},1));});!result)return std::unexpected(result.error());
    auto body=b.zstack(*root,named("content"),{.spacing=0,.alignment=gui::StackAlignment::leading,.minimum_height=1,.intrinsic_width=true,.measure_hidden_children=true});if(!body)return std::unexpected(body.error());p->content=*body;
    if(auto result=body->set_flex(1);!result)return std::unexpected(result.error());
    if(auto result=root->set_clip_content(true);!result)return std::unexpected(result.error());
    std::weak_ptr<Impl> weak=p;if(auto result=root->set_layout_handler([weak](gui::Rect bounds){if(auto p=weak.lock())surface_detail::require(p->paint(bounds));});!result)return std::unexpected(result.error());
    if(auto result=b.retain(p);!result)return std::unexpected(result.error());
    if(auto result=p->configure();!result)return std::unexpected(result.error());
    if(auto result=b.set_default_content(*root,*body);!result)return std::unexpected(result.error());
    if(content){try{auto result=content(b,*body);if(!result)return std::unexpected(result.error());}catch(...){return core::fail(core::ErrorCode::failed,"AppCard content factory failed",std::current_exception());}}
    if(auto live=root->snapshot();!live)return std::unexpected(live.error());
    AppCard result;result.root_=*root;result.content_=*body;result.state_=p;return result;
}
core::Result<std::shared_ptr<AppCard::Impl>> AppCard::lock()const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"AppCard scope retired");return p;}
core::Result<AppCardOptions> AppCard::options()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options;}
core::Result<void> AppCard::set_options(AppCardOptions options)const{auto p=lock();if(!p)return std::unexpected(p.error());if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid AppCard options");(*p)->options=std::move(options);return (*p)->configure();}
core::Result<void> AppCard::set_title(std::string_view title)const{auto o=options();if(!o)return std::unexpected(o.error());if(!surface_detail::text(title))return core::fail(core::ErrorCode::invalid_argument,"Invalid AppCard title");o->title=title;return set_options(std::move(*o));}
core::Result<void> AppCard::set_subtitle(std::string_view subtitle)const{auto o=options();if(!o)return std::unexpected(o.error());if(!surface_detail::text(subtitle))return core::fail(core::ErrorCode::invalid_argument,"Invalid AppCard subtitle");o->subtitle=subtitle;return set_options(std::move(*o));}
core::Result<void> AppCard::set_enabled(bool enabled)const{auto o=options();if(!o)return std::unexpected(o.error());o->enabled=enabled;return set_options(std::move(*o));}
core::Result<void> AppCard::set_background(gui::WidgetColor color)const{auto o=options();if(!o)return std::unexpected(o.error());o->background=color;return set_options(std::move(*o));}
core::Result<void> AppCard::set_shape(AppCardShape shape)const{auto o=options();if(!o)return std::unexpected(o.error());o->shape=shape;return set_options(std::move(*o));}
}
