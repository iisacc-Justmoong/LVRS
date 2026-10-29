#include "SvgSource.h"
#include "Icons.h"
#include <iiFileProviderNative.h>
#include <iiNetwork/Http.h>
#include <iiNetwork/Wire.h>
#include <iiSDKCore/Core/Text/Utf8.h>
#include <curl/curl.h>
#include <zlib.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <thread>

namespace lvrs {
namespace {
using ii::core::ErrorCode;
template<class T>using Result=ii::core::Result<T>;
std::string lower(std::string_view value){std::string out(value);for(auto& c:out)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return out;}
struct Url final {
    CURLU* handle=curl_url();
    Url(){if(!handle)throw std::bad_alloc();}
    ~Url(){curl_url_cleanup(handle);}
    bool set(std::string_view value){if(value.find('\0')!=value.npos||!ii::core::valid_utf8(value))return false;const std::string text(value);return curl_url_set(handle,CURLUPART_URL,text.c_str(),CURLU_NON_SUPPORT_SCHEME)==CURLUE_OK;}
    std::string part(CURLUPart part,unsigned flags=0)const{
        char* text=nullptr;if(curl_url_get(handle,part,&text,flags)!=CURLUE_OK)return {};
        std::string result(text);curl_free(text);return result;
    }
};
Result<std::string> unpack(std::string bytes,std::size_t maximum){
    if(bytes.size()>maximum)return ii::core::fail(ErrorCode::full,"SVG source exceeds its byte limit");
    if(bytes.size()<2||static_cast<unsigned char>(bytes[0])!=0x1f||static_cast<unsigned char>(bytes[1])!=0x8b)return bytes;
    z_stream stream{};
    if(inflateInit2(&stream,15+16)!=Z_OK)return ii::core::fail(ErrorCode::failed,"Cannot initialize SVGZ decoder");
    struct End final{z_stream& stream;~End(){inflateEnd(&stream);}}end{stream};
    stream.next_in=reinterpret_cast<Bytef*>(bytes.data());stream.avail_in=static_cast<uInt>(bytes.size());
    std::string output;std::array<char,8192> buffer{};
    for(;;){
        stream.next_out=reinterpret_cast<Bytef*>(buffer.data());stream.avail_out=static_cast<uInt>(buffer.size());
        const int code=inflate(&stream,Z_NO_FLUSH);const auto count=buffer.size()-stream.avail_out;
        if(count>maximum-output.size())return ii::core::fail(ErrorCode::full,"SVGZ expands beyond its byte limit");
        output.append(buffer.data(),count);
        if(code==Z_STREAM_END){if(stream.avail_in)return ii::core::fail(ErrorCode::invalid_argument,"Trailing SVGZ data");return output;}
        if(code!=Z_OK||(!count&&!stream.avail_in))return ii::core::fail(ErrorCode::invalid_argument,"Invalid SVGZ payload");
    }
}
class RemoteRequest final:public SvgRequest,public std::enable_shared_from_this<RemoteRequest> {
    ii::network::Http http_;
    ii::runtime::EventLoop::Sender sender_;
    SvgCompletion completion_;
    std::string current_;
    std::size_t maximum_;
    unsigned redirects_=0;
    bool finished_=false;
    ii::runtime::TaskToken queued_;
    ii::runtime::Clock::time_point deadline_=ii::runtime::Clock::now()+std::chrono::seconds(6);
    void finish(Result<std::string> result){
        if(finished_)return;
        finished_=true;auto completion=std::move(completion_);
        if(completion)completion(std::move(result));
    }
    void received(ii::network::HttpResponse response){
        if(finished_)return;
        if(response.timed_out){finish(ii::core::fail(ErrorCode::failed,"SVG request timed out"));return;}
        if(!response.error.empty()){finish(ii::core::fail(ErrorCode::failed,response.error));return;}
        if(response.status>=300&&response.status<400){
            const auto location=response.header_fields.get("location");
            if(!location||location->empty()||redirects_++==50){finish(ii::core::fail(ErrorCode::failed,"Invalid SVG redirect"));return;}
            Url target;
            if(!target.set(current_)){finish(ii::core::fail(ErrorCode::invalid_argument,"Invalid SVG URL"));return;}
            const auto previous=target.part(CURLUPART_SCHEME);
            if(!target.set(*location)){finish(ii::core::fail(ErrorCode::invalid_argument,"Invalid SVG redirect URL"));return;}
            const auto next=target.part(CURLUPART_SCHEME);
            if((next!="http"&&next!="https")||(previous=="https"&&next!="https")){
                finish(ii::core::fail(ErrorCode::unsupported,"Unsafe SVG redirect"));return;
            }
            current_=target.part(CURLUPART_URL);
            // A fresh request starts after Http's completion has unwound.
            if(!schedule())finish(ii::core::fail(ErrorCode::full,"SVG redirect queue is full"));
            return;
        }
        if(!response.ok()){finish(ii::core::fail(ErrorCode::failed,"SVG HTTP status "+std::to_string(response.status)));return;}
        if(response.body.empty()){finish(ii::core::fail(ErrorCode::failed,"Received empty SVG payload"));return;}
        finish(unpack(std::move(response.body),maximum_));
    }
public:
    RemoteRequest(ii::runtime::EventLoop& loop,std::string url,std::size_t maximum,SvgCompletion completion)
        :http_(loop),sender_(loop.sender()),completion_(std::move(completion)),current_(std::move(url)),maximum_(maximum){}
    ~RemoteRequest(){cancel();}
    bool schedule(){
        const auto weak=weak_from_this();auto accepted=sender_.post([weak]{if(auto self=weak.lock())self->begin();});
        if(!accepted)return false;queued_=accepted.token;return true;
    }
    void begin(){
        queued_={};
        if(finished_)return;
        const auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(deadline_-ii::runtime::Clock::now());
        if(remaining.count()<=0){finish(ii::core::fail(ErrorCode::failed,"SVG request timed out"));return;}
        ii::network::HttpRequest request;request.url=current_;request.timeout=remaining;
        request.initial_response_timeout=std::chrono::seconds(5);request.inactivity_timeout=std::chrono::seconds(5);
        request.maximum_bytes=maximum_;request.accept_compression=true;request.follow_redirects=false;
        const auto weak=weak_from_this();auto result=http_.request(std::move(request),[weak](auto response){if(auto self=weak.lock())self->received(std::move(response));});
        if(!result)finish(ii::core::fail(ErrorCode::failed,result.error().message));
    }
    void cancel()noexcept override{
        if(finished_)return;finished_=true;completion_={};
        (void)sender_.cancel(queued_);queued_={};
        try{http_.cancel();}catch(...){/* A retired source cannot deliver again. */}
    }
};
class NativeSource final:public SvgSource {
    ii::runtime::EventLoop* loop_;
    SvgSourceOptions options_;
    const std::thread::id owner_=std::this_thread::get_id();
    void owner()const{if(owner_!=std::this_thread::get_id())throw std::logic_error("SVG source requires its owner thread");}
    Result<SvgResource> ready(std::string_view key,std::string value){
        auto decoded=unpack(std::move(value),options_.maximum_source_bytes);
        if(!decoded)return std::unexpected(decoded.error());
        return SvgResource{std::string(key),std::move(*decoded)};
    }
    Result<SvgResource> embedded(std::string_view source){
        std::string path(source);
        if(path.starts_with(":/"))path="qrc"+path;
        else if(lower(path.substr(0,4))=="qrc:"){path="qrc:"+path.substr(4);if(auto end=path.find_first_of("?#");end!=path.npos)path.erase(end);}
        constexpr std::string_view icons="qrc:/qt/qml/LVRS/resources/iconset/";
        if(path.starts_with(icons)){
            auto result=Icons::data(path);
            if(!result)return std::unexpected(result.error());
            return ready(source,std::string(*result));
        }
        if(options_.resource){auto result=options_.resource(source);if(!result)return std::unexpected(result.error());return ready(source,std::move(*result));}
        return ii::core::fail(ErrorCode::not_found,"Unknown native SVG resource");
    }
public:
    NativeSource(ii::runtime::EventLoop* loop,SvgSourceOptions options):loop_(loop),options_(std::move(options)){
        if(!options_.maximum_source_bytes||options_.maximum_source_bytes>1024*1024)throw std::invalid_argument("SVG source byte limit must be 1..1048576");
    }
    double device_scale()const override{owner();return options_.device_scale?options_.device_scale():1.;}
    Result<SvgResource> read(std::string_view source)override{
        owner();
        try{
            if(source.empty()||source.find('\0')!=source.npos||!ii::core::valid_utf8(source))return ii::core::fail(ErrorCode::invalid_argument,"Invalid SVG URL");
            if(source.starts_with(":/")||lower(source.substr(0,4))=="qrc:")return embedded(source);
            if(lower(source.substr(0,5))=="data:"){
                const auto comma=source.find(',');
                if(comma==source.npos)return ii::core::fail(ErrorCode::invalid_argument,"Malformed SVG data URL");
                if(source.size()-comma-1>options_.maximum_source_bytes*2+16)return ii::core::fail(ErrorCode::full,"SVG data URL exceeds its byte limit");
                auto header=lower(source.substr(0,comma));auto payload=source.substr(comma+1);
                auto bytes=header.find(";base64")!=header.npos?ii::network::wire::unbase64(payload):ii::network::wire::percent_decode(payload);
                if(!bytes)return ii::core::fail(ErrorCode::invalid_argument,"Invalid SVG data encoding");
                return ready(source,std::move(*bytes));
            }
            if(source.find("://")==source.npos&&lower(source.substr(0,5))!="file:")
                return ready(source,iiFileProvider::native::File::read(std::filesystem::u8path(source),options_.maximum_source_bytes));
            Url url;if(!url.set(source))return ii::core::fail(ErrorCode::invalid_argument,"Invalid SVG URL");
            const auto scheme=url.part(CURLUPART_SCHEME);
            if(scheme=="file"){
                const auto host=url.part(CURLUPART_HOST);auto decoded=ii::network::wire::percent_decode(url.part(CURLUPART_PATH));
                if(!decoded)return ii::core::fail(ErrorCode::invalid_argument,"Invalid SVG file encoding");
                auto path=std::move(*decoded);
                if(path.find('\0')!=path.npos)return ii::core::fail(ErrorCode::invalid_argument,"Invalid SVG file path");
#if defined(_WIN32)
                if(path.size()>2&&path[0]=='/'&&path[2]==':')path.erase(0,1);
                if(!host.empty()&&host!="localhost")path="//"+host+path;
#else
                if(!host.empty()&&host!="localhost")return ii::core::fail(ErrorCode::invalid_argument,"Nonlocal SVG file URL");
#endif
                return ready(source,iiFileProvider::native::File::read(std::filesystem::u8path(path),options_.maximum_source_bytes));
            }
            if(scheme!="http"&&scheme!="https")return ii::core::fail(ErrorCode::unsupported,"Unsupported SVG URL scheme");
            return SvgResource{url.part(CURLUPART_URL),std::nullopt};
        }catch(const std::exception& error){return ii::core::fail(ErrorCode::failed,error.what(),std::current_exception());}
    }
    Result<std::shared_ptr<SvgRequest>> fetch(std::string_view key,SvgCompletion completion)override{
        owner();
        if(!loop_)return ii::core::fail(ErrorCode::unsupported,"SVG network access requires an application EventLoop");
        if(!completion)return ii::core::fail(ErrorCode::invalid_argument,"SVG request requires a completion");
        Url url;if(!url.set(key)||(url.part(CURLUPART_SCHEME)!="http"&&url.part(CURLUPART_SCHEME)!="https"))return ii::core::fail(ErrorCode::invalid_argument,"Invalid remote SVG URL");
        auto request=std::make_shared<RemoteRequest>(*loop_,url.part(CURLUPART_URL),options_.maximum_source_bytes,std::move(completion));
        // Admission itself is queued, including validation/transport failures.
        if(!request->schedule())return ii::core::fail(ErrorCode::full,"SVG request queue is full");
        return std::shared_ptr<SvgRequest>(std::move(request));
    }
};
}
std::shared_ptr<SvgSource> native_svg_source(ii::runtime::EventLoop* loop,SvgSourceOptions options){return std::make_shared<NativeSource>(loop,std::move(options));}
bool svg_vector_source(std::string_view source){
    if(source.empty()||source.find('\0')!=source.npos||!ii::core::valid_utf8(source))return false;
    const auto folded=lower(source);if(folded.starts_with("data:image/svg"))return true;
    const auto suffix=[](std::string_view path){return path.ends_with(".svg")||path.ends_with(".svgz");};
    if(source.starts_with(":/"))return suffix(folded);
    std::string path;
    if(folded.starts_with("qrc:"))path=std::string(source.substr(4));
    else if(source.find("://")!=source.npos){Url url;if(!url.set(source))return false;path=url.part(CURLUPART_PATH);}
    else path=std::string(source);
    if(auto end=path.find_first_of("?#");end!=path.npos)path.erase(end);
    auto decoded=ii::network::wire::percent_decode(path);return decoded&&suffix(lower(*decoded));
}
}
