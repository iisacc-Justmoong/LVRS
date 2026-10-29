#include <LVRS/Avatar.h>
#include <LVRS/SurfaceDetail.h>
namespace lvrs {
using namespace ii;
namespace {
bool valid(const AvatarOptions& o){return unsigned(o.size)<=2&&surface_detail::metric(o.width)&&surface_detail::metric(o.height)&&surface_detail::color(o.background)&&surface_detail::text(o.accessibility_name);}
gui::ImageOptions native(const AvatarOptions& o){const auto size=o.size==AvatarSize::small?24:o.size==AvatarSize::medium?40:64;return {.image=o.source,.width=o.width?o.width:size,.height=o.height?o.height:size,.accessibility_name=o.accessibility_name,.background=o.background,.fit=gui::ImageFit::cover,.circular=true,.allow_empty=true};}
}
struct Avatar::Impl final {AvatarOptions options;gui::Widget root;};
core::Result<Avatar> Avatar::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,AvatarOptions o){
    if(!valid(o))return core::fail(core::ErrorCode::invalid_argument,"Invalid Avatar options");auto p=std::make_shared<Impl>();p->options=std::move(o);auto root=b.image(parent,id,native(p->options));if(!root)return std::unexpected(root.error());p->root=*root;if(auto r=root->set_enabled(p->options.enabled);!r)return std::unexpected(r.error());if(auto r=b.retain(p);!r)return std::unexpected(r.error());Avatar result;result.root_=*root;result.state_=p;return result;
}
core::Result<std::shared_ptr<Avatar::Impl>> Avatar::lock()const{auto live=root_.snapshot();if(!live)return std::unexpected(live.error());auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"Avatar scope retired");return p;}
core::Result<AvatarOptions> Avatar::options()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options;}
core::Result<void> Avatar::set_options(AvatarOptions o)const{auto p=lock();if(!p)return std::unexpected(p.error());if(!valid(o))return core::fail(core::ErrorCode::invalid_argument,"Invalid Avatar options");if(auto r=root_.set_image_options(native(o));!r)return r;if(auto r=root_.set_enabled(o.enabled);!r)return r;(*p)->options=std::move(o);return {};}
core::Result<void> Avatar::set_source(std::shared_ptr<const gui::RasterImage> source)const{auto o=options();if(!o)return std::unexpected(o.error());o->source=std::move(source);return set_options(std::move(*o));}
core::Result<ImageStatus> Avatar::status()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options.source?ImageStatus::ready:ImageStatus::empty;}
core::Result<double> Avatar::progress()const{auto p=lock();if(!p)return std::unexpected(p.error());return (*p)->options.source?1.:0.;}
}
