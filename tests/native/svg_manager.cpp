#include <LVRS/SvgManager.h>
#include <LVRS/Icons.h>
#include <iiVisualCodec/ImageCodec.h>
#include <iiNetwork/Wire.h>
#include <iiFileProviderNative.h>
#include <iiSDKCore/Runtime/EventLoop.h>
#include <zlib.h>
#include <filesystem>
#include <future>
#include <iostream>
#include <map>
#include <limits>
#include <thread>
#include <vector>
using namespace ii;
namespace {
constexpr auto svg=R"(<svg xmlns="http://www.w3.org/2000/svg" width="8" height="8"><rect width="8" height="8" fill="red"/></svg>)";
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> result){if(!result)throw std::runtime_error(result.error().message);return std::move(*result);}
void good(core::Result<void> result){if(!result)throw std::runtime_error(result.error().message);}
struct Request final:lvrs::SvgRequest {
    lvrs::SvgCompletion completion;bool cancelled=false;
    explicit Request(lvrs::SvgCompletion value):completion(std::move(value)){}
    void cancel()noexcept override{cancelled=true;}
};
struct Source final:lvrs::SvgSource {
    double scale=2;mutable int scale_reads=0;bool fail=false,inline_reply=false,throw_read=false,throw_fetch=false;
    std::map<std::string,int> reads;std::vector<std::shared_ptr<Request>> requests;
    double device_scale()const override{++scale_reads;return scale;}
    core::Result<lvrs::SvgResource> read(std::string_view url)override{
        if(throw_read)throw std::runtime_error("Read exception");
        ++reads[std::string(url)];
        if(url.starts_with("http"))return lvrs::SvgResource{std::string(url),std::nullopt};
        return lvrs::SvgResource{std::string(url),std::string(url=="bad"?"invalid":svg)};
    }
    core::Result<std::shared_ptr<lvrs::SvgRequest>> fetch(std::string_view,lvrs::SvgCompletion completion)override{
        if(throw_fetch)throw std::runtime_error("Fetch exception");
        if(fail)return core::fail(core::ErrorCode::failed,"Transport failure");
        auto request=std::make_shared<Request>(std::move(completion));requests.push_back(request);
        if(inline_reply)request->completion(std::string(svg));
        return std::shared_ptr<lvrs::SvgRequest>(request);
    }
    void complete(std::size_t i,std::string value=svg){requests.at(i)->completion(std::move(value));}
};
struct Fixture final{std::shared_ptr<Source> source=std::make_shared<Source>();lvrs::SvgManager policy{source};};
void defaults(){Fixture f;check(f.policy.vectorFirst()&&f.policy.minimumScale()==3&&f.policy.maximumScale()==4&&f.policy.cacheSize()==256&&f.policy.revision()==0&&f.policy.lastError().empty()&&f.policy.deviceScale()==2,"SVG defaults differ from the original source");}
void vector(){Fixture f;check(f.policy.icon("  https://example.test/A.SVG?x=1#v ")=="https://example.test/A.SVG?x=1#v"&&f.source->reads.empty()&&f.source->requests.empty(),"Vector-first path performed I/O or changed spelling");check(f.policy.icon("data:image/svg+xml,<svg/>")=="data:image/svg+xml,<svg/>","SVG data vector source was not retained");}
void malformed(){Fixture f;check(f.policy.icon("").empty()&&f.policy.lastError()=="Empty SVG URL","Missing empty-source error");check(f.policy.icon("data:image/svg+xml").empty()&&f.policy.lastError()=="Malformed SVG data URL","Malformed vector data URL accepted");check(!f.policy.icon("x.svg").empty()&&f.policy.lastError().empty(),"Successful query did not clear last error");}
void raster(){Fixture f;f.policy.setVectorFirst(false);auto value=f.policy.icon("x.svg");check(value.starts_with("data:image/png;base64,"),"Raster mode did not encode PNG");auto bytes=network::wire::unbase64(std::string_view(value).substr(22));check(bytes.has_value(),"PNG URL encoding is invalid");auto decoded=image::decode({reinterpret_cast<const std::uint8_t*>(bytes->data()),bytes->size()});check(decoded&&decoded->width==54&&decoded->height==54&&decoded->rgba[0]==255&&decoded->rgba[3]==255,"Raster dimensions/alpha/content changed");}
void scales(){Fixture f;f.policy.setMinimumScale(9);check(f.policy.minimumScale()==4,"Minimum scale was not bounded");f.policy.setMaximumScale(2);check(f.policy.maximumScale()==2&&f.policy.minimumScale()==2,"Maximum scale did not clamp minimum");f.policy.ensureMinimumScale(5);check(f.policy.minimumScale()==5&&f.policy.maximumScale()==5,"Ensure-minimum failed to grow both bounds");f.policy.setMinimumScale(-1);check(f.policy.minimumScale()==1,"Minimum scale floor changed");}
void explicit_scale(){Fixture f;auto image=f.policy.image("a.svg",18,2);check(image&&image->width()==36&&f.source->scale_reads==0,"Explicit scale queried device state or was ignored");check(f.policy.image("a.svg",0,1)->width()==1,"Logical size lower clamp changed");check(f.policy.image("a.svg",3000,1)->width()==2048,"Logical size upper clamp changed");}
void identity(){Fixture f;auto first=f.policy.image("x.svg");auto second=f.policy.image("x.svg");check(first&&first==second&&f.source->reads["x.svg"]==1,"Repeated image query lost raster identity or reread the source");}
void quantized(){Fixture f;auto first=f.policy.image("x.svg",18,1.0001);check(first==f.policy.image("x.svg",18,1.0004)&&f.source->reads["x.svg"]==1,"Original three-decimal cache key behavior changed");}
void lru(){Fixture f;f.policy.setCacheSize(2);auto a=f.policy.image("a.svg"),b=f.policy.image("b.svg");check(a==f.policy.image("a.svg"),"Cache touch lost A");(void)f.policy.image("c.svg");check(b!=f.policy.image("b.svg")&&f.source->reads["b.svg"]==2,"Raster LRU evicted the touched entry");}
void disabled_cache(){Fixture f;f.policy.setCacheSize(0);auto a=f.policy.image("a.svg");check(a!=f.policy.image("a.svg")&&f.source->reads["a.svg"]==2,"Disabled cache retained raster data");}
void clear(){Fixture f;f.policy.clearCache();check(f.policy.revision()==0,"Empty clear changed revision");auto a=f.policy.image("a.svg");f.policy.clearCache();check(f.policy.revision()==1&&a!=f.policy.image("a.svg"),"Clear did not release cached ownership");f.policy.setCacheSize(0);check(f.policy.revision()==2,"Disabling a populated cache did not publish clear");}
void bounds(){Fixture f;f.policy.setCacheSize(5000);check(f.policy.cacheSize()==4096,"Cache entry upper bound changed");f.policy.setCacheSize(-1);check(f.policy.cacheSize()==0,"Cache entry lower bound changed");check(!f.policy.image("bad")&&!f.policy.lastError().empty(),"Invalid SVG accepted");f.policy.setMaximumScale(100);check(!f.policy.image("a.svg",2048,100)&&f.policy.lastError().find("dimension")!=std::string::npos,"Unbounded raster allocation accepted");bool failed=false;try{f.policy.setMinimumScale(std::numeric_limits<double>::quiet_NaN());}catch(const std::invalid_argument&){failed=true;}check(failed,"Nonfinite scale accepted");}
void memory_budget(){auto source=std::make_shared<Source>();lvrs::SvgManager p(source,{.raster_bytes=1});auto a=p.image("x.svg");check(a!=p.image("x.svg")&&source->reads["x.svg"]==2,"Raster cache byte budget ignored");}
void pending(){Fixture f;check(!f.policy.image("https://host/a")&&!f.policy.image("https://host/a")&&f.source->requests.size()==1&&f.policy.lastError()=="SVG request pending","Pending request was not coalesced");}
void completion(){Fixture f;(void)f.policy.image("https://host/a");f.source->complete(0);check(f.policy.revision()==1&&f.policy.lastError().empty(),"Successful network completion did not publish");auto a=f.policy.image("https://host/a");check(a&&a->width()==54&&a==f.policy.image("https://host/a")&&f.source->requests.size()==1,"Completed payload was not reused");}
void failure(){Fixture f;f.source->fail=true;check(!f.policy.image("https://host/a")&&f.policy.lastError()=="Transport failure"&&f.policy.revision()==0,"Transport admission error hidden");f.source->fail=false;(void)f.policy.image("https://host/a");f.source->requests[0]->completion(core::fail(core::ErrorCode::failed,"Async error"));check(f.policy.lastError()=="Async error"&&f.policy.revision()==0,"Asynchronous error hidden");}
void exceptions(){Fixture f;f.source->throw_read=true;check(!f.policy.image("a.svg")&&f.policy.lastError()=="Read exception","Read exception escaped its source contract");f.source->throw_read=false;f.source->throw_fetch=true;check(!f.policy.image("https://host/a")&&f.policy.lastError()=="Fetch exception","Fetch exception escaped its source contract");f.source->throw_fetch=false;(void)f.policy.image("https://host/a");check(f.source->requests.size()==1,"Failed admission left a permanently pending request");f.source->complete(0);check(f.policy.revision()==1,"Source failed to recover after an exception");}
void clear_pending(){Fixture f;(void)f.policy.image("https://host/a");f.policy.clearCache();check(f.policy.revision()==0&&!f.source->requests[0]->cancelled,"Original clear incorrectly canceled an active source request");f.source->complete(0);check(f.policy.revision()==1&&f.policy.image("https://host/a"),"Pending completion was lost after clear");}
void request_limit(){auto source=std::make_shared<Source>();lvrs::SvgManager p(source,{.pending_requests=1});(void)p.image("https://host/a");check(!p.image("https://host/b")&&source->requests.size()==1&&p.lastError().find("capacity")!=std::string::npos,"Pending request capacity ignored");}
void source_limit(){auto source=std::make_shared<Source>();lvrs::SvgManager p(source,{.source_bytes=8});(void)p.image("https://host/a");source->complete(0);check(p.revision()==0&&p.lastError().find("byte limit")!=std::string::npos,"Oversized remote payload published a retry loop");}
void events(){Fixture f;std::vector<lvrs::SvgManagerEvent> events;auto port=std::make_shared<lvrs::SvgManagerChangePort>([&](auto event){events.push_back(event);});good(f.policy.observe(port));f.policy.ensureMinimumScale(5);check(events==std::vector{lvrs::SvgManagerEvent::maximum_scale,lvrs::SvgManagerEvent::minimum_scale},"Scale event ordering changed");events.clear();f.policy.setVectorFirst(false);check(events==std::vector{lvrs::SvgManagerEvent::revision,lvrs::SvgManagerEvent::vector_first},"Vector mode invalidation order changed");}
void observer_failure(){Fixture f;std::vector<lvrs::SvgManagerEvent> events;auto bad=std::make_shared<lvrs::SvgManagerChangePort>([](auto){throw std::runtime_error("receiver");});auto good_port=std::make_shared<lvrs::SvgManagerChangePort>([&](auto e){events.push_back(e);});good(f.policy.observe(bad));good(f.policy.observe(good_port));bool failed=false;try{f.policy.setVectorFirst(false);}catch(...){failed=true;}check(failed&&!f.policy.vectorFirst()&&events==std::vector{lvrs::SvgManagerEvent::revision,lvrs::SvgManagerEvent::vector_first},"Receiver failure hid a committed SVG change");}
void owner(){Fixture f;check(std::async(std::launch::async,[&]{try{f.policy.clearCache();return false;}catch(const std::logic_error&){return true;}}).get(),"Foreign-thread SVG mutation admitted");}
void foreign_completion(){Fixture f;(void)f.policy.image("https://host/a");std::async(std::launch::async,[&]{f.source->complete(0);}).get();check(f.policy.revision()==0&&f.policy.lastError()=="SVG request pending","Foreign completion mutated owner-thread state");f.source->complete(0);check(f.policy.revision()==1,"Foreign delivery discarded the valid owner request");}
void retirement(){auto source=std::make_shared<Source>();auto p=std::make_unique<lvrs::SvgManager>(source);(void)p->image("https://host/a");p.reset();check(source->requests[0]->cancelled,"SVG retirement did not cancel owned requests");source->complete(0);}
void retirement_in_notify(){auto p=std::make_unique<lvrs::SvgManager>(std::make_shared<Source>());int late=0;auto first=std::make_shared<lvrs::SvgManagerChangePort>([&](auto){p.reset();});auto second=std::make_shared<lvrs::SvgManagerChangePort>([&](auto){++late;});good(p->observe(first));good(p->observe(second));auto* borrowed=p.get();(void)borrowed->icon("");check(!p&&late==0,"Retired SVG policy delivered later observers");}
void inline_completion(){Fixture f;f.source->inline_reply=true;check(!f.policy.image("https://host/a")&&f.policy.lastError().find("deferred")!=std::string::npos&&f.source->requests[0]->cancelled,"Inline completion bypassed source delivery contract");}
void native_data(){lvrs::SvgManager p;const auto url="data:image/svg+xml;base64,"+network::wire::base64(svg);check(p.image(url)&&p.image("data:image/svg+xml,%3Csvg%20xmlns='http://www.w3.org/2000/svg'%20width='1'%20height='1'/%3E"),"Native data source decoding failed");}
void native_file(){const auto path=std::filesystem::path(SVG_TEST_DIRECTORY)/"source with space.svg";std::filesystem::create_directories(path.parent_path());iiFileProvider::native::File::write(path,svg);lvrs::SvgManager p;check(p.image(path.string())!=nullptr,"Native FileProvider source failed");auto u8=std::filesystem::absolute(path).generic_u8string();std::string encoded(reinterpret_cast<const char*>(u8.data()),u8.size());for(std::size_t i=0;(i=encoded.find(' ',i))!=encoded.npos;i+=3)encoded.replace(i,1,"%20");if(encoded.starts_with('/'))encoded.erase(0,1);check(p.image("file:///"+encoded)!=nullptr,"File URL path decoding failed");}
void native_embedded(){const auto all=lvrs::Icons::sources();check(!all.empty(),"No embedded SVG fixtures");lvrs::SvgManager p;check(p.image("qrc:/qt/qml/LVRS/resources/iconset/"+std::string(all.front().name)+".svg")!=nullptr,"Native embedded SVG lookup failed");}
void native_gzip(){z_stream stream{};check(deflateInit2(&stream,Z_DEFAULT_COMPRESSION,Z_DEFLATED,15+16,8,Z_DEFAULT_STRATEGY)==Z_OK,"Gzip fixture init failed");std::string bytes(1024,'\0');stream.next_in=reinterpret_cast<Bytef*>(const_cast<char*>(svg));stream.avail_in=static_cast<uInt>(std::string_view(svg).size());stream.next_out=reinterpret_cast<Bytef*>(bytes.data());stream.avail_out=static_cast<uInt>(bytes.size());check(deflate(&stream,Z_FINISH)==Z_STREAM_END,"Gzip fixture encoding failed");bytes.resize(stream.total_out);deflateEnd(&stream);lvrs::SvgManager p;check(p.image("data:image/svg+xml;base64,"+network::wire::base64(bytes))!=nullptr,"Native SVGZ decoding failed");}
void native_network_unavailable(){lvrs::SvgManager p;check(!p.image("https://example.test/a")&&p.lastError().find("EventLoop")!=std::string::npos,"Missing application network loop was hidden");}
void http(std::string_view mode,std::string url){
    runtime::EventLoop loop;lvrs::SvgManager p(loop);check(!p.image(url),"HTTP request completed inline");
    const auto until=runtime::Clock::now()+std::chrono::seconds(8);
    while(runtime::Clock::now()<until&&!p.revision()&&(p.lastError()=="SVG request pending"||p.lastError().empty())){loop.poll();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    if(mode=="http_success"||mode=="http_redirect")check(p.revision()==1&&p.image(url)!=nullptr,"Native HTTP SVG or relative redirect failed");
    else check(p.revision()==0&&p.lastError()!="SVG request pending"&&!p.lastError().empty(),"Native HTTP error was not published");
}
}
int main(int argc,char** argv){try{
    const std::string name=argc>1?argv[1]:"defaults";
    const std::map<std::string,void(*)()> cases{{"defaults",defaults},{"vector",vector},{"malformed",malformed},{"raster",raster},{"scales",scales},{"explicit_scale",explicit_scale},{"identity",identity},{"quantized",quantized},{"lru",lru},{"disabled_cache",disabled_cache},{"clear",clear},{"bounds",bounds},{"memory_budget",memory_budget},{"pending",pending},{"completion",completion},{"failure",failure},{"exceptions",exceptions},{"clear_pending",clear_pending},{"request_limit",request_limit},{"source_limit",source_limit},{"events",events},{"observer_failure",observer_failure},{"owner",owner},{"foreign_completion",foreign_completion},{"retirement",retirement},{"retirement_in_notify",retirement_in_notify},{"inline_completion",inline_completion},{"native_data",native_data},{"native_file",native_file},{"native_embedded",native_embedded},{"native_gzip",native_gzip},{"native_network_unavailable",native_network_unavailable}};
    if(name.starts_with("http_")){check(argc==3,"HTTP test requires a local fixture URL");http(name,argv[2]);}else cases.at(name)();
    std::cout<<"Native SVG "<<name<<" passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
