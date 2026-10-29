#include <LVRS/ToolbarButton.h>
#include <LVRS/ToolbarDetail.h>
namespace lvrs {
using namespace ii;
struct ToolbarButton::Impl final {
    ToolbarButtonOptions options;gui::Widget root;runtime::Task clicked,activate,changed;bool active=false;
    core::Result<void> configure(gui::ButtonOptions native){
        if(auto r=root.set_button_options(std::move(native));!r)return r;
        return root.set_visible(options.visible);
    }
};
core::Result<ToolbarButton> ToolbarButton::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,ToolbarButtonOptions options,runtime::Task clicked){
    auto native=toolbar_detail::prepare(options);if(!native)return std::unexpected(native.error());auto p=std::make_shared<Impl>();p->options=std::move(options);p->clicked=std::move(clicked);std::weak_ptr<Impl> weak=p;
    auto root=b.button(parent,id,*native,[weak]{auto p=weak.lock();if(!p)return;auto activate=p->activate,clicked=p->clicked;if(activate)activate();if(p->root.snapshot()&&clicked)clicked();});if(!root)return std::unexpected(root.error());p->root=*root;
    if(auto r=b.retain(p);!r)return std::unexpected(r.error());if(auto r=p->configure(std::move(*native));!r)return std::unexpected(r.error());
    ToolbarButton result;result.root_=*root;result.state_=p;return result;
}
core::Result<std::shared_ptr<ToolbarButton::Impl>> ToolbarButton::lock()const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"ToolbarButton scope retired");return p;}
core::Result<ToolbarButtonOptions> ToolbarButton::options()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options;}
core::Result<bool> ToolbarButton::active()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->active;}
core::Result<void> ToolbarButton::set_options(ToolbarButtonOptions options)const{auto p=lock();if(!p)return std::unexpected(p.error());auto native=toolbar_detail::prepare(options,(*p)->active);if(!native)return std::unexpected(native.error());(*p)->options=std::move(options);if(auto r=(*p)->configure(std::move(*native));!r)return r;auto changed=(*p)->changed;if(changed)changed();return {};}
core::Result<void> ToolbarButton::set_clicked(runtime::Task callback)const{auto p=lock();if(!p)return std::unexpected(p.error());std::swap((*p)->clicked,callback);return {};}
core::Result<void> ToolbarButton::attach(runtime::Task callback,runtime::Task changed)const{auto p=lock();if(!p)return std::unexpected(p.error());std::swap((*p)->activate,callback);std::swap((*p)->changed,changed);return {};}
core::Result<void> ToolbarButton::set_active(bool value)const{auto p=lock();if(!p)return std::unexpected(p.error());if((*p)->active==value)return {};auto native=toolbar_detail::prepare((*p)->options,value);if(!native)return std::unexpected(native.error());if(auto r=(*p)->root.set_button_options(std::move(*native));!r)return r;(*p)->active=value;return {};}
}
