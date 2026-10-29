#include "SvgManager.h"
#include "PresentationDetail.h"
#include <iiVisualCodec/ImageCodec.h>
#include <iiNetwork/Wire.h>
#include <array>
#include <cmath>
#include <iomanip>
#include <list>
#include <locale>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace lvrs {
namespace pd=presentation_detail;
namespace {
std::string lower(std::string_view value){std::string result(value);for(auto& c:result)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return result;}
void finite(double value){if(!std::isfinite(value))throw std::invalid_argument("SVG scale must be finite");}
std::string cache_key(std::string_view url,int size,double scale){
    std::ostringstream text;text.imbue(std::locale::classic());text<<url<<'|'<<size<<'|'<<std::fixed<<std::setprecision(3)<<scale;return text.str();
}
}
struct SvgManager::Impl final:pd::Signals<SvgManagerEvent>,std::enable_shared_from_this<Impl> {
    struct Raster final {
        std::string key,data;
        std::shared_ptr<const ii::gui::RasterImage> image;
        std::size_t bytes=0;
    };
    struct Source final{std::string key,payload;std::size_t bytes()const{return sizeof(Source)+key.size()*2+payload.size();}};
    struct Flight final {
        std::string key;
        std::shared_ptr<SvgRequest> request;
        bool admitting=true,inline_completion=false;
    };
    std::shared_ptr<SvgSource> source;
    SvgCacheLimits limits;
    std::list<std::shared_ptr<Raster>> rasters;
    std::unordered_map<std::string,std::list<std::shared_ptr<Raster>>::iterator> raster_index;
    std::list<Source> sources;
    std::unordered_map<std::string,std::list<Source>::iterator> source_index;
    std::unordered_set<std::string> pending;
    std::unordered_map<std::uint64_t,Flight> flights;
    std::size_t raster_bytes=0,source_bytes=0;
    std::uint64_t next_flight=0,revision=0;
    int cache_size=256;
    double minimum=3,maximum=4;
    bool vector_first=true,querying=false;
    std::string error;
    std::array<std::uint64_t,6> counts{};
    ii::core::Observable<std::uint64_t> error_epoch{0},vector_epoch{0},minimum_epoch{0},maximum_epoch{0},cache_epoch{0},revision_epoch{0};
    auto& epoch(SvgManagerEvent event){
        switch(event){case SvgManagerEvent::last_error:return error_epoch;case SvgManagerEvent::vector_first:return vector_epoch;
        case SvgManagerEvent::minimum_scale:return minimum_epoch;case SvgManagerEvent::maximum_scale:return maximum_epoch;
        case SvgManagerEvent::cache_size:return cache_epoch;case SvgManagerEvent::revision:return revision_epoch;}
        throw std::logic_error("Unknown SVG event");
    }
    void emit(SvgManagerEvent event){
        if(closed)return;
        const bool previous=notifying;notifying=true;
        struct Restore final{bool& target;bool previous;~Restore(){target=previous;}}restore{notifying,previous};
        std::optional<ii::core::Error> failure;
        ii::core::ObservableReadScope::pause([&]{
            try{epoch(event).set(++counts[static_cast<std::size_t>(event)]);}catch(...){failure=ii::core::Error{ii::core::ErrorCode::failed,"SVG observer failed after commit",std::current_exception()};}
            if(!closed)send(event,failure);
        });
        if(failure){if(failure->cause)std::rethrow_exception(failure->cause);throw std::runtime_error(failure->message);}
    }
    void set_error(std::string value){if(closed||error==value)return;error=std::move(value);emit(SvgManagerEvent::last_error);}
    void changed(){++revision;emit(SvgManagerEvent::revision);}
    template<class F>void continue_after(F action,std::exception_ptr& failure){try{action();}catch(...){if(!failure)failure=std::current_exception();}}
    void write(){if(auto ready=writable();!ready)throw std::logic_error(ready.error().message);if(querying)throw std::logic_error("SVG query is active");}
    bool clear(){
        const bool any=!rasters.empty()||!sources.empty();rasters.clear();raster_index.clear();sources.clear();source_index.clear();raster_bytes=source_bytes=0;return any;
    }
    void trim_rasters(){
        while(!rasters.empty()&&(rasters.size()>static_cast<std::size_t>(cache_size)||raster_bytes>limits.raster_bytes)){
            raster_bytes-=rasters.front()->bytes;raster_index.erase(rasters.front()->key);rasters.pop_front();
        }
    }
    void store_source(std::string key,std::string payload){
        if(auto existing=source_index.find(key);existing!=source_index.end()){
            source_bytes-=existing->second->bytes();sources.erase(existing->second);source_index.erase(existing);
        }
        sources.push_back({std::move(key),std::move(payload)});source_bytes+=sources.back().bytes();source_index.emplace(sources.back().key,std::prev(sources.end()));
        while(source_bytes>limits.source_bytes||sources.size()>4096){source_bytes-=sources.front().bytes();source_index.erase(sources.front().key);sources.pop_front();}
    }
    void complete(std::uint64_t id,ii::core::Result<std::string> result){
        if(owner!=std::this_thread::get_id()||closed)return;
        const auto found=flights.find(id);if(found==flights.end())return;
        if(found->second.admitting){found->second.inline_completion=true;return;}
        auto flight=std::move(found->second);flights.erase(found);pending.erase(flight.key);
        if(!result){set_error(result.error().message.empty()?"SVG request failed":result.error().message);return;}
        if(result->empty()){set_error("Received empty SVG payload");return;}
        if(result->size()>1024*1024||sizeof(Source)+flight.key.size()*2+result->size()>limits.source_bytes){set_error("SVG source exceeds its cache byte limit");return;}
        store_source(std::move(flight.key),std::move(*result));
        std::exception_ptr failure;continue_after([&]{set_error({});},failure);
        if(!closed)continue_after([&]{changed();},failure);
        if(failure)std::rethrow_exception(failure);
    }
    void request(const std::string& key){
        if(pending.contains(key))return;
        if(flights.size()>=limits.pending_requests){set_error("SVG request capacity exhausted");return;}
        const auto id=++next_flight;flights.emplace(id,Flight{key});pending.insert(key);
        const auto weak=weak_from_this();
        const auto invoke=[&]()->ii::core::Result<std::shared_ptr<SvgRequest>>{
            try{return source->fetch(key,[weak,id](auto result){if(auto p=weak.lock())p->complete(id,std::move(result));});}
            catch(const std::exception& error){return ii::core::fail(ii::core::ErrorCode::failed,error.what(),std::current_exception());}
            catch(...){return ii::core::fail(ii::core::ErrorCode::failed,"SVG source request threw",std::current_exception());}
        };
        auto result=invoke();
        auto found=flights.find(id);
        if(closed||found==flights.end()){if(result&&*result)(*result)->cancel();return;}
        if(!result||!*result||found->second.inline_completion){
            if(result&&*result)(*result)->cancel();pending.erase(key);flights.erase(found);
            set_error(!result?(result.error().message.empty()?"SVG request failed":result.error().message):"SVG source completion must be deferred");return;
        }
        found->second.request=std::move(*result);found->second.admitting=false;
    }
    std::optional<std::string> payload(std::string_view url){
        const auto invoke=[&]()->ii::core::Result<SvgResource>{
            try{return source->read(url);}
            catch(const std::exception& error){return ii::core::fail(ii::core::ErrorCode::failed,error.what(),std::current_exception());}
            catch(...){return ii::core::fail(ii::core::ErrorCode::failed,"SVG source read threw",std::current_exception());}
        };
        auto resource=invoke();if(closed)return std::nullopt;
        if(!resource){set_error(resource.error().message.empty()?"Failed to load SVG payload":resource.error().message);return std::nullopt;}
        if(resource->payload)return std::move(*resource->payload);
        if(resource->key.empty()||resource->key.size()>8192){set_error("Invalid or oversized SVG request key");return std::nullopt;}
        if(auto found=source_index.find(resource->key);found!=source_index.end()){
            sources.splice(sources.end(),sources,found->second);return found->second->payload;
        }
        request(resource->key);if(!closed&&pending.contains(resource->key)&&error.empty())set_error("SVG request pending");
        return std::nullopt;
    }
    std::shared_ptr<Raster> raster(std::string_view url,int logical,double scale){
        (void)minimum_epoch.get();(void)maximum_epoch.get();(void)cache_epoch.get();(void)revision_epoch.get();
        finite(scale);logical=std::clamp(logical,1,2048);
        if(scale<=0){const double device=source->device_scale();scale=std::max(std::isfinite(device)?device:1.,minimum);}
        scale=std::clamp(scale,1.,maximum);
        const auto key=cache_key(url,logical,scale);
        if(auto found=raster_index.find(key);found!=raster_index.end()){
            rasters.splice(rasters.end(),rasters,found->second);return *found->second;
        }
        auto bytes=payload(url);if(closed)return {};
        if(!bytes||bytes->empty()){if(error.empty())set_error("Failed to load SVG payload");return {};}
        const auto edge=std::round(double(logical)*scale);
        if(!std::isfinite(edge)||edge<1||edge>ii::gui::maximum_image_dimension){set_error("SVG raster exceeds its dimension limit");return {};}
        const auto pixels=static_cast<std::uint32_t>(edge);
        auto image=ii::gui::svg_image(*bytes,pixels,pixels,ii::gui::ImageBudget{64ull*1024*1024});
        if(!image){set_error(image.error().message);return {};}
        auto result=std::make_shared<Raster>(Raster{key,{},std::move(*image)});
        result->bytes=sizeof(Raster)+result->key.size()*2+result->image->rgba().size();
        if(cache_size&&result->bytes<=limits.raster_bytes){
            rasters.push_back(result);raster_bytes+=result->bytes;raster_index.emplace(key,std::prev(rasters.end()));trim_rasters();
        }
        return result;
    }
    std::string encoded(const std::shared_ptr<Raster>& raster){
        if(!raster->data.empty())return raster->data;
        ii::visual::image::Limits codec_limits;codec_limits.output_bytes=320ull*1024*1024;
        auto png=ii::visual::image::encode_png({raster->image->width(),raster->image->height(),ii::visual::image::PixelFormat::rgba8,0,raster->image->rgba()},codec_limits);
        if(!png){set_error(png.error().message);return {};}
        const auto view=std::string_view(reinterpret_cast<const char*>(png->data()),png->size());
        raster->data="data:image/png;base64,"+ii::network::wire::base64(view);raster->bytes+=raster->data.size();
        if(auto cached=raster_index.find(raster->key);cached!=raster_index.end()&&*cached->second==raster){
            raster_bytes+=raster->data.size();trim_rasters();
        }
        return raster->data;
    }
    void retire(){
        closed=true;receivers.clear();pending.clear();
        auto active=std::move(flights);for(auto& [id,flight]:active){(void)id;if(flight.request)flight.request->cancel();}
        clear();
    }
};
SvgManager::SvgManager():SvgManager(native_svg_source()){}
SvgManager::SvgManager(ii::runtime::EventLoop& loop,SvgSourceOptions options):SvgManager(native_svg_source(&loop,std::move(options))){}
SvgManager::SvgManager(std::shared_ptr<SvgSource> source,SvgCacheLimits limits):impl_(std::make_shared<Impl>()){
    if(!source||!limits.source_bytes||limits.source_bytes>64ull*1024*1024||limits.raster_bytes>512ull*1024*1024||!limits.pending_requests||limits.pending_requests>4096)throw std::invalid_argument("Invalid SVG service or cache limits");
    impl_->source=std::move(source);impl_->limits=limits;
}
SvgManager::~SvgManager(){impl_->retire();}
std::string SvgManager::lastError()const{auto p=impl_;p->readable();(void)p->error_epoch.get();return p->error;}
bool SvgManager::vectorFirst()const{auto p=impl_;p->readable();(void)p->vector_epoch.get();return p->vector_first;}
double SvgManager::minimumScale()const{auto p=impl_;p->readable();(void)p->minimum_epoch.get();return p->minimum;}
double SvgManager::maximumScale()const{auto p=impl_;p->readable();(void)p->maximum_epoch.get();return p->maximum;}
int SvgManager::cacheSize()const{auto p=impl_;p->readable();(void)p->cache_epoch.get();return p->cache_size;}
std::uint64_t SvgManager::revision()const{auto p=impl_;p->readable();(void)p->revision_epoch.get();return p->revision;}
void SvgManager::setVectorFirst(bool value){auto p=impl_;p->write();if(p->vector_first==value)return;p->vector_first=value;p->clear();p->pending.clear();std::exception_ptr failure;p->continue_after([&]{p->changed();},failure);if(!p->closed)p->continue_after([&]{p->emit(SvgManagerEvent::vector_first);},failure);if(failure)std::rethrow_exception(failure);}
void SvgManager::setMinimumScale(double value){auto p=impl_;p->write();finite(value);value=std::clamp(value,1.,p->maximum);if(pd::fuzzy_equal(value,p->minimum))return;p->minimum=value;p->emit(SvgManagerEvent::minimum_scale);}
void SvgManager::setMaximumScale(double value){auto p=impl_;p->write();finite(value);value=std::max(1.,value);if(pd::fuzzy_equal(value,p->maximum))return;p->maximum=value;std::exception_ptr failure;if(p->minimum>value){p->minimum=value;p->continue_after([&]{p->emit(SvgManagerEvent::minimum_scale);},failure);}if(!p->closed)p->continue_after([&]{p->emit(SvgManagerEvent::maximum_scale);},failure);if(failure)std::rethrow_exception(failure);}
void SvgManager::setCacheSize(int value){auto p=impl_;p->write();value=std::clamp(value,0,4096);if(p->cache_size==value)return;p->cache_size=value;std::exception_ptr failure;if(!value){if(p->clear())p->continue_after([&]{p->changed();},failure);}else p->trim_rasters();if(!p->closed)p->continue_after([&]{p->emit(SvgManagerEvent::cache_size);},failure);if(failure)std::rethrow_exception(failure);}
void SvgManager::clearCache(){auto p=impl_;p->write();if(p->clear())p->changed();}
void SvgManager::ensureMinimumScale(double value){auto p=impl_;p->write();finite(value);value=std::max(1.,value);std::exception_ptr failure;if(value>p->maximum){p->maximum=value;p->continue_after([&]{p->emit(SvgManagerEvent::maximum_scale);},failure);}if(!p->closed&&value>p->minimum){p->minimum=value;p->continue_after([&]{p->emit(SvgManagerEvent::minimum_scale);},failure);}if(failure)std::rethrow_exception(failure);}
double SvgManager::deviceScale()const{auto p=impl_;p->readable();const auto value=p->source->device_scale();return std::isfinite(value)?std::max(1.,value):1.;}
std::string SvgManager::icon(std::string_view raw,int logical,double scale){
    auto p=impl_;p->readable();if(p->querying)throw std::logic_error("Recursive SVG query");pd::Flag guard{p->querying};p->set_error({});
    auto url=pd::normalized_text(raw);if(p->closed)return {};
    if(url.empty()){p->set_error("Empty SVG URL");return {};}
    (void)p->vector_epoch.get();(void)p->revision_epoch.get();
    if(svg_vector_source(url)){
        if(lower(url).starts_with("data:image/svg")&&url.find(',')==url.npos){p->set_error("Malformed SVG data URL");return {};}
        if(p->vector_first)return url;
    }
    auto result=p->raster(url,logical,scale);return result?p->encoded(result):std::string{};
}
std::shared_ptr<const ii::gui::RasterImage> SvgManager::image(std::string_view raw,int logical,double scale){
    auto p=impl_;p->readable();if(p->querying)throw std::logic_error("Recursive SVG query");pd::Flag guard{p->querying};p->set_error({});
    auto url=pd::normalized_text(raw);if(p->closed)return {};
    if(url.empty()){p->set_error("Empty SVG URL");return {};}
    auto result=p->raster(url,logical,scale);return result?result->image:nullptr;
}
ii::core::Result<void> SvgManager::observe(std::shared_ptr<const SvgManagerChangePort> port){return impl_->observe(std::move(port));}
}
