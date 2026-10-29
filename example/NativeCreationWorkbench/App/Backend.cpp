#include "Backend.h"
#include <iiGUI/Image.h>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <sstream>
using namespace ii;
namespace {
template<class T>T take(core::Result<T> v){if(!v)throw std::runtime_error(v.error().message);return std::move(*v);}
void good(core::Result<void> v){if(!v)throw std::runtime_error(v.error().message);}
void panel(gui::WidgetDrawing& d,gui::Rect r,gui::Color color,float stroke=0){d.panels.push_back({.bounds=r,.top=color,.bottom=color,.stroke_width=stroke});}
struct ViewState final {bool dragging=false;double x=0,y=0;std::optional<runtime::Clock::time_point> previous;core::ObservableSubscription subscription;};
class Surface final:public gui::NativeViewSource {
    std::shared_ptr<CreationModel> model_;
    bool timeline_;
public:
    Surface(std::shared_ptr<CreationModel> m,bool timeline):model_(std::move(m)),timeline_(timeline){}
    core::Result<void> build(gui::WidgetBuilder& b)const override{
        auto m=model_;const bool timeline=timeline_;
        auto root=b.absolute_stack({},"surface",{});if(!root)return std::unexpected(root.error());
        auto local=std::make_shared<ViewState>();if(auto r=b.retain(local);!r)return r;
        ++m->view_builds;if(timeline)m->timeline_widget=*root;else m->canvas_widget=*root;
        auto drawing=root->set_paint_handler([m,timeline](const gui::WidgetPaintState& p)->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{
            auto d=std::make_shared<gui::WidgetDrawing>();float width=float(p.size.width),height=float(p.size.height);
            if(width<=0||height<=0)return std::shared_ptr<const gui::WidgetDrawing>(std::move(d));
            if(timeline){
                panel(*d,{0,0,width,1},{.3f,.3f,.3f,1});
                for(unsigned i=0;i<24;++i){float x=12+(std::max(24.f,width)-24)*float(i)/23.f;panel(*d,{x,22,1,i%6?7.f:13.f},{.6f,.6f,.6f,1});if(i%6==0)d->texts.push_back({.bounds={x,2,40,18},.text=std::to_string(i),.color={.8f,.8f,.8f,1},.font_size=12});}
                float head=12+(std::max(24.f,width)-24)*float(m->frame)/23.f;panel(*d,{head,18,2,std::max(1.f,height-18)},{0,.4f,1,1});
                panel(*d,{12,44,std::max(1.f,width-24),24},{.16f,.22f,.34f,1});
                d->texts.push_back({.bounds={20,45,300,22},.text="Layer 1 · native frame sequence",.color={.9f,.9f,.9f,1},.font_size=13});
            }else if(!m->frames.empty()){
                auto image=m->frames[m->frame%m->frames.size()];float w=std::max(1.f,width-64),h=std::max(1.f,height-48);float ratio=std::min(w/float(image->width()),h/float(image->height()));w=float(image->width())*ratio;h=float(image->height())*ratio;
                gui::Rect r{(width-w)/2+float(m->pan_x),(height-h)/2+float(m->pan_y),w,h};d->images.push_back({.bounds=r,.image=image});d->images_before_text=true;
                panel(*d,r,{0,.4f,1,1},1);for(float x:{r.x,r.x+r.width/2,r.x+r.width})for(float y:{r.y,r.y+r.height/2,r.y+r.height})if(x!=r.x+r.width/2||y!=r.y+r.height/2)panel(*d,{x-3,y-3,6,6},{.8f,.9f,1,1});
            }else d->texts.push_back({.bounds={24,24,std::max(1.f,width-48),40},.text="Render a procedural frame sequence",.color={.8f,.8f,.8f,1},.font_size=16});
            return std::shared_ptr<const gui::WidgetDrawing>(std::move(d));
        });if(!drawing)return drawing;
        auto input=root->set_input_handler([m,timeline,local,widget=*root](const gui::WidgetInput& e)->core::Result<bool>{
            if(e.kind==gui::WidgetInputKind::pointer_down){local->dragging=true;local->x=e.x;local->y=e.y;}
            else if(e.kind==gui::WidgetInputKind::pointer_up)local->dragging=false;
            else if(e.kind==gui::WidgetInputKind::focus_lost){local->dragging=false;return false;}
            else if(e.kind==gui::WidgetInputKind::scroll){m->seek(e.scroll_y>0?m->frame+1:m->frame?m->frame-1:0);return true;}
            else if(e.kind!=gui::WidgetInputKind::pointer_move)return false;
            if(!local->dragging&&e.kind!=gui::WidgetInputKind::pointer_up)return false;
            if(timeline){auto bounds=widget.snapshot();if(!bounds)return std::unexpected(bounds.error());double t=(e.x-bounds->bounds.x-12)/std::max(1.,double(bounds->bounds.width)-24);m->seek(unsigned(std::clamp(t,0.,1.)*23));}
            else{m->pan_x+=e.x-local->x;m->pan_y+=e.y-local->y;local->x=e.x;local->y=e.y;m->touch();}
            return true;
        });if(!input)return input;
        if(!timeline){auto frame=root->set_frame_handler([m,local](runtime::Clock::time_point now)->core::Result<bool>{if(!m->playing){local->previous.reset();return false;}if(!local->previous||now-*local->previous>=std::chrono::milliseconds(42)){local->previous=now;if(!m->frames.empty()){++m->play_ticks;m->seek((m->frame+1)%unsigned(m->frames.size()));}}return true;});if(!frame)return frame;}
        local->subscription=m->visual.observe([widget=*root,m,timeline](const auto&,std::uint64_t){good(widget.request_repaint());if(!timeline&&m->playing)good(widget.request_frames());});
        return b.on_retire([local,m]{local->subscription.reset();++m->view_retires;});
    }
};
bool number(std::string_view text,unsigned low,unsigned high,unsigned& result){auto r=std::from_chars(text.data(),text.data()+text.size(),result);return r.ec==std::errc{}&&r.ptr==text.data()+text.size()&&result>=low&&result<=high;}
}
void CreationModel::touch(){visual.set(visual.get()+1);}
void CreationModel::seek(unsigned value){frame=std::min(value,23u);std::ostringstream out;out<<'F'<<std::setw(3)<<std::setfill('0')<<frame;frame_label.set(out.str());touch();}
void CreationModel::accept(CreationOutput output,unsigned count){
    frames=std::move(output.frames);std::vector<std::shared_ptr<CreationResult>> items;
    for(unsigned i=0;i<count&&i<frames.size();++i){auto item=std::make_shared<CreationResult>();item->id=std::to_string(i);item->label="Candidate "+std::to_string(i+1);item->image=frames[i];item->choose=[weak=weak_from_this(),i]{if(auto m=weak.lock()){m->selected.set("Candidate "+std::to_string(i+1));m->seek(i);}};items.push_back(std::move(item));}
    results.set(std::move(items));progress.set(100);status.set("Completed · 24 native frames");seek(0);
}
CreationOutput render_creation_probe(CreationSettings s){
    CreationOutput result;for(unsigned f=0;f<24;++f){std::vector<std::uint8_t> bytes(std::size_t(s.width)*s.height*4);for(unsigned y=0;y<s.height;++y)for(unsigned x=0;x<s.width;++x){auto p=(std::size_t(y)*s.width+x)*4;bytes[p]=std::uint8_t((x+f*11+s.seed)%256);bytes[p+1]=std::uint8_t((y*2+f*7)%256);bytes[p+2]=std::uint8_t((x+y+s.seed*3)%256);bytes[p+3]=255;}result.frames.push_back(take(gui::rgba_image(s.width,s.height,std::move(bytes))));}return result;
}
CreationBackend::CreationBackend(){++created;canvas=std::make_shared<Surface>(model,false);timeline=std::make_shared<Surface>(model,true);}
void CreationBackend::render(){CreationSettings s;if(!number(width.get(),64,512,s.width)||!number(height.get(),64,512,s.height)||!number(count.get(),1,8,s.count)||!number(seed.get(),0,65535,s.seed)){status.set("Width/height 64–512, output count 1–8, seed 0–65535");return;}if(on_render){status.set("Rendering native procedural frames");progress.set(0);on_render(s);}}
