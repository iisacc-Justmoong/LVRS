#include <LVRS/Image.h>
namespace lvrs {
struct Image::Impl final {ii::gui::Widget root;ii::gui::ImageOptions options;};
ii::core::Result<Image> Image::create(ii::gui::WidgetBuilder& builder,ii::gui::Widget parent,std::string_view id,ii::gui::ImageOptions options){
    auto root=builder.image(parent,id,options);if(!root)return std::unexpected(root.error());
    auto state=std::make_shared<Impl>();state->root=*root;state->options=std::move(options);
    if(auto retained=builder.retain(state);!retained)return std::unexpected(retained.error());
    Image result;result.root_=*root;result.state_=state;return result;
}
ii::core::Result<std::shared_ptr<Image::Impl>> Image::lock()const{
    auto live=root_.snapshot();if(!live)return std::unexpected(live.error());
    auto state=state_.lock();if(!state)return ii::core::fail(ii::core::ErrorCode::closed,"Image scope retired");return state;
}
ii::core::Result<ii::gui::ImageOptions> Image::options()const{auto state=lock();if(!state)return std::unexpected(state.error());return (*state)->options;}
ii::core::Result<void> Image::set_options(ii::gui::ImageOptions options)const{
    auto state=lock();if(!state)return std::unexpected(state.error());
    if(auto changed=root_.set_image_options(options);!changed)return changed;
    (*state)->options=std::move(options);return {};
}
ii::core::Result<void> Image::set_source(std::shared_ptr<const ii::gui::RasterImage> source)const{
    auto current=options();if(!current)return std::unexpected(current.error());current->image=std::move(source);return set_options(std::move(*current));
}
}
