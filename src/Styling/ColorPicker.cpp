#include <LVRS/ColorPicker.h>
#include <LVRS/Widgets.h>
#include <iiSDKCore/Core/Text/Number.h>
#include <array>
#include <charconv>
#include <cmath>
#include <stdexcept>
namespace lvrs {
namespace detail {std::span<const std::uint8_t> color_ring_png();}
ii::core::Result<std::shared_ptr<const ii::gui::RasterImage>> color_picker_hue_ring() {
    static const auto image=ii::gui::png_image(detail::color_ring_png());return image;
}
namespace {
using namespace ii;
struct Failure final {core::Error error;};
template<class T>T take(core::Result<T> r){if(!r)throw Failure{r.error()};return std::move(*r);}
void good(core::Result<void> r){if(!r)throw Failure{r.error()};}
template<class F>core::Result<void> attempt(F&& f){try{f();return {};}catch(const Failure& e){return std::unexpected(e.error);}}
void dispatch(core::Result<void> r){if(!r && r.error().code!=core::ErrorCode::closed)throw std::runtime_error(r.error().message);}
bool finite(gui::WidgetColor c){return std::isfinite(c.red)&&std::isfinite(c.green)&&std::isfinite(c.blue)&&std::isfinite(c.alpha);}
bool dimensions(double width,double padding){return std::isfinite(width)&&width>=320&&width<=1024&&std::isfinite(padding)&&padding>=0&&padding<=64&&width-2*padding>=228;}
gui::WidgetColor clamp(gui::WidgetColor c){return {std::clamp(c.red,0.,1.),std::clamp(c.green,0.,1.),std::clamp(c.blue,0.,1.),std::clamp(c.alpha,0.,1.)};}
std::string integer(double x){return std::to_string(std::lround(x));}
std::optional<double> decimal(std::string_view s){
    const auto first=s.find_first_not_of(" \t\r\n");if(first==s.npos)return {};s=s.substr(first,s.find_last_not_of(" \t\r\n")-first+1);if(s.starts_with('+'))s.remove_prefix(1);
    return ii::core::parse_decimal(s);
}
std::shared_ptr<const gui::RasterImage> swatch(double logical_width,double logical_height,gui::WidgetColor current,gui::WidgetColor previous,bool split,bool selected=false){
    const auto width=std::uint32_t(std::max(1.,std::ceil(logical_width*4))),height=std::uint32_t(std::max(1.,std::ceil(logical_height*4)));
    std::vector<std::uint8_t> pixels(std::size_t(width)*height*4);
    for(std::uint32_t y=0;y<height;++y)for(std::uint32_t x=0;x<width;++x){
        const double px=(x+.5)/4,py=(y+.5)/4,dx=std::max({4-px,px-(logical_width-4),0.}),dy=std::max({4-py,py-(logical_height-4),0.});
        const double coverage=std::clamp((4-std::hypot(dx,dy))*4+.5,0.,1.);auto c=split&&px<logical_width/2?previous:current;
        const double background=((int(px/4)+int(py/4))%2?112.:176.)/255;
        double rgb[]{c.red*c.alpha+background*(1-c.alpha),c.green*c.alpha+background*(1-c.alpha),c.blue*c.alpha+background*(1-c.alpha)};
        if(!split){const double edge=std::min({px,py,logical_width-px,logical_height-py,4-std::hypot(dx,dy)}),alpha=selected?.8:.12;if(edge<(selected?1.5:.5))for(auto& channel:rgb)channel=channel*(1-alpha)+alpha;}
        const auto at=(std::size_t(y)*width+x)*4;for(int i=0;i<3;++i)pixels[at+i]=std::uint8_t(std::lround(std::clamp(rgb[i],0.,1.)*255));pixels[at+3]=std::uint8_t(std::lround(coverage*255));
    }
    return take(gui::rgba_image(width,height,std::move(pixels)));
}
std::shared_ptr<const gui::RasterImage> ramp(const std::array<gui::WidgetColor,7>& stops){
    constexpr unsigned width=512,height=16;std::vector<std::uint8_t> pixels(width*height*4);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){double t=double(x)/(width-1)*6;auto i=std::min(unsigned(t),5u);auto a=stops[i],b=stops[i+1];t-=i;const double c[]{a.red+(b.red-a.red)*t,a.green+(b.green-a.green)*t,a.blue+(b.blue-a.blue)*t,a.alpha+(b.alpha-a.alpha)*t};const auto at=(y*width+x)*4;
        const double dx=std::max({8.-(x+.5),x+.5-(width-8.),0.}),dy=std::abs(y+.5-8.);const double coverage=std::clamp(8.-std::hypot(dx,dy)+.5,0.,1.);
        for(unsigned k=0;k<4;++k)pixels[at+k]=std::uint8_t(std::lround(std::clamp(c[k]*(k==3?coverage:1),0.,1.)*255));}
    return take(gui::rgba_image(width,height,std::move(pixels)));
}
constexpr const char* labels[]{"H","S","B","R","G","B","C","M","Y","K","A","K"};
constexpr const char* names[]{"Hue","Saturation","Brightness","Red","Green","Blue","Cyan","Magenta","Yellow","Black","Alpha","Grayscale"};
}
struct ColorPicker::Impl final {
    struct Row final {ii::gui::Widget group,label,slider,input,suffix;bool dirty=false,has_ramp=false;std::array<ii::gui::WidgetColor,7> stops{};};
    ii::gui::Widget root,field,rail,divider,comparison,hex_label,hex,alpha_label,alpha,percent,recent_label,eyedropper,space_label,apply_button,cancel_button;
    std::array<Row,12> rows;
    std::array<ii::gui::Widget,10> recent;
    std::array<std::optional<bool>,10> recent_selected;
    std::vector<ii::gui::Widget> enabled_controls;
    std::shared_ptr<ColorPickerModel> model;
    std::shared_ptr<const ii::gui::ColorChangePort> receiver;
    ColorPickerOptions options;
    ColorPickerEvents events;
    bool hex_dirty=false,alpha_dirty=false;
    float apply_width=52,cancel_width=58;
    double recent_width=0;
    std::optional<ii::gui::WidgetColor> swatch_current,swatch_previous;
    std::array<bool,12> active_rows() const {
        std::array<bool,12> result{};switch(options.type){
            case ColorPickerType::wheel:result[0]=result[1]=result[2]=true;break;
            case ColorPickerType::hue_saturation:result[2]=true;break;
            case ColorPickerType::saturation_brightness:result[0]=true;break;
            case ColorPickerType::grayscale:result[11]=true;break;
            case ColorPickerType::rgb:result[3]=result[4]=result[5]=true;break;
            case ColorPickerType::cmyk:result[6]=result[7]=result[8]=result[9]=true;break;
        }return result;
    }
    bool plane()const{return options.type==ColorPickerType::hue_saturation||options.type==ColorPickerType::saturation_brightness;}
    std::string display(unsigned channel)const{return integer(take(model->channel_value(ColorChannel(channel))));}
    void restore_input(unsigned channel){good(rows[channel].input.set_text(display(channel)));}
    void commit(unsigned channel){
        auto& row=rows[channel];if(!row.dirty)return;row.dirty=false;auto value=decimal(take(row.input.snapshot()).text);
        if(value)good(model->edit_channel(ColorChannel(channel),*value));if(!root.snapshot())return;restore_input(channel);
    }
    void commit_hex(){if(!hex_dirty)return;hex_dirty=false;auto text=take(hex.snapshot()).text;(void)take(model->edit_hex(text));if(root.snapshot())good(hex.set_text(take(model->state()).hex));}
    void commit_alpha(){if(!alpha_dirty)return;alpha_dirty=false;auto value=decimal(take(alpha.snapshot()).text);if(value)good(model->edit_channel(ColorChannel::alpha,*value));if(root.snapshot())good(alpha.set_text(display(10)));}
    void commit_all(){for(unsigned i=0;i<12;++i){commit(i);if(!root.snapshot())return;}commit_hex();if(root.snapshot())commit_alpha();}
    void accept(){good(root.focus());if(!root.snapshot())return;commit_all();if(!root.snapshot())return;good(model->accept());if(!root.snapshot())return;auto callback=events.accepted;auto color=take(model->state()).color;if(callback)callback(color);}
    void cancel(){good(root.focus());if(!root.snapshot())return;commit_all();if(!root.snapshot())return;good(model->cancel());if(!root.snapshot())return;auto callback=events.canceled;if(callback)callback();}
    void layout(){
        const float w=float(options.width-2*options.padding),padding=float(options.padding);float y=padding;auto active=active_rows();
        for(unsigned i=0;i<12;++i)good(rows[i].group.set_visible(active[i]));
        auto row_at=[&](unsigned i){auto& row=rows[i];good(row.group.set_frame({padding,y,w,22}));good(row.label.set_frame({0,3,12,19}));good(row.slider.set_frame({20,0,w-100,22}));good(row.input.set_frame({w-72,0,52,22}));good(row.suffix.set_frame({w-12,3,12,19}));y+=26;};
        if(!plane()){bool any=false;for(unsigned i=0;i<12;++i)if(active[i]){row_at(i);any=true;}if(any)y+=8;}
        const float domain_height=plane()?w-36:options.type==ColorPickerType::wheel?std::min(w,232.f):184;
        auto kind=options.type==ColorPickerType::wheel?ColorField::wheel:options.type==ColorPickerType::hue_saturation?ColorField::hue_saturation:options.type==ColorPickerType::saturation_brightness?ColorField::saturation_brightness:ColorField::spectrum;
        good(field.set_color_field(kind));good(field.set_frame({padding,y,plane()?w-36:w,domain_height}));
        good(rail.set_visible(plane()));good(rail.set_color_field(options.type==ColorPickerType::hue_saturation?ColorField::brightness_rail:ColorField::hue_rail));good(rail.set_frame({padding+w-24,y,24,domain_height}));y+=domain_height+12;
        if(plane()){row_at(options.type==ColorPickerType::hue_saturation?2:0);y+=8;}
        good(divider.set_frame({padding,y,w,1}));y+=13;
        good(comparison.set_frame({padding,y,40,28}));good(hex_label.set_frame({padding+48,y+5,25,18}));good(hex.set_frame({padding+81,y+3,std::max(40.f,w-187),22}));good(alpha_label.set_frame({padding+w-98,y+5,10,18}));good(alpha.set_frame({padding+w-80,y+3,56,22}));good(percent.set_frame({padding+w-16,y+5,16,18}));y+=28;
        good(recent_label.set_visible(options.show_recent_colors));const auto count=options.recent_colors->size();
        if(options.show_recent_colors){y+=12;good(recent_label.set_frame({padding,y+3,40,17}));}
        const auto width=count?std::max(1.f,(w-48-float(count-1)*8)/float(count)):1.f;
        for(unsigned i=0;i<10;++i){good(recent[i].set_visible(options.show_recent_colors&&i<count));if(i<count)good(recent[i].set_frame({padding+48+i*(width+8),y,width,20}));if(recent_width!=width)recent_selected[i].reset();}recent_width=width;
        if(options.show_recent_colors)y+=20;
        for(auto control:{eyedropper,space_label,apply_button,cancel_button})good(control.set_visible(options.show_actions));
        if(options.show_actions){y+=12;good(eyedropper.set_frame({padding,y,22,22}));const float aw=apply_width,cw=cancel_width;
            good(apply_button.set_frame({padding+w-aw,y,aw,22}));good(cancel_button.set_frame({padding+w-aw-8-cw,y,cw,22}));good(space_label.set_frame({padding+30,y+3,std::max(0.f,w-aw-cw-76),18}));y+=22;}
        good(root.set_extent({options.width,double(y)+options.padding}));
    }
    void refresh(){
        auto state=take(model->state());auto active=active_rows();
        for(unsigned i=0;i<12;++i)if(active[i]){auto& row=rows[i];good(row.slider.set_value(take(model->channel_value(ColorChannel(i)))));if(!take(row.input.snapshot()).focused)restore_input(i);
            std::array<ii::gui::WidgetColor,7> stops;for(unsigned k=0;k<7;++k)stops[k]=take(model->channel_color(ColorChannel(i),double(k)/6));if(!row.has_ramp||row.stops!=stops){row.stops=stops;row.has_ramp=true;good(row.slider.set_image(ramp(stops)));}}
        if(!take(hex.snapshot()).focused)good(hex.set_text(state.hex));if(!take(alpha.snapshot()).focused)good(alpha.set_text(display(10)));
        if(swatch_current!=state.color||swatch_previous!=state.previous_color){good(comparison.set_image(swatch(40,28,state.color,state.previous_color,true)));swatch_current=state.color;swatch_previous=state.previous_color;}
        const auto count=options.recent_colors->size();const auto width=count?std::max(1.,(options.width-2*options.padding-48-double(count-1)*8)/double(count)):1.;
        for(unsigned i=0;i<count;++i){const auto color=(*options.recent_colors)[i];const bool selected=color==state.color;if(!recent_selected[i]||*recent_selected[i]!=selected){good(recent[i].set_image(swatch(width,20,color,color,false,selected)));recent_selected[i]=selected;}}
    }
};
ii::core::Result<ColorPicker> ColorPicker::create(ii::gui::WidgetBuilder& builder,ii::gui::Widget parent,std::string_view id,ColorPickerOptions options,ColorPickerEvents events){
    using namespace ii;
    if(unsigned(options.type)>unsigned(ColorPickerType::cmyk)||!dimensions(options.width,options.padding)||id.empty())return core::fail(core::ErrorCode::invalid_argument,"ColorPicker requires a valid type, width 320..1024 and padding leaving 228 content units");
    if(!options.recent_colors){options.recent_colors.emplace();for(auto accent:{Accent::blue,Accent::red,Accent::green,Accent::yellow,Accent::purple,Accent::rose,Accent::orange,Accent::brown,Accent::cyan,Accent::teal})options.recent_colors->push_back(Theme::accent_color(accent));}
    if(options.recent_colors->size()>10)options.recent_colors->resize(10);for(auto& color:*options.recent_colors){if(!finite(color))return core::fail(core::ErrorCode::invalid_argument,"Recent colors must be finite");color=clamp(color);}
    try{
        auto state=std::make_shared<Impl>();state->options=std::move(options);state->events=std::move(events);state->model=state->options.model;
        if(!state->model){state->model=std::make_shared<ColorPickerModel>();good(state->model->set_color(Theme::accent_color(Accent::purple)));}
        (void)take(state->model->state());state->root=take(builder.absolute_stack(parent,id,{state->options.width,461}));std::weak_ptr<Impl> weak=state;
        auto name=[&](std::string suffix){return std::string(id)+"."+suffix;};auto placeholder=take(gui::rgba_image(1,1,{255,255,255,255}));
        for(unsigned i=0;i<12;++i){auto& row=state->rows[i];auto row_id=name("channel."+std::to_string(i));row.group=take(builder.absolute_stack(state->root,row_id,{296,22}));row.label=take(builder.label(row.group,row_id+".label",{.text=labels[i],.font_size=13,.selectable=false,.role=gui::TextRole::body}));
            row.slider=take(builder.slider(row.group,row_id+".slider",{.minimum=0,.maximum=i==0?360.:i>=3&&i<=5?255.:100.,.value=0,.step=1,.width=196,.thumb_diameter=12,.track_height=4,.range_inset=8,.accessibility_name=names[i]},[weak,i](double value){if(auto s=weak.lock())dispatch(s->model->edit_channel(ColorChannel(i),value));}));
            row.input=take(builder.input_field(row.group,row_id+".input",{.width=52,.commit_on_focus_loss=true,.accessibility_name=names[i]},[weak,i](std::string){if(auto s=weak.lock())s->rows[i].dirty=true;},[weak,i]{if(auto s=weak.lock())dispatch(attempt([&]{s->commit(i);}));}));
            row.suffix=take(builder.label(row.group,row_id+".suffix",{.text=i==0?"°":i>=3&&i<=5?"":"%",.font_size=11,.selectable=false,.role=gui::TextRole::caption}));state->enabled_controls.push_back(row.slider);state->enabled_controls.push_back(row.input);
        }
        Widgets controls(builder);
        state->field=take(controls.color_surface(state->root,name("field"),{.model=state->model,.width=296,.height=232,.accessibility_name="Color field"}));state->rail=take(controls.color_surface(state->root,name("rail"),{.model=state->model,.field=ColorField::hue_rail,.width=24,.height=232,.accessibility_name="Color rail"}));
        state->divider=take(builder.image(state->root,name("divider"),{.image=take(gui::rgba_image(1,1,{255,255,255,28})),.width=296,.height=1}));
        state->comparison=take(builder.image(state->root,name("comparison"),{.image=placeholder,.width=40,.height=28,.accessibility_name="Previous and current color"}));
        auto caption=[&](const char* suffix,const char* text){return take(builder.label(state->root,name(suffix),{.text=text,.font_size=11,.selectable=false,.role=gui::TextRole::caption}));};
        state->hex_label=caption("hexLabel","Hex");state->alpha_label=caption("alphaLabel","A");state->percent=caption("percent","%");state->recent_label=caption("recentLabel","Recent");state->space_label=caption("colorSpace","sRGB");
        state->hex=take(builder.input_field(state->root,name("hex"),{.width=109,.commit_on_focus_loss=true,.accessibility_name="Hex color, RRGGBB or RRGGBBAA",.maximum_length=9},[weak](std::string){if(auto s=weak.lock())s->hex_dirty=true;},[weak]{if(auto s=weak.lock())dispatch(attempt([&]{s->commit_hex();}));}));
        state->alpha=take(builder.input_field(state->root,name("alpha"),{.width=56,.commit_on_focus_loss=true,.accessibility_name="Opacity percent"},[weak](std::string){if(auto s=weak.lock())s->alpha_dirty=true;},[weak]{if(auto s=weak.lock())dispatch(attempt([&]{s->commit_alpha();}));}));
        for(unsigned i=0;i<10;++i){state->recent[i]=take(builder.button(state->root,name("recent."+std::to_string(i)),{.tone=gui::ButtonTone::borderless,.accessibility_name="Recent color "+std::to_string(i+1),.background_image=placeholder},[weak,i]{if(auto s=weak.lock())dispatch(attempt([&]{good(s->recent[i].focus());if(s->root.snapshot()&&i<s->options.recent_colors->size())good(s->model->edit_color((*s->options.recent_colors)[i]));}));}));state->enabled_controls.push_back(state->recent[i]);}
        state->eyedropper=take(controls.icon_button(state->root,name("eyedropper"),{.tone=gui::ButtonTone::normal,.icon_name="imagecolorPicker",.accessibility_name="Pick a color from the document"},[weak]{if(auto s=weak.lock())dispatch(attempt([&]{good(s->eyedropper.focus());if(!s->root.snapshot())return;auto callback=s->events.eyedropper_requested;if(callback)callback();}));}));
        state->apply_button=take(builder.button(state->root,name("apply"),{.text=state->options.apply_text,.tone=gui::ButtonTone::primary},[weak]{if(auto s=weak.lock())dispatch(attempt([&]{s->accept();}));}));
        state->cancel_button=take(builder.button(state->root,name("cancel"),{.text=state->options.cancel_text,.tone=gui::ButtonTone::normal},[weak]{if(auto s=weak.lock())dispatch(attempt([&]{s->cancel();}));}));
        state->apply_width=std::max(52.f,float(take(state->apply_button.snapshot()).implicit_size.width));state->cancel_width=std::max(58.f,float(take(state->cancel_button.snapshot()).implicit_size.width));
        good(state->root.set_escape_action([weak]{if(auto s=weak.lock())dispatch(attempt([&]{s->cancel();}));}));
        for(auto control:{state->field,state->rail,state->hex,state->alpha,state->eyedropper,state->apply_button,state->cancel_button})state->enabled_controls.push_back(control);
        state->layout();state->refresh();for(auto control:state->enabled_controls)good(control.set_enabled(state->options.enabled));
        state->receiver=std::make_shared<gui::ColorChangePort>([weak](const gui::ColorPickerChange& change){if(auto s=weak.lock()){if(!s->root.snapshot())return;dispatch(attempt([&]{s->refresh();}));if(change.edited){auto callback=s->events.color_edited;auto color=take(s->model->state()).color;if(callback)callback(color);}}});good(state->model->observe(state->receiver));good(builder.retain(state));
        ColorPicker result;result.root_=state->root;result.state_=state;return result;
    }catch(const Failure& e){return std::unexpected(e.error);}
}
ii::core::Result<std::shared_ptr<ColorPicker::Impl>> ColorPicker::lock()const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto state=state_.lock();if(!state)return ii::core::fail(ii::core::ErrorCode::closed,"Color picker has retired");return state;}
ii::core::Result<std::shared_ptr<ColorPickerModel>> ColorPicker::model()const{auto s=lock();if(!s)return std::unexpected(s.error());return (*s)->model;}
ii::core::Result<void> ColorPicker::set_type(ColorPickerType type)const{auto s=lock();if(!s)return std::unexpected(s.error());if(unsigned(type)>unsigned(ColorPickerType::cmyk))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid color picker type");return attempt([&]{(*s)->options.type=type;(*s)->layout();(*s)->refresh();});}
ii::core::Result<void> ColorPicker::set_width(double width)const{auto s=lock();if(!s)return std::unexpected(s.error());if(!dimensions(width,(*s)->options.padding))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid color picker width");return attempt([&]{(*s)->options.width=width;(*s)->layout();(*s)->refresh();});}
ii::core::Result<void> ColorPicker::set_enabled(bool enabled)const{auto s=lock();if(!s)return std::unexpected(s.error());return attempt([&]{(*s)->options.enabled=enabled;for(auto control:(*s)->enabled_controls)good(control.set_enabled(enabled));});}
ii::core::Result<void> ColorPicker::show_recent_colors(bool visible)const{auto s=lock();if(!s)return std::unexpected(s.error());return attempt([&]{(*s)->options.show_recent_colors=visible;(*s)->layout();(*s)->refresh();});}
ii::core::Result<void> ColorPicker::show_actions(bool visible)const{auto s=lock();if(!s)return std::unexpected(s.error());return attempt([&]{(*s)->options.show_actions=visible;(*s)->layout();(*s)->refresh();});}
ii::core::Result<void> ColorPicker::set_recent_colors(std::vector<ii::gui::WidgetColor> colors)const{auto s=lock();if(!s)return std::unexpected(s.error());if(colors.size()>10)colors.resize(10);for(auto& c:colors){if(!finite(c))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Recent colors must be finite");c=clamp(c);}return attempt([&]{(*s)->options.recent_colors=std::move(colors);for(auto& selected:(*s)->recent_selected)selected.reset();(*s)->layout();(*s)->refresh();});}
ii::core::Result<void> ColorPicker::set_color(ii::gui::WidgetColor color)const{auto s=lock();if(!s)return std::unexpected(s.error());return (*s)->model->edit_color(color);}
ii::core::Result<bool> ColorPicker::set_hex(std::string_view text)const{auto s=lock();if(!s)return std::unexpected(s.error());return (*s)->model->edit_hex(text);}
ii::core::Result<void> ColorPicker::accept()const{auto s=lock();if(!s)return std::unexpected(s.error());return attempt([&]{(*s)->accept();});}
ii::core::Result<void> ColorPicker::cancel()const{auto s=lock();if(!s)return std::unexpected(s.error());return attempt([&]{(*s)->cancel();});}
}
