#include <LVRS/List.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace lvrs {
using namespace ii;
namespace {
struct Failure final{core::Error error;};
template<class T>T take(core::Result<T> value){if(!value)throw Failure{value.error()};return std::move(*value);}
void good(core::Result<void> value){if(!value)throw Failure{value.error()};}
gui::Color ink(double r,double g,double b,double alpha=1){const auto linear=[](double c){return float(c<=.04045?c/12.92:std::pow((c+.055)/1.055,2.4));};return {linear(r),linear(g),linear(b),float(alpha)};}
template<class F>core::Result<void> attempt(F&& work){try{work();return {};}catch(const Failure& error){return std::unexpected(error.error);}}
void dispatch(core::Result<void> value){if(!value&&value.error().code!=core::ErrorCode::closed)throw std::runtime_error(value.error().message);}
}
struct List::Impl final {
    gui::Widget root,viewport,body,editor;ListOptions options;ListEvents events;std::shared_ptr<ListModel> model;
    core::ObservableSubscription changed,selection;std::vector<ListVisibleRow> rows;
    double width=0,height=0,offset=0,press_x=0,press_y=0,last_click=0;std::string pressed,last_key,editing,draft;bool dragging=false,drawing=false;std::size_t painted=0;
    void draw(){
        if(drawing||!body.snapshot())return;drawing=true;struct Reset final{bool& value;~Reset(){value=false;}}reset{drawing};
        const auto color=[](gui::WidgetColor c){return ink(c.red,c.green,c.blue,c.alpha);};
        gui::WidgetDrawing output;output.panels.push_back({{0,0,float(width),float(height)},color(options.background),color(options.background)});painted=0;
        const auto selected=model->selection().get();const auto first=std::min(rows.size(),std::size_t(std::max(0.,std::floor(offset/options.row_height))));
        for(std::size_t i=first;i<rows.size();++i){const double y=i*options.row_height-offset;if(y>=height)break;++painted;
            const auto& row=model->rows()[rows[i].index];const auto text_color=row.enabled?color(options.text_color):ink(.5,.5,.5);
            if(row.key==selected){const float top=float(std::max(0.,y)),bottom=float(std::min(height,y+options.row_height));output.panels.push_back({{0,top,float(width),std::max(0.f,bottom-top)},color(options.selection_color),color(options.selection_color),float(options.selection_radius)});}
            const auto x=8+rows[i].depth*options.indent;const auto text_height=std::min(options.row_height,std::max(16.,options.font_size*16/13));const auto baseline=y+(options.row_height-text_height)/2;
            if(rows[i].has_children)output.texts.push_back({{float(x),float(baseline),16,16},rows[i].expanded?"▾":"▸",text_color,12,400,gui::TextAlignment::left,"Pretendard"});
            const auto label_x=x+(options.hierarchy?18:0);
            output.texts.push_back({{float(label_x),float(baseline),float(std::max(0.,width-label_x-(options.show_checks?28:8))),float(text_height)},row.text,text_color,float(options.font_size),400,gui::TextAlignment::left,"Pretendard",float(text_height),true});
            if(options.show_checks)output.texts.push_back({{float(width-25),float(baseline),20,16},row.checked?"●":"○",text_color,12,400,gui::TextAlignment::center,"Pretendard"});
        }good(body.set_drawing(std::move(output)));
        if(!editing.empty()){
            auto found=std::ranges::find_if(rows,[&](const auto& row){return model->rows()[row.index].key==editing;});
            if(found==rows.end()){editing.clear();good(editor.set_visible(false));}
            else{const auto y=std::size_t(found-rows.begin())*options.row_height-offset;good(editor.set_frame({float(8+found->depth*options.indent),float(y),float(std::max(32.,width-16-found->depth*options.indent)),float(options.row_height)}));}
        }
    }
    void refresh(){rows=take(model->visible_rows(options.hierarchy));good(viewport.set_scroll_options({.width=width,.height=height,.virtual_content_height=rows.size()*options.row_height}));draw();}
    void geometry(gui::Rect bounds){width=std::max(1.f,bounds.width);height=std::max(1.f,bounds.height);good(viewport.set_frame({0,0,float(width),float(height)}));good(body.set_extent({width,height}));refresh();}
    std::optional<std::size_t> at(double y)const{if(y<0||y>=height)return {};const auto index=std::size_t((y+offset)/options.row_height);return index<rows.size()?std::optional{index}:std::nullopt;}
    void select(std::string key){if(take(model->select(key))&&events.selected)events.selected(std::move(key));}
    void rename(){
        const auto key=model->selection().get();auto row=take(model->row(key));if(!options.editable||!row.editable||!row.enabled)return;
        editing=key;draft=row.text;good(editor.set_text(draft));good(editor.set_visible(true));draw();good(editor.focus());
    }
    void commit(){
        if(editing.empty())return;const auto key=std::exchange(editing,{});good(editor.set_visible(false));
        if(take(model->rename(key,draft))&&events.renamed)events.renamed(key,draft);good(body.focus());draw();
    }
    core::Result<bool> input(const gui::WidgetInput& event){try{
        if(event.kind==gui::WidgetInputKind::focus_lost){pressed.clear();dragging=false;return false;}
        if(event.kind==gui::WidgetInputKind::pointer_down){auto index=at(event.y);if(!index)return false;const auto info=rows[*index];const auto row=model->rows()[info.index];if(!row.enabled)return true;
            const auto x=8+info.depth*options.indent;
            if(options.hierarchy&&info.has_children&&event.x>=x&&event.x<x+18){take(model->set_expanded(row.key,!info.expanded));return true;}
            if(options.show_checks&&event.x>=width-28){const bool value=!row.checked;if(take(model->set_checked(row.key,value))&&events.checked)events.checked(row.key,value);return true;}
            select(row.key);pressed=row.key;press_x=event.x;press_y=event.y;dragging=false;
            if(options.editable&&row.editable&&last_key==row.key&&event.timestamp>0&&event.timestamp-last_click<.4){rename();pressed.clear();}
            last_key=row.key;last_click=event.timestamp;return true;
        }
        if(event.kind==gui::WidgetInputKind::pointer_move&&!pressed.empty()){if(options.reorderable&&std::hypot(event.x-press_x,event.y-press_y)>5)dragging=true;return true;}
        if(event.kind==gui::WidgetInputKind::pointer_up&&!pressed.empty()){
            const auto key=std::exchange(pressed,{});if(dragging){dragging=false;auto index=at(event.y);if(index){const auto target=model->rows()[rows[*index].index];std::string parent=target.parent,before=target.key;
                    if(options.hierarchy&&event.x>8+(rows[*index].depth+1)*options.indent){parent=target.key;before.clear();}
                    auto moved=model->move(key,parent,before);if(moved&&*moved&&events.moved)events.moved(key,parent,before);}
            }return true;
        }
        if(event.kind!=gui::WidgetInputKind::key_down||event.text_input_active)return false;
        const auto key=model->selection().get();auto found=std::ranges::find_if(rows,[&](const auto& row){return model->rows()[row.index].key==key;});auto index=found==rows.end()?std::size_t(0):std::size_t(found-rows.begin());
        if(event.key==gui::WidgetKey::up||event.key==gui::WidgetKey::down||event.key==gui::WidgetKey::home||event.key==gui::WidgetKey::end){
            if(rows.empty())return true;const int direction=event.key==gui::WidgetKey::up||event.key==gui::WidgetKey::end?-1:1;
            if(event.key==gui::WidgetKey::home)index=0;else if(event.key==gui::WidgetKey::end)index=rows.size()-1;else if(found!=rows.end()){if(direction<0&&index)--index;else if(direction>0&&index+1<rows.size())++index;}
            for(std::size_t steps=0;steps<rows.size();++steps){const auto& row=model->rows()[rows[index].index];if(row.enabled){select(row.key);const auto top=index*options.row_height;good(viewport.set_scroll_offset(top<offset?top:top+options.row_height>offset+height?top+options.row_height-height:offset));break;}if(direction<0&&index)--index;else if(direction>0&&index+1<rows.size())++index;else break;}return true;
        }
        if(found!=rows.end()&&options.hierarchy&&(event.key==gui::WidgetKey::left||event.key==gui::WidgetKey::right)){
            const auto row=model->rows()[found->index];if(event.key==gui::WidgetKey::left){if(found->has_children&&found->expanded)take(model->set_expanded(row.key,false));else if(!row.parent.empty())select(row.parent);}else if(found->has_children)take(model->set_expanded(row.key,true));return true;
        }
        if(event.key==gui::WidgetKey::enter&&!key.empty()){if(options.editable)rename();else if(events.activated)events.activated(key);return true;}
        return false;
    }catch(const Failure& error){return std::unexpected(error.error);}}
};
core::Result<List> List::create(gui::WidgetBuilder& builder,gui::Widget parent,std::string_view id,ListOptions options,ListEvents events){
    const auto valid=[](gui::WidgetColor c){for(double v:{c.red,c.green,c.blue,c.alpha})if(!std::isfinite(v)||v<0||v>1)return false;return true;};
    if(!std::isfinite(options.font_size)||options.font_size<1||options.font_size>256)return core::fail(core::ErrorCode::invalid_argument,"Invalid native list font size");
    if(!valid(options.background)||!valid(options.selection_color)||!valid(options.text_color)||!std::isfinite(options.selection_radius)||options.selection_radius<0||options.selection_radius>128)return core::fail(core::ErrorCode::invalid_argument,"Invalid native list appearance");
    if(!std::isfinite(options.width)||!std::isfinite(options.height)||!std::isfinite(options.row_height)||!std::isfinite(options.indent)||options.width<32||options.width>8192||options.height<24||options.height>8192||options.row_height<18||options.row_height>256||options.indent<0||options.indent>128)return core::fail(core::ErrorCode::invalid_argument,"Invalid native list geometry");
    try{auto state=std::make_shared<Impl>();state->options=std::move(options);state->events=std::move(events);state->model=state->options.model?state->options.model:std::make_shared<ListModel>();state->width=state->options.width;state->height=state->options.height;
        const auto prefix=std::string(id);state->root=take(builder.absolute_stack(parent,id,{state->width,state->height}));good(state->root.set_clip_content(true));
        state->viewport=take(builder.scroll_view(state->root,prefix+".viewport",{.width=state->width,.height=state->height,.virtual_content_height=0}));
        state->body=take(builder.absolute_stack(state->viewport,prefix+".body",{state->width,state->height}));
        auto weak=std::weak_ptr(state);state->editor=take(builder.input_field(state->root,prefix+".editor",{.width=state->width,.commit_on_focus_loss=true},[weak](std::string text){if(auto self=weak.lock())self->draft=std::move(text);},[weak]{if(auto self=weak.lock())self->commit();}));good(state->editor.set_visible(false));
        good(state->editor.set_escape_action([weak]{if(auto self=weak.lock()){self->editing.clear();good(self->editor.set_visible(false));good(self->body.focus());self->draw();}}));
        good(state->root.set_layout_handler([weak](gui::Rect bounds){if(auto self=weak.lock())self->geometry(bounds);}));
        good(state->viewport.set_scroll_handler([weak](const auto& value){if(auto self=weak.lock()){self->offset=value.offset;self->width=value.bounds.width;self->height=value.bounds.height;self->draw();}}));
        good(state->body.set_input_handler([weak](const auto& event)->core::Result<bool>{if(auto self=weak.lock())return self->input(event);return false;}));
        state->changed=state->model->changes().observe([weak](auto,auto){if(auto self=weak.lock())self->refresh();});state->selection=state->model->selection().observe([weak](const auto&,auto){if(auto self=weak.lock())self->draw();});
        state->refresh();good(builder.retain(state));List view;view.root_=state->root;view.state_=state;return view;
    }catch(const Failure& error){return std::unexpected(error.error);}
}
core::Result<ListViewState> List::snapshot()const{if(auto ready=root_.snapshot();!ready)return std::unexpected(ready.error());auto state=state_.lock();if(!state)return core::fail(core::ErrorCode::closed,"List view closed");return ListViewState{state->model->selection().get(),state->offset,state->rows.size(),state->painted,!state->editing.empty()};}
core::Result<void> List::begin_rename()const{if(auto ready=root_.snapshot();!ready)return std::unexpected(ready.error());auto state=state_.lock();if(!state)return core::fail(core::ErrorCode::closed,"List view closed");return attempt([&]{state->rename();});}
core::Result<void> List::set_scroll_offset(double value)const{auto state=state_.lock();if(!state)return core::fail(core::ErrorCode::closed,"List view closed");return state->viewport.set_scroll_offset(value);}
}
