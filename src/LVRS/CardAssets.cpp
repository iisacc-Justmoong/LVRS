#include "Card.h"
#include "Icons.h"
#include <algorithm>
#include <mutex>
#include <list>
namespace lvrs {
namespace detail {std::span<const IconSource> embedded_card_icons();}
ii::core::Result<std::shared_ptr<const ii::gui::RasterImage>> card_icon(std::string_view name,std::uint32_t pixels){
    using namespace ii;
    if(!pixels||pixels>512)return core::fail(core::ErrorCode::invalid_argument,"Card icon dimensions must be 1..512 pixels");
    auto sources=detail::embedded_card_icons();auto found=std::ranges::find(sources,name,&IconSource::name);if(found==sources.end())return core::fail(core::ErrorCode::not_found,"Unknown authored Card icon");
    struct Cached{std::string name;std::uint32_t pixels;std::shared_ptr<const gui::RasterImage> image;};static std::mutex mutex;static std::list<Cached> cache;static std::size_t bytes=0;
    {std::lock_guard lock(mutex);auto entry=std::ranges::find_if(cache,[&](const auto& e){return e.name==name&&e.pixels==pixels;});if(entry!=cache.end()){auto image=entry->image;cache.splice(cache.begin(),cache,entry);return image;}}
    auto image=gui::svg_image(found->svg,pixels,pixels);if(!image)return std::unexpected(image.error());
    std::lock_guard lock(mutex);auto existing=std::ranges::find_if(cache,[&](const auto& e){return e.name==name&&e.pixels==pixels;});if(existing!=cache.end()){auto image=existing->image;cache.splice(cache.begin(),cache,existing);return image;}
    bytes+=(*image)->rgba().size();cache.push_front({std::string(name),pixels,*image});while(cache.size()>64||bytes>8*1024*1024){bytes-=cache.back().image->rgba().size();cache.pop_back();}return *image;
}
}
