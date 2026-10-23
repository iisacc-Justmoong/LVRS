#include <LVRS/Icons.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace lvrs;
namespace { void check(bool v,const char* why) { if(!v) throw std::runtime_error(why); } }
int main() {
    try {
        check(Icons::sources().size()>=2860,"authored icon inventory lost");
        check(Icons::normalize(" /general/add.SVG ")=="generaladd","Theme path/name normalization");
        check(Icons::normalize(" ADD ")=="generaladd" && Icons::normalize("wechat")=="weChat","Theme name aliases");
        auto canonical=Icons::data("generaladd"),alias=Icons::data("add");
        check(canonical && alias && *canonical==*alias,"alias resolves to exact authored SVG");
        auto qrc=Icons::data("qrc:/qt/qml/LVRS/resources/iconset/generaladd.svg");check(qrc && *qrc==*canonical,"existing resource names resolve from embedded data");
        auto legacy=Icons::data("GroupByModule-1");auto base=Icons::data("GroupByModule");
        check(legacy && base && *legacy==*base,"Figma QRC alias retained");
        check(!Icons::data("does-not-exist") && !Icons::image("add",0) && !Icons::image("add",513),"unknown or unbounded icon request rejected");
        auto first=Icons::image("add"),second=Icons::image("generaladd");
        check(first && second && *first==*second,"canonical aliases share immutable raster cache");
        std::vector<std::shared_ptr<const ii::gui::RasterImage>> images(8);std::vector<std::thread> workers;
        for(std::size_t i=0;i<images.size();++i) workers.emplace_back([&,i]{auto r=Icons::image("nodesfolder",48);if(r) images[i]=*r;});
        for(auto& worker:workers) worker.join();
        check(images[0]!=nullptr && std::ranges::all_of(images,[&](const auto& image){return image==images[0];}),"concurrent decode converges on one immutable image");
        std::size_t rendered=0,unsupported=0;
        for(const auto& source:Icons::sources()) {
            auto image=ii::gui::svg_image(source.svg,24,24);
            if(image) { check((*image)->rgba().size()==24*24*4,"icon output pixel dimensions");++rendered; }
            else { check(image.error().code==ii::core::ErrorCode::unsupported,"authored SVG cannot decode");++unsupported; }
        }
        check(rendered>=2859 && unsupported>=1,"full icon inventory render audit");
        std::cout<<"NATIVE_ICONS_OK lookup_names="<<Icons::sources().size()<<" rendered="<<rendered<<" explicit_unsupported="<<unsupported<<'\n';
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
