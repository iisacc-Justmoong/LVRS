#include <LVRS/CodeEditorDetail.h>
#include <iiGUI/Desktop.h>
#include <iiGUI/TextRasterCache.h>
#include <iiSDKCore/Runtime/TaskScope.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <list>
#include <map>
#include <thread>
#include <unordered_set>

namespace lvrs {
using namespace ii;
using namespace code_detail;
namespace {
template<class T,class F>core::Result<T> protect(F&& f){
    try{return f();}catch(const Failure& e){return std::unexpected(e.error);}
    catch(const std::bad_alloc&){return core::fail(core::ErrorCode::full,"Code editor allocation failed");}
    catch(...){return core::fail(core::ErrorCode::failed,"Code editor operation failed",std::current_exception());}
}
void fill(gui::WidgetDrawing& drawing,double x,double y,double w,double h,gui::WidgetColor color,double width,double height){
    const auto right=std::min(width,x+w),bottom=std::min(height,y+h);x=std::max(0.,x);y=std::max(0.,y);
    if(right>x&&bottom>y&&color.alpha>0){auto c=ink(color);drawing.panels.push_back({{float(x),float(y),float(right-x),float(bottom-y)},c,c});}
}
gui::TextRange selected(TextPosition a,TextPosition b){return {std::size_t(std::min(a,b)),std::size_t(std::max(a,b)-std::min(a,b))};}
}
struct CodeEditor::Impl final: std::enable_shared_from_this<Impl> {
    struct Shape final {
        gui::TextLine line;
        std::size_t cost=0;
        std::uint64_t used=0;
    };
    struct Row final {
        std::size_t number=0;
        TextPosition start=0,length=0;
        double y=0;
        std::shared_ptr<Shape> shape;
        std::vector<gui::TextLineSpan> selection,marked;
        bool selected_newline=false;
        std::string error;
    };
    struct ViewHistory final { TextDocumentSelection before,after; };
    CodeEditorOptions options;
    CodeEditorEvents events;
    CodeEditorState value;
    Projection projection;
    gui::Widget root,viewport,body,caret;
    gui::WidgetColor primary;
    std::string chrome_font;
    gui::Rect bounds;
    gui::Rect native_caret;
    double width=180,height=144,top=34,view_height=110,origin_y=43,observed_width=0;
    std::optional<double> preferred_x;
    std::map<ShapeKey,std::shared_ptr<Shape>> shapes;
    std::unique_ptr<gui::TextRasterCache> rasters;
    std::vector<Row> rows;
    std::map<std::uint64_t,ViewHistory> selection_history;
    std::optional<TextDocumentSelection> pending_before;
    std::shared_ptr<TextDocumentPort> receiver;
    runtime::EventLoop::Sender loop;
    runtime::TaskScope lifetime;
    runtime::TaskToken blink;
    std::uint64_t use=0,composition_generation=0,blink_generation=0;
    std::thread::id owner=std::this_thread::get_id();
    bool refreshing=false,local_edit=false,dragging=false,caret_on=true,caret_in_view=false;
    std::atomic_bool wrong_dispatch=false;
    ~Impl(){(void)blink.cancel();lifetime.request_close();(void)lifetime.close();if(receiver)(void)receiver->close();}
    void ready()const{
        if(owner!=std::this_thread::get_id()||wrong_dispatch.load())throw Failure{{core::ErrorCode::wrong_executor,"Code editor requires its owner thread"}};
        take(root.placement());take(projection.model->state());
    }
    bool editable()const{
        const auto& s=projection.state;
        return options.enabled&&!options.read_only&&!s.loading&&s.last_error.empty()&&(!s.has_file_path||s.loaded_byte_count==s.total_byte_count);
    }
    double inset()const{return options.inset_horizontal+options.text_margin;}
    double line_height()const{return std::max(options.text_line_height,options.centered_text_height);}
    gui::TextLineStyle style()const{return {.font_family=options.font_family,.font_size=options.font_pixel_size,.line_height=line_height(),.tab_stop_distance=options.tab_stop_distance,.font_weight=options.font_weight};}
    void clear_shapes(){rows.clear();shapes.clear();good(rasters->clear());value.cache_bytes=value.raster_bytes=value.cached_lines=0;observed_width=0;}
    void remove_shape(std::map<ShapeKey,std::shared_ptr<Shape>>::iterator it){
        value.cache_bytes-=it->second->cost;shapes.erase(it);
    }
    std::shared_ptr<Shape> shape(ProjectedLine line){
        if(auto found=shapes.find(line.key);found!=shapes.end()){found->second->used=++use;return found->second;}
        if(line.index.text().size()>gui::TextLine::maximum_text_bytes)throw Failure{{core::ErrorCode::full,"Paragraph exceeds the 1 MiB native shaping budget"}};
        const auto cost=line.index.storage_bytes()+line.index.text().size()*48+4096;
        if(cost>options.shape_cache_bytes)throw Failure{{core::ErrorCode::full,"Paragraph exceeds the editor's shaping cache admission budget"}};
        while(value.cache_bytes>options.shape_cache_bytes-cost||shapes.size()>=options.shape_cache_entries){
            auto oldest=shapes.end();for(auto it=shapes.begin();it!=shapes.end();++it)if(it->second.use_count()==1&&(oldest==shapes.end()||it->second->used<oldest->second->used))oldest=it;
            if(oldest==shapes.end())throw Failure{{core::ErrorCode::full,"Visible paragraphs exceed the editor's bounded shaping cache"}};remove_shape(oldest);
        }
        auto result=std::make_shared<Shape>();result->line=take(gui::TextLine::create(std::move(line.index),style()));result->cost=cost;result->used=++use;
        const auto metrics=take(result->line.metrics());observed_width=std::max(observed_width,metrics.width);
        shapes.emplace(line.key,result);value.cache_bytes+=cost;value.cached_lines=shapes.size();++value.shaped_lines;return result;
    }
    void sync_state(){
        value.document_revision=projection.state.document_revision;value.length=projection.state.character_count;value.line_count=projection.lines();
        value.read_only=!editable();value.overwrite_mode=options.overwrite_mode;value.input_method_composing=bool(projection.composition);
        value.selection=selected(value.anchor_position,value.cursor_position);value.marked_range.reset();
        if(projection.composition)value.marked_range=gui::TextRange{std::size_t(projection.composition->first),projection.composition->text.utf16_size()};
        value.cursor_visible=value.focused&&editable()&&caret_on&&caret_in_view;value.cached_lines=shapes.size();
        const auto raster_state=take(rasters->stats());value.raster_bytes=raster_state.bytes;value.rasterizations=raster_state.rasterizations;
        value.selection_history_size=selection_history.size();
    }
    TextPosition native_cursor()const{const auto r=projection.native_selection(value.anchor_position,value.cursor_position);return projection.composition?r.location+r.length:value.cursor_position;}
    gui::TextInputState input_state()const{
        return {.length=std::size_t(projection.length()),.selection=projection.native_selection(value.anchor_position,value.cursor_position),.marked=value.marked_range,
            .caret=native_caret,.read_only=!editable(),.multiline=true,.auto_focus_on_press=options.auto_focus_on_press};
    }
    void prepare_rows(){
        rows.clear();for(auto it=shapes.begin();it!=shapes.end();){if(it->first.composition&&(!projection.composition||it->first.composition!=projection.composition->generation)){auto old=it++;remove_shape(old);}else ++it;}
        value.last_error=projection.state.last_error;
        const auto lh=line_height();const auto first=std::size_t(std::max(0.,std::floor((value.scroll_y-origin_y)/lh)));
        value.first_visible_line=std::min(first,projection.lines()-1);
        const auto selection=projection.native_selection(value.anchor_position,value.cursor_position);
        const auto end_selection=TextPosition(selection.location)+selection.length;
        for(auto row=value.first_visible_line;row<projection.lines();++row){
            const auto y=origin_y+double(row)*lh-value.scroll_y;if(y>=view_height)break;
            if(rows.size()>=512){value.last_error="Visible paragraph count exceeds the 512-row viewport budget";break;}
            Row line{.number=row,.y=y};
            try{
                auto projected=projection.line(row);line.start=projected.start;line.length=projected.index.utf16_size();line.shape=shape(std::move(projected));
                const auto first=std::max(TextPosition(selection.location),line.start),last=std::min(end_selection,line.start+line.length);
                if(selection.length&&last>first)line.selection=take(line.shape->line.selection(std::size_t(first-line.start),std::size_t(last-line.start)));
                line.selected_newline=selection.length&&end_selection>line.start+line.length&&TextPosition(selection.location)<=line.start+line.length;
                if(value.marked_range){const auto a=std::max(TextPosition(value.marked_range->location),line.start),b=std::min(TextPosition(value.marked_range->location)+value.marked_range->length,line.start+line.length);
                    if(b>a)line.marked=take(line.shape->line.selection(std::size_t(a-line.start),std::size_t(b-line.start)));}
            }catch(const Failure& e){line.error=e.error.message;value.last_error=e.error.message;}
            rows.push_back(std::move(line));
        }
        value.visible_lines=rows.size();
        try{
            const auto point=projection.locate(native_cursor());auto line=projection.line(point.line);auto shaped=shape(std::move(line));
            const auto x=inset()+take(shaped->line.caret(std::size_t(point.column)))-value.scroll_x;
            const auto y=top+origin_y+double(point.line)*lh-value.scroll_y;
            value.cursor_rectangle={float(x),float(y),1,float(lh)};caret_in_view=x+1>inset()&&x<width-inset()&&y+lh>top&&y<height;
            native_caret={float(std::clamp(x,0.,std::max(0.,width-1))),float(std::clamp(y,top,std::max(top,height-lh))),1,float(std::min(lh,height-top))};
        }catch(const Failure& e){value.last_error=e.error.message;caret_in_view=false;value.cursor_rectangle=native_caret={float(inset()),float(top),1,float(std::min(lh,view_height))};}
        sync_state();
    }
    void refresh(){
        if(refreshing)return;refreshing=true;struct Reset{bool& b;~Reset(){b=false;}}reset{refreshing};
        auto current=root.snapshot();if(!current){if(current.error().code==core::ErrorCode::closed)return;throw Failure{current.error()};}
        bounds=current->bounds;width=std::max(1.,double(bounds.width));height=std::max(1.,double(bounds.height));
        if(!current->font_family.empty())chrome_font=current->font_family;
        top=std::min(height-1,options.inset_vertical+(options.show_snippet_header?options.header_height+options.header_spacing:0));view_height=std::max(1.,height-top);
        const auto desired=std::max(options.field_min_height,options.editor_height);
        const auto area=std::max(0.,desired-top-options.inset_vertical);
        value.centered_text_y=top+std::max(0.,std::floor((area-options.centered_text_height)/2));origin_y=value.centered_text_y-top+options.text_margin;
        good(viewport.set_frame({0,float(top),float(width),float(view_height)}));good(body.set_extent({width,view_height}));
        good(caret.set_frame({0,0,float(width),float(view_height)}));
        auto v=take(viewport.snapshot());value.scroll_x=v.horizontal_scroll_offset;value.scroll_y=v.scroll_offset;sync_state();prepare_rows();
        value.content_height=std::max(view_height,origin_y+double(projection.lines())*line_height()+options.inset_vertical+options.text_margin);
        value.content_width=std::max(width,observed_width+inset()*2);
        good(viewport.set_scroll_options({.width=width,.height=view_height,.enabled=options.enabled,.show_indicator=options.show_scroll_bar,
            .virtual_content_height=value.content_height,.virtual_content_width=value.content_width,.horizontal_enabled=true}));
        v=take(viewport.snapshot());if(value.scroll_x!=v.horizontal_scroll_offset||value.scroll_y!=v.scroll_offset){value.scroll_x=v.horizontal_scroll_offset;value.scroll_y=v.scroll_offset;prepare_rows();}
        good(body.request_repaint());good(caret.request_repaint());good(root.request_repaint());
    }
    void scroll(double x,double y){
        if(!std::isfinite(x)||!std::isfinite(y))throw Failure{{core::ErrorCode::invalid_argument,"Editor scroll position must be finite"}};
        x=std::max(0.,x);y=std::max(0.,y);if(x==value.scroll_x&&y==value.scroll_y)return;
        const auto previous=refreshing;refreshing=true;
        {struct Reset{bool& target;bool old;~Reset(){target=old;}}reset{refreshing,previous};good(viewport.set_scroll_position(x,y));}
        if(!previous)refresh();
    }
    void reveal(){
        const auto point=projection.locate(native_cursor());auto line=shape(projection.line(point.line));const auto x=take(line->line.caret(std::size_t(point.column)))+inset();
        const auto y=origin_y+double(point.line)*line_height();
        const auto sx=std::max(0.,x<value.scroll_x+inset()?x-inset():x+inset()>value.scroll_x+width?x+inset()-width:value.scroll_x);
        const auto sy=std::max(0.,y<value.scroll_y?y:y+line_height()>value.scroll_y+view_height?y+line_height()-view_height:value.scroll_y);
        scroll(sx,sy);
    }
    TextPosition hit(double x,double y){
        const auto logical_y=y-top+value.scroll_y-origin_y;
        const auto row=std::size_t(std::clamp(std::floor(logical_y/line_height()),0.,double(projection.lines()-1)));
        auto line=projection.line(row);const auto start=line.start;auto shaped=shape(std::move(line));return start+take(shaped->line.hit_test(x-inset()+value.scroll_x));
    }
    void validate(TextPosition p){if(p>projection.state.character_count)throw Failure{{core::ErrorCode::out_of_range,"Editor position exceeds the committed document"}};(void)take(projection.model->line_column_for_position(p));}
    void selection_changed(){if(events.selection_changed)events.selection_changed(selected(value.anchor_position,value.cursor_position));}
    bool select(TextPosition anchor,TextPosition cursor,bool show=true){
        if(projection.composition)commit();validate(anchor);validate(cursor);
        if(value.anchor_position==anchor&&value.cursor_position==cursor)return false;
        value.anchor_position=anchor;value.cursor_position=cursor;++value.revision;preferred_x.reset();reset_blink();refresh();if(show)reveal();selection_changed();return true;
    }
    TextPosition previous(TextPosition pos){auto p=take(projection.model->line_column_for_position(pos));if(!p.column)return pos?pos-1:0;auto index=take(projection.model->line_index(p.line));return pos-p.column+index.previous_grapheme(std::size_t(p.column));}
    TextPosition next(TextPosition pos){auto p=take(projection.model->line_column_for_position(pos));auto index=take(projection.model->line_index(p.line));if(p.column==index.utf16_size())return std::min(projection.state.character_count,pos+1);return pos-p.column+index.next_grapheme(std::size_t(p.column));}
    bool move(TextCursorMove direction,bool extend){
        if(projection.composition)commit();auto pos=value.cursor_position;const auto p=take(projection.model->line_column_for_position(pos));
        const auto old_preferred=preferred_x;bool vertical=false;
        switch(direction){
            case TextCursorMove::left:pos=!extend&&value.selection.length?value.selection.location:previous(pos);break;
            case TextCursorMove::right:pos=!extend&&value.selection.length?value.selection.location+value.selection.length:next(pos);break;
            case TextCursorMove::line_start:pos-=p.column;break;
            case TextCursorMove::line_end:pos=pos-p.column+take(projection.model->line_length(p.line));break;
            case TextCursorMove::document_start:pos=0;break;
            case TextCursorMove::document_end:pos=projection.state.character_count;break;
            case TextCursorMove::word_left:pos=take(projection.model->previous_word_boundary_position(pos));break;
            case TextCursorMove::word_right:pos=take(projection.model->next_word_boundary_position(pos));break;
            case TextCursorMove::up:case TextCursorMove::down:{
                vertical=true;auto line=shape(projection.line(p.line));const auto x=old_preferred.value_or(take(line->line.caret(std::size_t(p.column))));
                const auto row=direction==TextCursorMove::up?(p.line?p.line-1:0):std::min(p.line+1,projection.lines()-1);auto target=projection.line(row);const auto start=target.start;auto shaped=shape(std::move(target));pos=start+take(shaped->line.hit_test(x));preferred_x=x;break;
            }
        }
        const auto x=preferred_x;auto changed=select(extend?value.anchor_position:pos,pos);if(vertical)preferred_x=x;return changed;
    }
    bool replace(TextPosition first,TextPosition last,std::string_view text){
        if(local_edit)throw Failure{{core::ErrorCode::busy,"Recursive editor replacement is not allowed"}};
        if(!editable())return false;validate(first);validate(last);if(first>last)throw Failure{{core::ErrorCode::invalid_argument,"Editor replacement is reversed"}};
        const auto before=projection.state.document_revision;pending_before=TextDocumentSelection{value.anchor_position,value.cursor_position};local_edit=true;
        struct Reset{Impl& p;~Reset(){p.local_edit=false;p.pending_before.reset();}}reset{*this};
        auto changed=projection.model->replace_range(first,last,text);auto current=take(projection.model->state());
        // The notification normally updates this View synchronously. Preserve
        // navigation performed by a user callback after that committed change.
        if(current.document_revision!=projection.state.document_revision){projection.state=current;value.anchor_position=value.cursor_position=current.cursor.position;++value.revision;projection.composition.reset();}
        if(current.document_revision!=before&&root.placement()){reset_blink();refresh();reveal();selection_changed();}
        if(!changed)throw Failure{changed.error()};return *changed;
    }
    bool replace_selection(std::string_view text){
        if(!editable())return false;
        if(projection.composition){auto c=std::move(*projection.composition);projection.composition.reset();const auto revision=projection.state.document_revision;
            try{auto changed=replace(c.first,c.last,text);refresh();return changed;}catch(...){if(projection.state.document_revision==revision){projection.composition=std::move(c);refresh();}throw;}}
        auto range=selected(value.anchor_position,value.cursor_position);auto last=TextPosition(range.location)+range.length;
        if(!range.length&&options.overwrite_mode&&text.find_first_of("\r\n")==text.npos){auto inserted=take(gui::TextIndex::create(std::string(text)));auto p=TextPosition(range.location);for(std::size_t i=0;i<inserted.utf16_size();i=inserted.next_grapheme(i)){const auto point=take(projection.model->line_column_for_position(p));if(point.column==take(projection.model->line_length(point.line)))break;p=next(p);}last=p;}
        return replace(range.location,last,text);
    }
    bool commit(){
        if(!projection.composition)return false;auto c=std::move(*projection.composition);projection.composition.reset();
        if(!editable()){refresh();return false;}const auto revision=projection.state.document_revision;
        try{auto changed=replace(c.first,c.last,c.text.text());refresh();return changed;}catch(...){if(projection.state.document_revision==revision){projection.composition=std::move(c);refresh();}throw;}
    }
    bool cancel(){if(!projection.composition)return false;projection.composition.reset();++value.revision;refresh();return true;}
    void compose(const gui::WidgetInput& e){
        if(!editable())return;if(e.text.empty()){cancel();return;}
        if(projection.composition&&e.replacement_range){auto mark=*value.marked_range;if(*e.replacement_range!=mark)commit();}
        auto range=projection.composition?gui::TextRange{std::size_t(projection.composition->first),std::size_t(projection.composition->last-projection.composition->first)}:selected(value.anchor_position,value.cursor_position);
        if(e.replacement_range&&!projection.composition)range=*e.replacement_range;
        if(range.location>projection.state.character_count||range.length>projection.state.character_count-range.location)throw Failure{{core::ErrorCode::out_of_range,"Native composition replacement exceeds the source document"}};
        validate(range.location);validate(range.location+range.length);
        Projection::Composition c;c.first=range.location;c.last=range.location+range.length;c.begin=take(projection.model->line_column_for_position(c.first));c.end=take(projection.model->line_column_for_position(c.last));
        c.text=take(gui::TextIndex::create(newlines(e.text)));c.selection=e.composition_selection.value_or(gui::TextRange{c.text.utf16_size(),0});
        if(c.selection.location>c.text.utf16_size()||c.selection.length>c.text.utf16_size()-c.selection.location)throw Failure{{core::ErrorCode::out_of_range,"Native preedit selection exceeds its text"}};
        take(c.text.byte_for_utf16(c.selection.location));take(c.text.byte_for_utf16(c.selection.location+c.selection.length));
        for(std::size_t byte=0;byte<c.text.text().size();++byte)if(c.text.text()[byte]=='\n')c.starts.push_back(take(c.text.utf16_for_byte(byte+1)));
        c.generation=++composition_generation;projection.composition=std::move(c);++value.revision;reset_blink();refresh();reveal();
    }
    std::string selected_text()const{auto range=projection.native_selection(value.anchor_position,value.cursor_position);return projection.range(range.location,TextPosition(range.location)+range.length);}
    void copy()const{auto range=projection.native_selection(value.anchor_position,value.cursor_position);if(range.length)good(gui::write_clipboard({.text=selected_text()}));}
    bool cut(){
        const auto range=projection.native_selection(value.anchor_position,value.cursor_position);if(!editable()||!range.length)return false;
        copy();if(projection.composition){commit();select(range.location,TextPosition(range.location)+range.length,false);}return replace_selection({});
    }
    bool select_all(){if(projection.composition)commit();return select(0,projection.state.character_count);}
    bool undo(bool redo){
        if(local_edit)throw Failure{{core::ErrorCode::busy,"Recursive editor undo is not allowed"}};
        if(!editable())return false;cancel();const auto before=projection.state.document_revision;
        auto result=redo?projection.model->redo():projection.model->undo();auto current=take(projection.model->state());
        if(current.document_revision!=projection.state.document_revision){projection.state=current;value.anchor_position=value.cursor_position=current.cursor.position;++value.revision;}
        if(current.document_revision!=before&&root.placement()){reset_blink();refresh();reveal();}
        if(!result)throw Failure{result.error()};return *result;
    }
    void submit(){if(projection.composition)commit();if(events.submitted)events.submitted(take(projection.model->text()));}
    void on_change(const TextDocumentChange& change){
        const auto was_editable=editable();
        const TextDocumentSelection previous{value.anchor_position,value.cursor_position};
        projection.state=change.state;
        if(!change.state.undo_depth&&!change.state.redo_depth)selection_history.clear();
        if(change.content){
            if(!local_edit)projection.composition.reset();
            if(change.reset){value.anchor_position=value.cursor_position=0;selection_history.clear();clear_shapes();}
            else if(change.edit){
                const auto& edit=*change.edit;
                if(local_edit&&edit.operation==TextEditOperation::replace){
                    value.anchor_position=value.cursor_position=edit.first+edit.inserted;projection.composition.reset();
                    if(edit.undoable&&options.selection_history_entries&&pending_before){
                        selection_history.insert_or_assign(edit.transaction,ViewHistory{*pending_before,{value.anchor_position,value.cursor_position}});
                        while(selection_history.size()>options.selection_history_entries)selection_history.erase(selection_history.begin());
                    }
                }else if(auto entry=selection_history.find(edit.transaction);edit.operation!=TextEditOperation::replace&&entry!=selection_history.end()){
                    const auto selection=edit.operation==TextEditOperation::undo?entry->second.before:entry->second.after;
                    value.anchor_position=selection.anchor;value.cursor_position=selection.cursor;
                }else{
                    const auto selection=take(map_text_selection(previous,edit));value.anchor_position=selection.anchor;value.cursor_position=selection.cursor;
                }
            }
            value.anchor_position=std::min(value.anchor_position,change.state.character_count);value.cursor_position=std::min(value.cursor_position,change.state.character_count);
            if(!change.edit){
                const auto snap=[&](TextPosition pos){auto point=projection.model->line_column_for_position(pos);if(!point&&pos&&point.error().code==core::ErrorCode::out_of_range){--pos;point=projection.model->line_column_for_position(pos);}take(std::move(point));return pos;};
                value.anchor_position=snap(value.anchor_position);value.cursor_position=snap(value.cursor_position);
            }
            ++value.revision;preferred_x.reset();
        }
        refresh();
        if(change.reset)scroll(0,0);
        if(was_editable!=editable())reset_blink();
        if(!local_edit&&previous!=TextDocumentSelection{value.anchor_position,value.cursor_position})selection_changed();
        if(change.edited){if(events.document_edited)events.document_edited(change);if(events.text_edited)events.text_edited(take(projection.model->text()));}
    }
    void schedule_blink(){
        if(!value.focused||!editable()||!root.placement())return;const auto generation=blink_generation;
        auto submitted=lifetime.at(loop,runtime::Clock::now()+std::chrono::milliseconds(500),[weak=weak_from_this(),generation]{
            if(auto self=weak.lock()){
                if(self->owner!=std::this_thread::get_id()){self->wrong_dispatch.store(true);return;}
                if(generation!=self->blink_generation||!self->value.focused||!self->editable()||!self->root.placement())return;
                self->caret_on=!self->caret_on;self->sync_state();auto repaint=self->caret.request_repaint();if(!repaint)return;self->schedule_blink();
            }
        });
        if(submitted)blink=submitted.token;else value.last_error="Core loop rejected caret blink; caret remains steady";
    }
    void reset_blink(){(void)blink.cancel();blink={};++blink_generation;caret_on=true;sync_state();schedule_blink();}
    void focus(bool focused){
        if(value.focused==focused)return;value.focused=focused;
        if(!focused){dragging=false;if(projection.composition)commit();}reset_blink();refresh();
    }
    bool input(const gui::WidgetInput& e){
        if(e.kind==gui::WidgetInputKind::focus_lost){dragging=false;if(projection.composition)commit();return false;}
        if(e.kind==gui::WidgetInputKind::commit_composition){commit();return true;}
        if(e.kind==gui::WidgetInputKind::composition){compose(e);return true;}
        if(e.kind==gui::WidgetInputKind::text){
            if(!editable())return true;
            if(e.replacement_range){if(projection.composition)commit();auto r=*e.replacement_range;replace(r.location,TextPosition(r.location)+r.length,e.content?e.content->text:e.text);}
            else replace_selection(e.content?e.content->text:e.text);return true;
        }
        if(e.kind==gui::WidgetInputKind::pointer_down){
            if(projection.composition)commit();dragging=options.select_by_mouse;
            if(dragging){auto p=hit(e.x-bounds.x,e.y-bounds.y);select(e.shift?value.anchor_position:p,p,false);}return true;
        }
        if(e.kind==gui::WidgetInputKind::pointer_move&&dragging){
            const auto y=e.y-bounds.y;if(y<top)scroll(value.scroll_x,value.scroll_y-line_height());else if(y>height)scroll(value.scroll_x,value.scroll_y+line_height());
            auto p=hit(e.x-bounds.x,y);select(value.anchor_position,p,false);return true;
        }
        if(e.kind==gui::WidgetInputKind::pointer_up){if(dragging){dragging=false;return true;}return false;}
        if(e.kind!=gui::WidgetInputKind::key_down)return false;
        const bool shortcut=e.control||e.meta,extend=e.shift&&options.select_by_keyboard;
        if(e.key==gui::WidgetKey::copy){copy();return true;}
        if(e.key==gui::WidgetKey::cut){cut();return true;}
        if(e.key==gui::WidgetKey::select_all){select_all();return true;}
        if(e.key==gui::WidgetKey::undo||e.key==gui::WidgetKey::redo){undo(e.key==gui::WidgetKey::redo);return true;}
        if(shortcut&&e.key==gui::WidgetKey::character){if(e.text=="z"){undo(e.shift);return true;}if(e.text=="y"){undo(true);return true;}}
        if(e.key==gui::WidgetKey::escape){if(projection.composition){cancel();return true;}return false;}
        if(e.key==gui::WidgetKey::insert){options.overwrite_mode=!options.overwrite_mode;sync_state();return true;}
        if(e.key==gui::WidgetKey::enter){if(shortcut)submit();else replace_selection("\n");return true;}
        if(e.key==gui::WidgetKey::tab){if(e.shift||!editable())return false;replace_selection("\t");return true;}
        if(e.key==gui::WidgetKey::backspace||e.key==gui::WidgetKey::delete_forward){
            if(!editable())return true;if(projection.composition)commit();auto r=selected(value.anchor_position,value.cursor_position);
            if(r.length)replace(r.location,TextPosition(r.location)+r.length,{});else if(e.key==gui::WidgetKey::backspace)replace(previous(value.cursor_position),value.cursor_position,{});else replace(value.cursor_position,next(value.cursor_position),{});return true;
        }
        std::optional<TextCursorMove> direction;
        if(e.key==gui::WidgetKey::left)direction=e.meta?TextCursorMove::line_start:(e.control||e.alt)?TextCursorMove::word_left:TextCursorMove::left;
        if(e.key==gui::WidgetKey::right)direction=e.meta?TextCursorMove::line_end:(e.control||e.alt)?TextCursorMove::word_right:TextCursorMove::right;
        if(e.key==gui::WidgetKey::up)direction=e.meta?TextCursorMove::document_start:TextCursorMove::up;
        if(e.key==gui::WidgetKey::down)direction=e.meta?TextCursorMove::document_end:TextCursorMove::down;
        if(e.key==gui::WidgetKey::home)direction=shortcut?TextCursorMove::document_start:TextCursorMove::line_start;
        if(e.key==gui::WidgetKey::end)direction=shortcut?TextCursorMove::document_end:TextCursorMove::line_end;
        if(direction){move(*direction,extend);return true;}
        if(e.key==gui::WidgetKey::page_up||e.key==gui::WidgetKey::page_down){
            if(projection.composition)commit();const auto point=projection.locate(value.cursor_position);auto current=shape(projection.line(point.line));
            const auto x=preferred_x.value_or(take(current->line.caret(std::size_t(point.column))));const auto steps=std::size_t(std::max(1.,std::floor(view_height/line_height())));
            const auto row=e.key==gui::WidgetKey::page_up?(point.line>steps?point.line-steps:0):std::min(projection.lines()-1,point.line+steps);
            auto target=projection.line(row);const auto start=target.start;auto shaped=shape(std::move(target));const auto pos=start+take(shaped->line.hit_test(x));
            select(extend?value.anchor_position:pos,pos);preferred_x=x;return true;
        }
        return false;
    }
    std::shared_ptr<const gui::WidgetDrawing> background(const gui::WidgetPaintState& p)const{
        auto d=std::make_shared<gui::WidgetDrawing>();auto c=!p.enabled?options.background_color_disabled.value_or(options.background_color):p.focused?options.background_color_focused.value_or(options.background_color):p.hovered?options.background_color_hover:options.background_color;
        auto color=ink(c);d->panels.push_back({{0,0,float(p.size.width),float(p.size.height)},color,color,float(std::min(options.corner_radius,std::min(p.size.width,p.size.height)/2))});
        if(p.focused&&p.enabled){auto focus=ink(primary);d->panels.push_back({{0,0,float(p.size.width),float(p.size.height)},focus,focus,float(options.corner_radius),1});}
        if(options.show_snippet_header){auto text=options.snippet_title;if(!options.snippet_language.empty())text+=" - "+options.snippet_language;
            d->texts.push_back({.bounds={float(options.inset_horizontal),float(options.inset_vertical),float(std::max(1.,p.size.width-options.inset_horizontal*2)),11},.text=std::move(text),.color=ink(options.header_text_color),.font_size=11,.font_weight=400,.font_family=chrome_font,.line_height=11,.elide_right=true});}
        return d;
    }
    std::shared_ptr<const gui::WidgetDrawing> paint(const gui::WidgetPaintState& p){
        auto d=std::make_shared<gui::WidgetDrawing>();const auto w=std::max(1.,width-inset()*2),h=view_height;
        const auto selection_color=options.selection_color.value_or(primary);const auto normal=p.enabled?options.text_color:options.text_color_disabled;
        const bool show_selection=p.focused||options.persistent_selection;
        std::size_t frame_bytes=0;
        std::unordered_set<const gui::RasterImage*> frame_images;
        for(auto& row:rows){
            if(!row.shape){d->texts.push_back({.bounds={float(inset()),float(std::max(0.,row.y)),float(w),16},.text="Paragraph unavailable: "+row.error,.color=ink(options.placeholder_color),.font_size=11,.line_height=16,.elide_right=true});continue;}
            try{
                std::vector<std::pair<double,double>> selected_spans;
                if(show_selection)for(const auto& span:row.selection){auto a=std::clamp(span.x-value.scroll_x,0.,w),b=std::clamp(span.x+span.width-value.scroll_x,0.,w);if(b>a)selected_spans.emplace_back(a,b);}
                std::sort(selected_spans.begin(),selected_spans.end());std::vector<std::pair<double,double>> merged;
                for(auto span:selected_spans){if(!merged.empty()&&span.first<=merged.back().second)merged.back().second=std::max(merged.back().second,span.second);else merged.push_back(span);}
                for(auto [a,b]:merged)fill(*d,inset()+a,row.y,b-a,line_height(),selection_color,width,h);
                if(show_selection&&row.selected_newline){const auto end=take(row.shape->line.caret(std::size_t(row.length)))-value.scroll_x;fill(*d,inset()+end,row.y,w-end,line_height(),selection_color,width,h);}
                if(row.length){
                    const auto part=[&](double a,double b,gui::WidgetColor color){if(b<=a||color.alpha<=0)return;if(d->images.size()>=4096)throw Failure{{core::ErrorCode::full,"Visible bidi selection exceeds the image-run budget"}};
                        auto painted=take(rasters->paint(row.shape->line,value.scroll_x,w,ink(color),p.backing_scale));
                        if(frame_images.insert(painted.image.get()).second){
                            const auto bytes=painted.image->rgba().size();
                            if(bytes>options.raster_cache_bytes||frame_bytes>options.raster_cache_bytes-bytes)throw Failure{{core::ErrorCode::full,"Visible text palette images exceed the editor's per-frame raster budget"}};
                            frame_bytes+=bytes;
                        }
                        d->images.push_back({.bounds={float(inset()+a),float(row.y),float(b-a),float(line_height())},.image=std::move(painted.image),.opacity=painted.opacity,.source={float(a/w),0,float((b-a)/w),1}});};
                    double x=0;for(auto [a,b]:merged){part(x,a,normal);part(a,b,options.selected_text_color);x=b;}part(x,w,normal);
                }
                for(const auto& span:row.marked)fill(*d,inset()+span.x-value.scroll_x,row.y+line_height()-1,span.width,1,primary,width,h);
            }catch(const Failure& error){value.last_error=error.error.message;d->texts.push_back({.bounds={float(inset()),float(std::max(0.,row.y)),float(w),16},.text=error.error.message,.color=ink(options.placeholder_color),.font_size=11,.line_height=16,.elide_right=true});}
        }
        if(!projection.length()&&!options.placeholder_text.empty()){
            auto color=p.enabled?options.placeholder_color:options.placeholder_color_disabled;color.alpha*=options.placeholder_opacity;
            d->texts.push_back({.bounds={float(inset()),float(origin_y-value.scroll_y),float(w),float(std::max(1.,h-origin_y+value.scroll_y))},.text=options.placeholder_text,.color=ink(color),.font_size=12,.font_weight=600,.font_family=chrome_font,.line_height=float(options.centered_text_height)});
        }
        return d;
    }
    std::shared_ptr<const gui::WidgetDrawing> paint_caret()const{
        auto drawing=std::make_shared<gui::WidgetDrawing>();if(value.cursor_visible){auto r=value.cursor_rectangle;fill(*drawing,r.x,r.y-top,r.width,r.height,options.text_color,width,view_height);}return drawing;
    }
};

core::Result<CodeEditor> CodeEditor::create(gui::WidgetBuilder& builder,gui::Widget parent,std::string_view id,CodeEditorOptions options,CodeEditorEvents events){
    return protect<CodeEditor>([&]()->core::Result<CodeEditor>{
        if(!code_detail::valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid CodeEditor options");
        if(options.model&&!options.text.empty())return core::fail(core::ErrorCode::invalid_argument,"Use a model or initial text, not both");
        auto loop=take(builder.event_loop());auto self=std::make_shared<Impl>();self->loop=loop.get().sender();self->options=std::move(options);self->events=std::move(events);const auto appearance=take(builder.appearance());self->primary=appearance.primary;self->chrome_font=appearance.font_family;
        self->rasters=take(gui::TextRasterCache::create({.maximum_bytes=self->options.raster_cache_bytes,.maximum_entries=self->options.shape_cache_entries*2}));
        if(self->options.font_family.empty()){
#if defined(__APPLE__)
            self->options.font_family="Menlo";
#else
            self->options.font_family="Monospace";
#endif
        }
        self->projection.model=self->options.model?self->options.model:take(TextDocumentModel::create(loop.get()));
        if(!self->options.text.empty())take(self->projection.model->set_text(self->options.text));self->options.text.clear();self->options.model=self->projection.model;self->projection.state=take(self->projection.model->state());
        const auto size=gui::Size{self->options.width,std::max(self->options.editor_height,self->options.field_min_height)};
        self->root=take(builder.absolute_stack(parent,id,size));self->viewport=take(builder.scroll_view(self->root,std::string(id)+".viewport",{.width=size.width,.height=size.height,.horizontal_enabled=true}));
        self->body=take(builder.absolute_stack(self->viewport,std::string(id)+".body",size));self->caret=take(builder.absolute_stack(self->body,std::string(id)+".caret",size));good(self->root.set_clip_content(true));
        auto weak=std::weak_ptr<Impl>(self);
        good(self->root.set_paint_handler([weak](const auto& p)->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{return protect<std::shared_ptr<const gui::WidgetDrawing>>([&]()->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{if(auto s=weak.lock())return s->background(p);return core::fail(core::ErrorCode::closed,"Code editor retired");});}));
        good(self->body.set_paint_handler([weak](const auto& p)->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{return protect<std::shared_ptr<const gui::WidgetDrawing>>([&]()->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{if(auto s=weak.lock())return s->paint(p);return core::fail(core::ErrorCode::closed,"Code editor retired");});},self->root));
        good(self->caret.set_paint_handler([weak](const auto&)->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{return protect<std::shared_ptr<const gui::WidgetDrawing>>([&]()->core::Result<std::shared_ptr<const gui::WidgetDrawing>>{if(auto s=weak.lock())return s->paint_caret();return core::fail(core::ErrorCode::closed,"Code editor retired");});},self->root));
        good(self->root.set_input_handler([weak](const auto& e)->core::Result<bool>{return protect<bool>([&]()->core::Result<bool>{if(auto s=weak.lock())return s->input(e);return false;});}));
        good(self->root.set_layout_handler([weak](gui::Rect){if(auto s=weak.lock())s->refresh();}));
        good(self->viewport.set_scroll_handler([weak](const gui::ScrollViewport& v){if(auto s=weak.lock()){s->value.scroll_x=v.horizontal_offset;s->value.scroll_y=v.offset;if(!s->refreshing)s->refresh();}}));
        auto input=std::make_shared<gui::TextInputProvider>();
        input->state=[weak]()->core::Result<gui::TextInputState>{if(auto s=weak.lock())return s->input_state();return core::fail(core::ErrorCode::closed,"Code editor retired");};
        input->read=[weak](gui::TextRange r)->core::Result<gui::TextContent>{return protect<gui::TextContent>([&]()->core::Result<gui::TextContent>{if(auto s=weak.lock())return gui::TextContent{r,s->projection.range(r.location,TextPosition(r.location)+r.length)};return core::fail(core::ErrorCode::closed,"Code editor retired");});};
        input->position=[weak](double x,double y)->core::Result<std::size_t>{return protect<std::size_t>([&]()->core::Result<std::size_t>{if(auto s=weak.lock())return std::size_t(s->hit(x,y));return core::fail(core::ErrorCode::closed,"Code editor retired");});};
        input->focus_changed=[weak](bool value)->core::Result<void>{return protect<void>([&]()->core::Result<void>{if(auto s=weak.lock())s->focus(value);return {};});};good(self->root.set_text_input_provider(std::move(input)));good(self->root.set_accessibility_name(self->options.accessibility_name));
        self->receiver=std::make_shared<TextDocumentPort>([weak](const TextDocumentChange& change){if(auto s=weak.lock())s->on_change(change);});good(self->projection.model->observe(self->receiver));
        good(self->root.set_enabled(self->options.enabled));good(builder.retain(self));self->refresh();CodeEditor view;view.root_=self->root;view.viewport_=self->viewport;view.body_=self->body;view.state_=self;return view;
    });
}
template<class T,class F>core::Result<T> CodeEditor::with_state(F&& f)const{
    return protect<T>([&]()->core::Result<T>{auto p=state_.lock();if(!p)return core::fail(core::ErrorCode::closed,"Code editor retired");p->ready();return f(*p);});
}
core::Result<std::shared_ptr<TextDocumentModel>> CodeEditor::model()const{return with_state<std::shared_ptr<TextDocumentModel>>([](auto& p){return p.projection.model;});}
core::Result<CodeEditorState> CodeEditor::state()const{return with_state<CodeEditorState>([](auto& p){p.sync_state();return p.value;});}
core::Result<CodeEditorOptions> CodeEditor::options()const{return with_state<CodeEditorOptions>([](auto& p){return p.options;});}
core::Result<void> CodeEditor::set_options(CodeEditorOptions options)const{return with_state<void>([&](auto& p)->core::Result<void>{
    if(!code_detail::valid(options)||(!options.text.empty())||(options.model&&options.model!=p.projection.model))return core::fail(core::ErrorCode::invalid_argument,"Change editor text with set_text and model ownership with a new View");
    if(options.font_family.empty())options.font_family=p.options.font_family;const auto old=p.style();
    if(options.read_only||!options.enabled)p.cancel();options.model=p.projection.model;good(p.rasters->set_limits({.maximum_bytes=options.raster_cache_bytes,.maximum_entries=options.shape_cache_entries*2}));p.options=std::move(options);if(old!=p.style())p.clear_shapes();
    while(p.selection_history.size()>p.options.selection_history_entries)p.selection_history.erase(p.selection_history.begin());
    good(p.root.set_enabled(p.options.enabled));good(p.root.set_accessibility_name(p.options.accessibility_name));good(p.root.set_extent({p.options.width,std::max(p.options.editor_height,p.options.field_min_height)}));p.reset_blink();p.refresh();return {};
});}
core::Result<bool> CodeEditor::set_text(std::string_view text)const{return with_state<bool>([&](auto& p){p.cancel();return p.projection.model->set_text(text);});}
core::Result<std::string> CodeEditor::text()const{return with_state<std::string>([](auto& p){return p.projection.model->text();});}
core::Result<std::string> CodeEditor::selected_text()const{return with_state<std::string>([](auto& p){return p.selected_text();});}
core::Result<bool> CodeEditor::select(TextPosition anchor,TextPosition cursor)const{return with_state<bool>([&](auto& p){return p.select(anchor,cursor);});}
core::Result<bool> CodeEditor::select_all()const{return with_state<bool>([](auto& p){return p.select_all();});}
core::Result<bool> CodeEditor::deselect()const{return with_state<bool>([](auto& p){if(p.projection.composition)p.commit();return p.select(p.value.cursor_position,p.value.cursor_position);});}
core::Result<bool> CodeEditor::set_cursor_position(TextPosition pos,bool extend)const{return with_state<bool>([&](auto& p){return p.select(extend?p.value.anchor_position:pos,pos);});}
core::Result<bool> CodeEditor::move_cursor(TextCursorMove move,bool extend)const{return with_state<bool>([&](auto& p){return p.move(move,extend);});}
core::Result<bool> CodeEditor::insert_text(std::string_view text)const{return with_state<bool>([&](auto& p){if(p.projection.composition)p.commit();return p.replace(p.value.cursor_position,p.value.cursor_position,text);});}
core::Result<bool> CodeEditor::replace_selection(std::string_view text)const{return with_state<bool>([&](auto& p){return p.replace_selection(text);});}
core::Result<bool> CodeEditor::clear()const{return set_text({});}
core::Result<bool> CodeEditor::undo()const{return with_state<bool>([](auto& p){return p.undo(false);});}
core::Result<bool> CodeEditor::redo()const{return with_state<bool>([](auto& p){return p.undo(true);});}
core::Result<void> CodeEditor::copy()const{return with_state<void>([](auto& p)->core::Result<void>{p.copy();return {};});}
core::Result<bool> CodeEditor::cut()const{return with_state<bool>([](auto& p){return p.cut();});}
core::Result<bool> CodeEditor::paste()const{return with_state<bool>([](auto& p){if(!p.editable())return false;return p.replace_selection(take(gui::read_clipboard()).text);});}
core::Result<void> CodeEditor::submit()const{return with_state<void>([](auto& p)->core::Result<void>{p.submit();return {};});}
core::Result<void> CodeEditor::force_editor_focus()const{return with_state<void>([](auto& p){return p.root.focus();});}
core::Result<void> CodeEditor::set_scroll_position(double x,double y)const{return with_state<void>([&](auto& p)->core::Result<void>{p.scroll(x,y);return {};});}
core::Result<void> CodeEditor::reveal_cursor()const{return with_state<void>([](auto& p)->core::Result<void>{p.refresh();p.reveal();return {};});}
core::Result<bool> CodeEditor::commit_composition()const{return with_state<bool>([](auto& p){return p.commit();});}
core::Result<bool> CodeEditor::cancel_composition()const{return with_state<bool>([](auto& p){return p.cancel();});}
} // namespace lvrs
