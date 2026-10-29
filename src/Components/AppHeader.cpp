#include <LVRS/AppHeader.h>
#include <LVRS/SurfaceDetail.h>
namespace lvrs {
using namespace ii;
namespace {
bool valid(const AppHeaderOptions& o){
    if(!surface_detail::text(o.title)||!surface_detail::text(o.subtitle)||!surface_detail::metric(o.width)||!surface_detail::metric(o.height))return false;
    for(auto n:{o.content_horizontal_padding,o.content_vertical_padding,o.row_spacing,o.action_spacing,o.menu_button_padding})if(n&&!surface_detail::metric(*n))return false;
    for(auto c:{o.background,o.title_color,o.subtitle_color,o.menu_background,o.menu_pressed_background})if(!surface_detail::color(c))return false;return true;
}
}
struct AppHeader::Impl final {
    AppHeaderOptions options;AppHeaderEvents events;
    gui::Widget root,menu,title,subtitle,title_block,actions;
    core::Result<void> paint(gui::Rect bounds)const{return surface_detail::paint(root,bounds,options.background,0);}
    core::Result<void> configure(){
        const auto& o=options;const auto horizontal=o.content_horizontal_padding.value_or(o.compact?10:16),vertical=o.content_vertical_padding.value_or(o.compact?10:16),spacing=o.row_spacing.value_or(o.compact?8:12),padding=o.menu_button_padding.value_or(o.compact?8:10);
        if(auto r=root.set_stack_options({.spacing=spacing,.alignment=gui::StackAlignment::center,.insets=gui::Insets{horizontal,vertical,horizontal,vertical},.minimum_height=56,.implicit_insets=gui::Insets{horizontal,16,horizontal,16}});!r)return r;
        if(auto r=root.set_extent({o.width,o.height});!r)return r;
        if(auto r=title.set_label_options({.text=o.title,.font_size=22,.role=gui::TextRole::title2,.color=o.title_color,.line_height=22,.elide_right=true});!r)return r;
        if(auto r=subtitle.set_label_options({.text=o.subtitle,.font_size=12,.role=gui::TextRole::description,.color=o.subtitle_color,.line_height=12,.elide_right=true});!r)return r;
        if(auto r=subtitle.set_visible(!o.subtitle.empty());!r)return r;
        if(auto r=actions.set_stack_options({.spacing=o.action_spacing.value_or(o.compact?6:8),.alignment=gui::StackAlignment::center});!r)return r;
        if(auto r=menu.set_button_options({.text="Menu",.tone=gui::ButtonTone::normal,.accessibility_name="Menu",.height=12+padding*2,.corner_radius=4,.horizontal_padding=padding,.style=gui::ButtonStyle{.fill=o.menu_background,.hover_fill=o.menu_background,.pressed_fill=o.menu_pressed_background,.disabled_fill=o.menu_background,.text=o.title_color,.disabled_text=o.title_color,.line_height=12,.text_role=gui::TextRole::description,.font_size=12}});!r)return r;
        // Button geometry is kept as a retained native button; replacing the
        // callback never reconstructs user action children or backend scopes.
        if(auto r=menu.set_visible(o.menu_visible);!r)return r;
        if(auto r=root.set_enabled(o.enabled);!r)return r;
        auto snapshot=root.snapshot();if(!snapshot)return std::unexpected(snapshot.error());return paint(snapshot->bounds);
    }
};
core::Result<AppHeader> AppHeader::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,AppHeaderOptions options,AppHeaderEvents events,AppHeaderActionFactory content){
    if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid AppHeader options");auto p=std::make_shared<Impl>();p->options=std::move(options);p->events=std::move(events);std::weak_ptr<Impl> weak=p;
    const auto named=[&](std::string_view part){return std::string(id)+"/"+std::string(part);};
    auto root=b.hstack(parent,id);if(!root)return std::unexpected(root.error());p->root=*root;
    const auto padding=p->options.menu_button_padding.value_or(p->options.compact?8:10);
    auto menu=b.button(*root,named("menu"),{.text="Menu",.tone=gui::ButtonTone::normal,.height=12+padding*2,.corner_radius=4,.horizontal_padding=padding},[weak]{if(auto p=weak.lock()){auto callback=p->events.menu_clicked;if(callback)callback();}});if(!menu)return std::unexpected(menu.error());p->menu=*menu;
    auto title_block=b.vstack(*root,named("titles"),{.spacing=2,.alignment=gui::StackAlignment::stretch});if(!title_block)return std::unexpected(title_block.error());p->title_block=*title_block;
    if(auto result=title_block->set_flex(1);!result)return std::unexpected(result.error());
    auto title=b.label(*title_block,named("title"));if(!title)return std::unexpected(title.error());p->title=*title;
    auto subtitle=b.label(*title_block,named("subtitle"));if(!subtitle)return std::unexpected(subtitle.error());p->subtitle=*subtitle;
    auto actions=b.hstack(*root,named("actions"));if(!actions)return std::unexpected(actions.error());p->actions=*actions;
    if(auto result=root->set_clip_content(true);!result)return std::unexpected(result.error());
    if(auto result=root->set_layout_handler([weak](gui::Rect bounds){if(auto p=weak.lock())surface_detail::require(p->paint(bounds));});!result)return std::unexpected(result.error());
    if(auto result=b.retain(p);!result)return std::unexpected(result.error());if(auto result=p->configure();!result)return std::unexpected(result.error());
    if(auto result=b.set_default_content(*root,*actions);!result)return std::unexpected(result.error());
    if(content){try{auto result=content(b,*actions);if(!result)return std::unexpected(result.error());}catch(...){return core::fail(core::ErrorCode::failed,"AppHeader action factory failed",std::current_exception());}}
    if(auto live=root->snapshot();!live)return std::unexpected(live.error());
    AppHeader result;result.root_=*root;result.actions_=*actions;result.state_=p;return result;
}
core::Result<std::shared_ptr<AppHeader::Impl>> AppHeader::lock()const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"AppHeader scope retired");return p;}
core::Result<AppHeaderOptions> AppHeader::options()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options;}
core::Result<void> AppHeader::set_options(AppHeaderOptions options)const{auto p=lock();if(!p)return std::unexpected(p.error());if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid AppHeader options");(*p)->options=std::move(options);return (*p)->configure();}
core::Result<void> AppHeader::set_title(std::string_view title)const{auto o=options();if(!o)return std::unexpected(o.error());if(!surface_detail::text(title))return core::fail(core::ErrorCode::invalid_argument,"Invalid AppHeader title");o->title=title;return set_options(std::move(*o));}
core::Result<void> AppHeader::set_subtitle(std::string_view subtitle)const{auto o=options();if(!o)return std::unexpected(o.error());if(!surface_detail::text(subtitle))return core::fail(core::ErrorCode::invalid_argument,"Invalid AppHeader subtitle");o->subtitle=subtitle;return set_options(std::move(*o));}
core::Result<void> AppHeader::set_menu_visible(bool visible)const{auto o=options();if(!o)return std::unexpected(o.error());o->menu_visible=visible;return set_options(std::move(*o));}
core::Result<void> AppHeader::set_compact(bool compact)const{auto o=options();if(!o)return std::unexpected(o.error());o->compact=compact;return set_options(std::move(*o));}
core::Result<void> AppHeader::set_enabled(bool enabled)const{auto o=options();if(!o)return std::unexpected(o.error());o->enabled=enabled;return set_options(std::move(*o));}
core::Result<void> AppHeader::set_background(gui::WidgetColor color)const{auto o=options();if(!o)return std::unexpected(o.error());o->background=color;return set_options(std::move(*o));}
core::Result<void> AppHeader::set_menu_clicked(runtime::Task callback)const{auto p=lock();if(!p)return std::unexpected(p.error());std::swap((*p)->events.menu_clicked,callback);return {};}
}
