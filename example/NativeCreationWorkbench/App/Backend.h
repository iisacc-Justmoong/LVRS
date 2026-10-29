#pragma once
#include <iiGUI/NativeView.h>
#include <iiSDKCore/Core/Property/Observable.h>
struct CreationModel;
struct CreationResult final {
    std::string id,label;
    std::shared_ptr<const ii::gui::RasterImage> image;
    std::function<void()> choose;
    void select(){if(choose)choose();}
};
struct CreationSettings final {unsigned width=192,height=128,count=4,seed=7;};
struct CreationOutput final {std::vector<std::shared_ptr<const ii::gui::RasterImage>> frames;};
struct CreationModel final:std::enable_shared_from_this<CreationModel> {
    ii::core::Observable<std::string> prompt{"Native procedural frame probe"},width{"192"},height{"128"},count{"4"},seed{"7"},status{"Ready"},frame_label{"F000"},selected{"None"};
    ii::core::Observable<double> view_width{800},progress{0};
    ii::core::Observable<std::uint64_t> visual{0};
    ii::core::Observable<std::vector<std::shared_ptr<CreationResult>>> results;
    std::vector<std::shared_ptr<const ii::gui::RasterImage>> frames;
    ii::gui::Widget canvas_widget,timeline_widget;
    unsigned frame=0,view_builds=0,view_retires=0,file_revision=0,play_ticks=0;
    bool playing=false;
    double pan_x=0,pan_y=0;
    void touch();
    void seek(unsigned);
    void accept(CreationOutput,unsigned count);
};
CreationOutput render_creation_probe(CreationSettings);
class CreationBackend final {
public:
    inline static unsigned created=0,retired=0;
    std::shared_ptr<CreationModel> model=std::make_shared<CreationModel>();
    std::shared_ptr<ii::gui::NativeViewSource> canvas,timeline;
    ii::core::Observable<std::string>& prompt=model->prompt;
    ii::core::Observable<std::string>& width=model->width;
    ii::core::Observable<std::string>& height=model->height;
    ii::core::Observable<std::string>& count=model->count;
    ii::core::Observable<std::string>& seed=model->seed;
    ii::core::Observable<std::string>& status=model->status;
    ii::core::Observable<std::string>& frame_label=model->frame_label;
    ii::core::Observable<std::string>& selected=model->selected;
    ii::core::Observable<double>& view_width=model->view_width;
    ii::core::Observable<double>& progress=model->progress;
    ii::core::Observable<std::vector<std::shared_ptr<CreationResult>>>& results=model->results;
    std::function<void(CreationSettings)> on_render;
    std::function<void()> on_close;
    CreationBackend();
    ~CreationBackend(){++retired;}
    void edit_prompt(std::string v){prompt.set(std::move(v));}
    void edit_width(std::string v){width.set(std::move(v));}
    void edit_height(std::string v){height.set(std::move(v));}
    void edit_count(std::string v){count.set(std::move(v));}
    void edit_seed(std::string v){seed.set(std::move(v));}
    void render();
    void play(){model->playing=!model->playing;model->touch();}
    void previous(){model->seek(model->frame?model->frame-1:0);}
    void next(){model->seek(model->frame+1);}
    void reset_pan(){model->pan_x=model->pan_y=0;model->touch();}
    void close(){if(on_close)on_close();}
};
