#include <LVRS/Table.h>
#include <LVRS/TableDetail.h>
#include <LVRS/Icons.h>
#include <iiGUI/Desktop.h>
#include <iiGUI/Text.h>
#include <algorithm>
#include <cmath>
#include <type_traits>
#include <utility>

namespace lvrs {
using namespace ii;
using namespace table_detail;
namespace {
gui::Color ink(gui::WidgetColor value){
    const auto linear=[](double channel){return float(channel<=.04045?channel/12.92:std::pow((channel+.055)/1.055,2.4));};
    return {linear(value.red),linear(value.green),linear(value.blue),float(value.alpha)};
}
double gutter_width(const TableOptions& o){return o.structure_controls_visible&&o.add_row_controls_visible?o.structure_gutter_width:0;}
double gutter_height(const TableOptions& o){return o.structure_controls_visible&&o.add_column_controls_visible?o.structure_gutter_height:0;}
bool valid(const TableOptions& o){
    return metric(o.width,32)&&metric(o.height,26)&&metric(o.header_height,1)&&o.header_height+1<o.height&&metric(o.content_spacing)&&metric(o.border_width)&&o.border_width<=std::min(o.width,o.height)/2&&unsigned(o.selection_mode)<=2
        &&metric(o.column_resize_handle_width)&&metric(o.row_resize_handle_height)&&metric(o.structure_gutter_width)&&metric(o.structure_gutter_height)
        &&o.width-gutter_width(o)>=32&&o.header_height+1<o.height-gutter_height(o)
        &&color(o.resize_handle_color)&&(!o.resize_handle_hover_color||color(*o.resize_handle_hover_color))
        &&color(o.background)&&color(o.divider)&&color(o.header_text)&&color(o.cell_text)&&(!o.selection_color||color(*o.selection_color))&&(!o.current_color||color(*o.current_color));
}
bool contains(gui::Rect r,double x,double y){return x>=r.x&&y>=r.y&&x<r.x+r.width&&y<r.y+r.height;}
bool intersects(TableRange a,TableRange b){return a.row<b.row+b.row_count&&b.row<a.row+a.row_count&&a.column<b.column+b.column_count&&b.column<a.column+a.column_count;}
void fill(gui::WidgetDrawing& drawing,double x,double y,double width,double height,gui::WidgetColor color,double view_width,double view_height){
    const auto right=std::min(view_width,x+width),bottom=std::min(view_height,y+height);x=std::max(0.,x);y=std::max(0.,y);
    if(right<=x||bottom<=y||color.alpha<=0)return;
    const auto value=ink(color);drawing.panels.push_back({{float(x),float(y),float(right-x),float(bottom-y)},value,value});
}
void label(gui::WidgetDrawing& drawing,double x,double y,double width,double size,std::string text,gui::WidgetColor color,const std::string& font,double view_width,double view_height,bool elide,unsigned weight=400){
    width=std::min(8192.,width);
    if(text.empty()||width<=0||x>=view_width||x+width<=0||y>=view_height||y+size<=0)return;
    drawing.texts.push_back({.bounds={float(x),float(y),float(width),float(size)},.text=std::move(text),.color=ink(color),.font_size=float(size),.font_weight=weight,.font_family=font,.line_height=float(size),.elide_right=elide,
        .clip=gui::Rect{0,0,float(view_width),float(view_height)}});
}
void icon(gui::WidgetDrawing& drawing,std::string_view name,double x,double y,double size,
          double column_x,double column_width,double view_width,double view_height){
    if(name.empty()||size<=0)return;
    const auto left=std::max({0.,column_x,x}),right=std::min({view_width,column_x+column_width,x+size});
    const auto top=std::max(0.,y),bottom=std::min(view_height,y+size);
    if(right<=left||bottom<=top)return;
    auto image=take(Icons::image(name));
    drawing.images.push_back({.bounds={float(left),float(top),float(right-left),float(bottom-top)},
        .image=std::move(image),.source={float((left-x)/size),float((top-y)/size),float((right-left)/size),float((bottom-top)/size)}});
}
struct ResizeTarget final{bool column=false;std::size_t index=0;friend bool operator==(const ResizeTarget&,const ResizeTarget&)=default;};
}
struct Table::Impl final {
    TableOptions options;TableEvents events;TableViewState value;
    gui::Widget root,header,viewport,body,editor,frame,menu,delete_row_item,delete_column_item;
    gui::WidgetColor primary;
    std::string font;
    std::shared_ptr<TableModel> model;
    std::shared_ptr<TableChangePort> receiver;
    std::optional<TablePosition> anchor,editing;
    std::optional<ResizeTarget> resizing,hover;
    std::vector<TablePosition> visible_positions;
    bool painting=false,pressed=false,resize_pressed=false;
    double resize_start_size=0;
    double view_width=0,view_height=0,total_width=0,total_height=0,heading_height=0,body_height=0;
    ~Impl(){if(resizing){try{(void)model->request_end_resize();}catch(...){}}}
    void sync_model_state(){const auto s=take(model->state());value.model_revision=s.revision;value.resizing_column=s.resizing_column;value.resizing_row=s.resizing_row;value.context_cell=s.context_cell;if(resizing&&!(resizing->column?s.resizing_column==resizing->index:s.resizing_row==resizing->index))resizing.reset();}
    void draw_chrome(){
        gui::WidgetDrawing drawing;const auto width=view_width,height=view_height;
        fill(drawing,0,0,width,options.border_width,options.divider,total_width,total_height);fill(drawing,0,height-options.border_width,width,options.border_width,options.divider,total_width,total_height);
        fill(drawing,0,0,options.border_width,height,options.divider,total_width,total_height);fill(drawing,width-options.border_width,0,options.border_width,height,options.divider,total_width,total_height);
        const auto active=options.resize_handle_hover_color.value_or(gui::WidgetColor{primary.red,primary.green,primary.blue,.4});
        const auto first_column=take(model->column_at_x(value.horizontal_scroll_offset));
        double x=first_column?take(model->column_x(*first_column))-value.horizontal_scroll_offset:width;
        for(std::size_t column=first_column.value_or(model->column_count());column<model->column_count()&&x<width;++column){
            const auto cw=take(model->column_width(column)),right=x+cw;
            if(gutter_height(options)>0)label(drawing,x+(cw-12)/2,height+(total_height-height-12)/2,12,12,"+",options.cell_text,font,total_width,total_height,false);
            if(options.resize_handles_visible)fill(drawing,right-.5,0,1,height,(resizing==ResizeTarget{true,column}||hover==ResizeTarget{true,column})?active:options.resize_handle_color,width,height);
            x=right;
        }
        if(auto first=take(model->row_at_y(value.scroll_offset)))for(auto row=*first;row<model->row_count();++row){
            const auto y=take(model->row_y(row))-value.scroll_offset,rh=take(model->row_height(row));if(y>=body_height)break;
            if(gutter_width(options)>0)label(drawing,width+(total_width-width-12)/2,heading_height+1+y+(rh-12)/2,12,12,"+",options.cell_text,font,total_width,total_height,false);
            const auto line_y=heading_height+1+y+rh;
            if(options.resize_handles_visible&&line_y>=heading_height+1)fill(drawing,0,line_y-.5,width,1,(resizing==ResizeTarget{false,row}||hover==ResizeTarget{false,row})?active:options.resize_handle_color,width,height);
        }
        good(frame.set_drawing(std::move(drawing)));
    }

    void normalize(){
        if(!model->row_count()){value.current.reset();value.selection.reset();anchor.reset();editing.reset();return;}
        auto clamp=[&](TablePosition position){
            position.row=std::min(position.row,model->row_count()-1);
            const auto& cells=model->options().rows[position.row].cells;
            const auto count=cells.empty()?std::max(std::size_t(1),model->header_count()):cells.size();
            position.column=std::min(position.column,count-1);return take(model->merge_anchor(position));
        };
        if(value.current)value.current=clamp(*value.current);
        if(anchor)anchor=clamp(*anchor);
        if(value.selection){auto& range=*value.selection;range.row=std::min(range.row,model->row_count()-1);range.column=std::min(range.column,model->column_count()-1);range.row_count=std::min(range.row_count,model->row_count()-range.row);range.column_count=std::min(range.column_count,model->column_count()-range.column);}
        if(editing){auto cell=model->cell_at(*editing);if(!cell||take(model->merge_anchor(*editing))!=*editing)editing.reset();}
    }
    void refresh(bool configure_viewport=true){
        if(painting)return;
        painting=true;struct Reset final{bool& flag;~Reset(){flag=false;}}reset{painting};
        auto outer=root.snapshot();if(!outer){if(outer.error().code==core::ErrorCode::closed)return;throw BuildError{outer.error()};}
        if(!outer->font_family.empty())font=outer->font_family;
        normalize();
        total_width=std::max(1.,double(outer->bounds.width));total_height=std::max(1.,double(outer->bounds.height));
        const auto width=std::max(1.,total_width-gutter_width(options)),height=std::max(1.,total_height-gutter_height(options));view_width=width;view_height=height;
        const auto header_height=std::min(options.header_height,height-1),body_height=std::max(1.,height-header_height-1);
        heading_height=header_height;this->body_height=body_height;
        good(header.set_frame({0,0,float(width),float(header_height+1)}));
        good(viewport.set_frame({0,float(header_height+1),float(width),float(body_height)}));
        if(configure_viewport)good(viewport.set_scroll_options({.width=width,.height=body_height,.virtual_content_height=model->total_body_height(),.virtual_content_width=model->total_width()}));
        good(body.set_extent({width,body_height}));good(frame.set_frame({0,0,float(total_width),float(total_height)}));
        const auto view=take(viewport.snapshot());value.scroll_offset=view.scroll_offset;value.horizontal_scroll_offset=view.horizontal_scroll_offset;
        gui::WidgetDrawing background,heading,drawing;
        fill(background,0,0,total_width,total_height,options.background,total_width,total_height);
        const auto first_column=take(model->column_at_x(value.horizontal_scroll_offset));
        double x=first_column?take(model->column_x(*first_column))-value.horizontal_scroll_offset:width;
        for(std::size_t column=first_column.value_or(model->column_count());column<model->column_count()&&x<width;++column){
            const auto cell_width=take(model->column_width(column));
            const auto& columns=model->options().columns;
            const auto item=column<columns.size()?columns[column]:TableHeaderColumn{.text=model->options().default_header_text};
            const auto padding=item.padding.value_or(8),available=std::max(0.,cell_width-2*padding);
            const double lead=item.leading_icon.empty()?0:std::min(18.,available);
            const double lead_gap=lead>0?std::min(7.,std::max(0.,available-lead)):0;
            const double tail=item.trailing_icon.empty()?0:std::min(18.,std::max(0.,available-lead-lead_gap));
            const double tail_gap=tail>0?std::min(7.,std::max(0.,available-lead-lead_gap-tail)):0;
            const auto measured=take(gui::measure_text_layout({.text=item.text,.font_size=12,.font_weight=600,.font_family=font,.line_height=12}));
            const auto label_width=std::min(measured.width,std::max(0.,available-lead-lead_gap-tail-tail_gap));
            const auto left=x+padding;
            icon(heading,item.leading_icon,left,(header_height-lead)/2,lead,x,cell_width,width,header_height);
            label(heading,left+lead+lead_gap,(header_height-12)/2,label_width,12,item.text,options.header_text,font,width,header_height,true,600);
            icon(heading,item.trailing_icon,left+lead+lead_gap+label_width+tail_gap,(header_height-tail)/2,tail,x,cell_width,width,header_height);
            x+=cell_width;
        }
        fill(heading,0,header_height,width,1,options.divider,width,header_height+1);
        auto cells=take(model->visible_cells(value.scroll_offset,value.scroll_offset+body_height,value.horizontal_scroll_offset,value.horizontal_scroll_offset+width));
        visible_positions.clear();visible_positions.reserve(cells.size());for(const auto& cell:cells)visible_positions.push_back(cell.position);
        value.visible_cells=cells.size();value.first_visible_row=cells.empty()?0:cells.front().position.row;
        value.first_visible_column=first_column.value_or(0);
        if(cells.size()>4096)throw BuildError{{core::ErrorCode::full,"Table viewport exceeds the renderer's 4096 text-run budget"}};
        auto tint=options.selection_color.value_or(gui::WidgetColor{primary.red,primary.green,primary.blue,primary.alpha*(31./255)});
        const auto current=options.current_color.value_or(primary);
        if(editing){const auto g=take(model->cell_geometry(*editing));const auto left=g.x-value.horizontal_scroll_offset+1+options.content_spacing,y=g.y-value.scroll_offset+(g.height-13)/2;
            if(left<0||left>=width||y<0||y+13>body_height)editing.reset();}
        for(auto& cell:cells){
            const auto& g=cell.geometry;const auto x=g.x-value.horizontal_scroll_offset,y=g.y-value.scroll_offset;
            if(value.selection&&intersects(*value.selection,{cell.position.row,cell.position.column,g.row_span,g.column_span}))fill(drawing,x,y,g.width,g.height,tint,width,body_height);
            fill(drawing,x,y,1,g.height,options.divider,width,body_height);
            if(value.current==cell.position){
                fill(drawing,x,y,g.width,1,current,width,body_height);fill(drawing,x,y+g.height-1,g.width,1,current,width,body_height);
                fill(drawing,x,y,1,g.height,current,width,body_height);fill(drawing,x+g.width-1,y,1,g.height,current,width,body_height);
            }
            if(editing!=cell.position)label(drawing,x+1+options.content_spacing,y+(g.height-13)/2,g.width-1-options.content_spacing,13,std::move(cell.text),options.cell_text,font,width,body_height,!cell.inputable);
        }
        good(root.set_drawing(std::move(background)));good(header.set_drawing(std::move(heading)));good(body.set_drawing(std::move(drawing)));draw_chrome();
        if(editing){
            const auto g=take(model->cell_geometry(*editing));const auto y=g.y-value.scroll_offset+(g.height-13)/2;
            const auto left=g.x-value.horizontal_scroll_offset+1+options.content_spacing,editor_width=std::min({8192.,g.width-1-options.content_spacing,width-left});
            if(editor_width>0&&left>=0&&left<width&&y>=0&&y+13<=body_height){
                good(editor.set_input_options({.text=take(model->cell_text(*editing)),.width=editor_width,.commit_on_focus_loss=false,
                    .style=gui::InputStyle{.height=13,.font_size=13,.line_height=13,.horizontal_padding=0,.corner_radius=0,.background=gui::WidgetColor{0,0,0,0},.text=options.cell_text,.focus_ring=false}}));
                good(editor.set_frame({float(left),float(y),float(editor_width),13}));good(editor.set_visible(true));
            }else{editing.reset();good(editor.set_visible(false));}
        }else good(editor.set_visible(false));
        value.editing=editing.has_value();sync_model_state();
    }
    bool select(TablePosition position,bool extend){
        if(!options.selection_enabled||options.selection_mode==TableSelectionMode::none)return false;
        take(model->cell_at(position));position=take(model->merge_anchor(position));
        const auto g=take(model->cell_geometry(position));
        if(!extend||!anchor||options.selection_mode==TableSelectionMode::single_cell)anchor=position;
        const auto a=take(model->cell_geometry(*anchor));
        const auto row=std::min(anchor->row,position.row),column=std::min(anchor->column,position.column);
        TableRange range{row,column,std::max(anchor->row+a.row_span,position.row+g.row_span)-row,std::max(anchor->column+a.column_span,position.column+g.column_span)-column};
        const bool changed=value.current!=position||value.selection!=range;
        editing.reset();value.current=position;value.selection=range;refresh();
        auto selection=events.selection_changed;auto activated=events.activated;
        if(changed&&selection)selection(value.selection);
        if(root.snapshot()&&activated)activated(position.row,position.column);
        return changed;
    }
    void finish(bool submit){
        auto position=editing;editing.reset();refresh();
        if(root.snapshot())good(root.focus());
        auto callback=events.submitted;
        if(submit&&position&&callback)callback(position->row,position->column,take(model->cell_text(*position)));
    }
    void edit_at(TablePosition position){
        if(!take(model->cell_inputable(position)))throw BuildError{{core::ErrorCode::invalid_argument,"Current table cell is not editable"}};
        const auto g=take(model->cell_geometry(position));
        const auto bounds=take(viewport.snapshot()).bounds;
        if(g.width<=1+options.content_spacing)throw BuildError{{core::ErrorCode::invalid_argument,"Cell padding leaves no editor space"}};
        double x=value.horizontal_scroll_offset;
        if(g.width<=bounds.width){if(g.x<x)x=g.x;else if(g.x+g.width>x+bounds.width)x=g.x+g.width-bounds.width;}
        else x=g.x;
        const auto text_left=g.x+1+options.content_spacing;if(text_left>=x+bounds.width)x=text_left-bounds.width+std::min(32.,double(bounds.width));
        double offset=value.scroll_offset;const auto center=g.y+g.height/2;
        if(center-6.5<offset||center+6.5>offset+bounds.height)offset=std::max(0.,center-bounds.height/2);
        good(viewport.set_scroll_position(x,offset));editing=position;refresh();good(editor.focus());
    }
    void reveal(TablePosition position){
        take(model->cell_at(position));const auto g=take(model->cell_geometry(take(model->merge_anchor(position))));const auto bounds=take(viewport.snapshot()).bounds;
        auto x=value.horizontal_scroll_offset,y=value.scroll_offset;
        if(g.width<=bounds.width){if(g.x<x)x=g.x;else if(g.x+g.width>x+bounds.width)x=g.x+g.width-bounds.width;}
        else if(g.x>=x+bounds.width||g.x+g.width<=x)x=g.x;
        if(g.height<=bounds.height){if(g.y<y)y=g.y;else if(g.y+g.height>y+bounds.height)y=g.y+g.height-bounds.height;}
        else if(g.y>=y+bounds.height||g.y+g.height<=y)y=g.y;
        good(viewport.set_scroll_position(x,y));
    }
    void edit(){if(!value.current)throw BuildError{{core::ErrorCode::invalid_argument,"Select a table cell before editing"}};edit_at(*value.current);}
    void edited(std::string input){
        if(!editing)return;const auto position=*editing;
        auto validated=take(model->validate_cell_input(position,TableValue{input}));
        if(!validated.accepted){
            good(editor.set_text(take(model->cell_text(position))));auto callback=events.rejected;
            if(callback)callback(position.row,position.column,std::move(input),take(model->column_type(position.column)));return;
        }
        take(model->set_cell_value(position,validated.value));good(editor.set_text(validated.text));auto callback=events.edited;
        if(callback)callback(position.row,position.column,std::move(validated.text));
    }
    bool sort(std::size_t column,TableSortOrder order){
        if(column>=model->column_count()||(order!=TableSortOrder::ascending&&order!=TableSortOrder::descending))throw BuildError{{core::ErrorCode::invalid_argument,"Invalid table sort column/order"}};
        if(editing)finish(false);const bool changed=take(model->sort_rows(column,order));value.sort_column=column;value.sort_order=order;refresh();
        auto callback=events.sorted;if(callback)callback(column,order);return changed;
    }
    void bind_model(const std::shared_ptr<Impl>& self,std::shared_ptr<TableModel> next){
        take(next->state());std::weak_ptr<Impl> weak=self;std::weak_ptr<TableModel> expected=next;
        auto port=std::make_shared<TableChangePort>([weak,expected](const TableChange& change){if(auto p=weak.lock();p&&p->model==expected.lock()){
            if(change.reset||change.structure)p->editing.reset();const auto& range=change.affected;
            const bool visible=change.cells&&std::ranges::any_of(p->visible_positions,[&](TablePosition v){return v.row>=range.row&&v.row-range.row<range.row_count&&v.column>=range.column&&v.column-range.column<range.column_count;});
            try{if(change.reset||change.geometry||change.structure||change.headers||visible)p->refresh();else p->sync_model_state();}
            catch(const BuildError& error){throw std::runtime_error(error.error.message);}
        }});
        good(next->observe(port));model=std::move(next);receiver=std::move(port);
    }
    bool select_range(TablePosition first,TablePosition last){
        if(!options.selection_enabled||options.selection_mode==TableSelectionMode::none)return false;
        take(model->cell_at(first));take(model->cell_at(last));first=take(model->merge_anchor(first));last=take(model->merge_anchor(last));
        if(options.selection_mode==TableSelectionMode::single_cell)return select(last,false);
        const auto g=take(model->cell_geometry(last));const auto end_row=last.row+g.row_span-1,end_column=last.column+g.column_span-1;
        const auto row=std::min(first.row,end_row),column=std::min(first.column,end_column);
        const TableRange range{row,column,std::max(first.row,end_row)-row+1,std::max(first.column,end_column)-column+1};
        const bool changed=value.current!=last||value.selection!=range;anchor=first;value.current=last;value.selection=range;editing.reset();refresh();
        auto callback=events.selection_changed;if(changed&&callback)callback(value.selection);return changed;
    }
    bool set_values(TablePosition first,const TableInputMatrix& matrix){
        const bool changed=take(model->set_range_values(first,matrix));if(!changed)return false;
        std::size_t columns=0;for(const auto& row:matrix)columns=std::max(columns,row.size());
        const TableRange range{first.row,first.column,matrix.size(),columns};auto callback=events.range_values_changed;if(callback)callback(range);
        const TablePosition last{first.row+matrix.size()-1,first.column+columns-1};
        if(root.snapshot()&&options.selection_enabled&&model->cell_at(first)&&model->cell_at(last))select_range(first,last);return true;
    }
    std::string selection_tsv(std::size_t byte_limit=TableLimits::transfer_bytes)const{return value.selection?take(table_range_as_tsv(*model,*value.selection,byte_limit)):std::string{};}
    void copy()const{
        auto text=selection_tsv(1024*1024);
        good(gui::write_clipboard({.text=std::move(text)}));
    }
    bool paste(std::string_view text){return value.selection?set_values({value.selection->row,value.selection->column},take(parse_table_tsv(text))):false;}
    std::optional<ResizeTarget> resize_at(double x,double y)const{
        if(!options.resize_handles_visible||x<0||y<0||x>=view_width||y>=view_height)return {};
        if(y>=heading_height+1&&options.row_resize_handle_height>0){
            const auto logical=y-heading_height-1+value.scroll_offset,half=options.row_resize_handle_height/2;auto row=take(model->row_at_y(std::max(0.,logical-half)));
            if(row&&std::abs(take(model->row_y(*row))+take(model->row_height(*row))-logical)<=half)return ResizeTarget{false,*row};
        }
        if(options.column_resize_handle_width>0){const auto logical=x+value.horizontal_scroll_offset,half=options.column_resize_handle_width/2;auto column=take(model->column_at_x(std::max(0.,logical-half)));
            if(column&&std::abs(take(model->column_x(*column))+take(model->column_width(*column))-logical)<=half)return ResizeTarget{true,*column};}
        return {};
    }
    void end_resize(bool notify){
        const auto previous=std::exchange(resizing,{});if(!previous)return;
        auto resized_model=model;const auto start_size=resize_start_size;
        take(resized_model->request_end_resize());sync_model_state();if(model!=resized_model||!root.snapshot())return;draw_chrome();
        auto callback=previous->column?events.column_resized:events.row_resized;
        const auto size=previous->column?take(model->column_width(previous->index)):take(model->row_height(previous->index));
        if(notify&&callback&&size!=start_size)callback(previous->index,size);
    }
    void begin_resize(ResizeTarget target,double pointer){
        const auto state=take(model->state());if(state.resizing_column||state.resizing_row)throw BuildError{{core::ErrorCode::busy,"The shared model already has a resize owner"}};
        if(editing)finish(false);resizing=target;resize_start_size=target.column?take(model->column_width(target.index)):take(model->row_height(target.index));
        auto started=target.column?model->begin_column_resize(target.index,pointer):model->begin_row_resize(target.index,pointer);
        if(!started){const auto actual=take(model->state());if(!(target.column?actual.resizing_column==target.index:actual.resizing_row==target.index))resizing.reset();throw BuildError{started.error()};}
        if(root.snapshot()){good(root.focus());sync_model_state();draw_chrome();}else end_resize(false);
    }
    void context(TablePosition position,double x,double y){
        if(!options.delete_context_menu_enabled)return;
        if(resizing)end_resize(false);if(editing)finish(false);
        take(model->set_context_cell(position));if(options.select_on_context_click)select(position,false);if(!root.snapshot())return;
        good(delete_row_item.set_enabled(model->row_count()>0));good(delete_column_item.set_enabled(model->column_count()>1));good(menu.open_at(x,y));
    }
    void context_action(TableContextAction action){const auto state=take(model->state());if(state.context_cell)take(model->trigger_context_action(action,*state.context_cell));}
    core::Result<bool> input(const gui::WidgetInput& event){
        try{
            if(event.kind==gui::WidgetInputKind::focus_lost){pressed=false;resize_pressed=false;if(resizing)end_resize(false);hover.reset();editing.reset();if(root.snapshot())refresh();return false;}
            if(event.kind==gui::WidgetInputKind::scroll&&resizing)return true;
            const auto outer=take(root.snapshot()).bounds;const auto x=event.x-outer.x,y=event.y-outer.y;
            if(event.kind==gui::WidgetInputKind::pointer_leave){if(!resizing&&hover){hover.reset();draw_chrome();}return false;}
            if(event.kind==gui::WidgetInputKind::pointer_move){
                if(resizing){take(resizing->column?model->update_column_resize(x):model->update_row_resize(y));return true;}
                const auto next=resize_at(x,y);if(next!=hover){hover=next;draw_chrome();}return false;
            }
            if(event.kind==gui::WidgetInputKind::context_request){
                if(!options.delete_context_menu_enabled)return false;
                if(x>=0&&x<view_width&&y>=heading_height+1&&y<view_height)if(auto position=take(model->position_at(x+value.horizontal_scroll_offset,y-heading_height-1+value.scroll_offset)))context(*position,event.x,event.y);return true;
            }
            if(event.kind==gui::WidgetInputKind::text&&event.content&&!event.text_input_active){if(options.clipboard_enabled)paste(event.content->text);return options.clipboard_enabled;}
            if(event.kind==gui::WidgetInputKind::pointer_down){
                if(auto target=resize_at(x,y)){pressed=true;resize_pressed=true;begin_resize(*target,target->column?x:y);return true;}
                if(editing&&contains(take(editor.snapshot()).bounds,event.x,event.y))return false;
                pressed=true;resize_pressed=false;if(editing)finish(false);good(root.focus());return true;
            }
            if(event.kind==gui::WidgetInputKind::pointer_up){
                if(!pressed)return false;pressed=false;
                if(resizing){resize_pressed=false;end_resize(true);return true;}
                if(std::exchange(resize_pressed,false))return true;
                if(editing&&contains(take(editor.snapshot()).bounds,event.x,event.y))return false;
                if(gutter_width(options)>0&&x>=view_width&&x<total_width&&y>=heading_height+1&&y<view_height){if(auto row=take(model->row_at_y(y-heading_height-1+value.scroll_offset)))take(model->insert_row(*row+1));return true;}
                if(gutter_height(options)>0&&y>=view_height&&y<total_height&&x>=0&&x<view_width){if(auto column=take(model->column_at_x(x+value.horizontal_scroll_offset)))take(model->insert_column(*column+1));return true;}
                const auto head=take(header.snapshot()).bounds,view=take(viewport.snapshot()).bounds;
                if(contains(head,event.x,event.y)){
                    if(options.sorting_enabled)if(auto column=take(model->column_at_x(event.x-head.x+value.horizontal_scroll_offset)))sort(*column,value.sort_column==column&&value.sort_order==TableSortOrder::ascending?TableSortOrder::descending:TableSortOrder::ascending);
                    return true;
                }
                if(contains(view,event.x,event.y))if(auto position=take(model->position_at(event.x-view.x+value.horizontal_scroll_offset,event.y-view.y+value.scroll_offset))){
                    select(*position,event.shift);if(!root.snapshot())return true;
                    if(!event.shift&&take(model->cell_inputable(*position)))edit_at(*position);return true;
                }
                return true;
            }
            if(event.kind!=gui::WidgetInputKind::key_down)return false;
            if(resizing){if(event.key==gui::WidgetKey::escape)end_resize(true);return true;}
            if(editing){if(event.key==gui::WidgetKey::escape){finish(false);return true;}return false;}
            if(options.clipboard_enabled&&event.key==gui::WidgetKey::copy){if(value.selection)copy();return true;}
            if(!options.keyboard_navigation||!options.selection_enabled||options.selection_mode==TableSelectionMode::none)return false;
            if(event.key==gui::WidgetKey::enter&&value.current&&take(model->cell_inputable(*value.current))){edit();return true;}
            if(event.key==gui::WidgetKey::escape){const bool changed=value.selection.has_value();value.selection.reset();value.current.reset();anchor.reset();refresh();if(changed&&events.selection_changed)events.selection_changed({});return changed;}
            if(!model->row_count())return false;
            auto position=value.current.value_or(TablePosition{});
            switch(event.key){
            case gui::WidgetKey::left:if(position.column)--position.column;break;
            case gui::WidgetKey::right:position.column=std::min(position.column+1,model->column_count()-1);break;
            case gui::WidgetKey::up:if(position.row)--position.row;break;
            case gui::WidgetKey::down:position.row=std::min(position.row+1,model->row_count()-1);break;
            case gui::WidgetKey::home:position.column=0;break;
            case gui::WidgetKey::end:position.column=model->column_count()-1;break;
            case gui::WidgetKey::select_all:{select_range({0,0},{model->row_count()-1,model->column_count()-1});return true;}
            default:return false;
            }
            const auto& cells=model->options().rows[position.row].cells;const auto count=cells.empty()?std::max(std::size_t(1),model->header_count()):cells.size();position.column=std::min(position.column,count-1);
            select(position,event.shift);
            // Selection callbacks may clear the current cell or retire this
            // mount. Resolve the current state again before revealing it.
            if(!root.snapshot()||!value.current)return true;
            reveal(*value.current);return true;
        }catch(const BuildError& error){return std::unexpected(error.error);}
    }
};

core::Result<Table> Table::create(gui::WidgetBuilder& builder,gui::Widget parent,std::string_view id,TableOptions options,TableEvents events){
    if(!valid(options))return core::fail(core::ErrorCode::invalid_argument,"Invalid native Table options");
    try{
        auto state=std::make_shared<Impl>();state->options=options;state->events=std::move(events);state->model=options.model?options.model:std::make_shared<TableModel>();state->options.model=state->model;
        take(state->model->state());if(options.fit_columns_to_viewport)take(state->model->set_table_width(options.width-gutter_width(options)));
        const auto appearance=take(builder.appearance());state->primary=appearance.primary;state->font=appearance.font_family;const auto prefix=std::string(id);
        state->root=take(builder.absolute_stack(parent,id,{options.width,options.height}));good(state->root.set_clip_content(true));
        state->header=take(builder.absolute_stack(state->root,prefix+".header",{options.width,options.header_height+1}));
        state->viewport=take(builder.scroll_view(state->root,prefix+".viewport",{.width=options.width,.height=options.height-options.header_height-1,.virtual_content_height=state->model->total_body_height(),.virtual_content_width=state->model->total_width()}));
        state->body=take(builder.absolute_stack(state->viewport,prefix+".body",{options.width,options.height-options.header_height-1}));
        std::weak_ptr<Impl> weak=state;
        state->editor=take(builder.input_field(state->body,prefix+".editor",{.commit_on_focus_loss=false},[weak](std::string input){if(auto state=weak.lock())state->edited(std::move(input));},[weak]{if(auto state=weak.lock())state->finish(true);}));
        good(state->editor.set_visible(false));state->frame=take(builder.absolute_stack(state->root,prefix+".frame",{options.width,options.height}));
        state->menu=take(builder.menu(state->root,prefix+".context",{.style=gui::MenuStyle::context,.item_width=160}));
        state->delete_row_item=take(builder.menu_item(state->menu,prefix+".delete-row",{.style=gui::MenuStyle::context,.label="Delete row",.show_icon_slot=false,.show_chevron=false,.has_child_items=false},[weak]{if(auto p=weak.lock())p->context_action(TableContextAction::delete_row);}));
        state->delete_column_item=take(builder.menu_item(state->menu,prefix+".delete-column",{.style=gui::MenuStyle::context,.label="Delete column",.show_icon_slot=false,.show_chevron=false,.has_child_items=false},[weak]{if(auto p=weak.lock())p->context_action(TableContextAction::delete_column);}));
        if(options.delete_context_menu_enabled)good(state->root.set_context_menu(state->menu));
        good(state->root.set_input_handler([weak](const auto& input)->core::Result<bool>{if(auto state=weak.lock())return state->input(input);return false;}));
        good(state->viewport.set_scroll_handler([weak](const auto&){if(auto state=weak.lock())state->refresh(false);}));
        state->bind_model(state,state->model);state->refresh();good(builder.retain(state));
        Table table;table.root_=state->root;table.viewport_=state->viewport;table.body_=state->body;table.editor_=state->editor;table.menu_=state->menu;table.state_=state;return table;
    }catch(const BuildError& error){return std::unexpected(error.error);}
}
template<class T,class F>core::Result<T> Table::with_state(F&& action)const{
    if(auto ready=root_.snapshot();!ready)return std::unexpected(ready.error());
    auto state=state_.lock();if(!state)return core::fail(core::ErrorCode::closed,"Table closed");
    try{if constexpr(std::is_void_v<T>){action(*state);return {};}else return action(*state);}
    catch(const BuildError& error){return std::unexpected(error.error);}
    catch(...){return core::fail(core::ErrorCode::failed,"Table callback failed after any committed change",std::current_exception());}
}
core::Result<TableViewState> Table::snapshot()const{return with_state<TableViewState>([](Impl& p){return p.value;});}
core::Result<void> Table::set_options(TableOptions options)const{return with_state<void>([&](Impl& p){
    if(!valid(options))throw BuildError{{core::ErrorCode::invalid_argument,"Invalid native Table options"}};
    auto next=options.model?options.model:p.model;take(next->state());if(p.resizing)p.end_resize(false);
    if(options.fit_columns_to_viewport)take(next->set_table_width(options.width-gutter_width(options)));
    if(next!=p.model){p.editing.reset();p.bind_model(state_.lock(),std::move(next));}
    options.model=p.model;p.options=std::move(options);
    if(!p.options.selection_enabled||p.options.selection_mode==TableSelectionMode::none){p.value.current.reset();p.value.selection.reset();p.anchor.reset();}
    if(!p.options.delete_context_menu_enabled)good(p.menu.close_popup());
    good(p.root.set_context_menu(p.options.delete_context_menu_enabled?p.menu:gui::Widget{}));
    good(p.root.set_extent({p.options.width,p.options.height}));p.refresh();
});}
core::Result<void> Table::set_scroll_offset(double value)const{return with_state<void>([&](Impl& p){good(p.viewport.set_scroll_offset(value));});}
core::Result<void> Table::set_horizontal_scroll_offset(double value)const{return with_state<void>([&](Impl& p){good(p.viewport.set_horizontal_scroll_offset(value));});}
core::Result<void> Table::set_scroll_position(double x,double y)const{return with_state<void>([&](Impl& p){good(p.viewport.set_scroll_position(x,y));});}
core::Result<void> Table::reveal_cell(TablePosition position)const{return with_state<void>([&](Impl& p){p.reveal(position);});}
core::Result<bool> Table::select_cell(TablePosition pos,bool extend)const{return with_state<bool>([&](Impl& p){return p.select(pos,extend);});}
core::Result<bool> Table::select_range(TablePosition first,TablePosition last)const{return with_state<bool>([&](Impl& p){return p.select_range(first,last);});}
core::Result<bool> Table::select_row(std::size_t row)const{return with_state<bool>([&](Impl& p){return p.select_range({row,0},{row,p.model->column_count()-1});});}
core::Result<bool> Table::select_column(std::size_t col)const{return with_state<bool>([&](Impl& p){return p.model->row_count()?p.select_range({0,col},{p.model->row_count()-1,col}):false;});}
core::Result<bool> Table::select_all()const{return with_state<bool>([](Impl& p){return p.model->row_count()?p.select_range({0,0},{p.model->row_count()-1,p.model->column_count()-1}):false;});}
core::Result<bool> Table::is_cell_selected(TablePosition pos)const{return with_state<bool>([&](Impl& p){const auto anchor=take(p.model->merge_anchor(pos));const auto g=take(p.model->cell_geometry(anchor));return p.value.selection&&intersects(*p.value.selection,{anchor.row,anchor.column,g.row_span,g.column_span});});}
core::Result<bool> Table::clear_selection()const{return with_state<bool>([](Impl& p){const bool changed=p.value.selection.has_value();p.value.selection.reset();p.value.current.reset();p.anchor.reset();p.editing.reset();p.refresh();if(changed&&p.events.selection_changed)p.events.selection_changed({});return changed;});}
core::Result<TableMatrix> Table::selection_values()const{return with_state<TableMatrix>([](Impl& p){return p.value.selection?take(p.model->range_values(*p.value.selection)):TableMatrix{};});}
core::Result<std::string> Table::selection_as_tsv()const{return with_state<std::string>([](Impl& p){return p.selection_tsv();});}
core::Result<bool> Table::set_range_values(TablePosition pos,const TableInputMatrix& data)const{return with_state<bool>([&](Impl& p){return p.set_values(pos,data);});}
core::Result<bool> Table::paste_tsv(TablePosition pos,std::string_view text)const{return with_state<bool>([&](Impl& p){return p.set_values(pos,take(parse_table_tsv(text)));});}
core::Result<bool> Table::paste_selection_tsv(std::string_view text)const{return with_state<bool>([&](Impl& p){return p.paste(text);});}
core::Result<void> Table::copy_selection()const{return with_state<void>([](Impl& p){if(p.value.selection)p.copy();});}
core::Result<bool> Table::paste_from_clipboard()const{return with_state<bool>([](Impl& p){return p.value.selection?p.paste(take(gui::read_clipboard()).text):false;});}
core::Result<void> Table::begin_edit()const{return with_state<void>([](Impl& p){if(!p.value.current)throw BuildError{{core::ErrorCode::invalid_argument,"Select a table cell before editing"}};p.edit_at(*p.value.current);});}
core::Result<void> Table::end_edit()const{return with_state<void>([](Impl& p){p.finish(false);});}
core::Result<bool> Table::sort_rows(std::size_t column,TableSortOrder order)const{return with_state<bool>([&](Impl& p){return p.sort(column,order);});}
core::Result<bool> Table::merge_selection()const{return with_state<bool>([](Impl& p){return p.value.selection?take(p.model->merge_cells(*p.value.selection)):false;});}
core::Result<bool> Table::split_cell(TablePosition pos)const{return with_state<bool>([&](Impl& p){return take(p.model->split_cell(pos));});}
core::Result<bool> Table::insert_row(std::size_t index)const{return with_state<bool>([&](Impl& p){return take(p.model->insert_row(index));});}
core::Result<bool> Table::delete_row(std::size_t index)const{return with_state<bool>([&](Impl& p){return take(p.model->delete_row(index));});}
core::Result<bool> Table::insert_column(std::size_t index)const{return with_state<bool>([&](Impl& p){return take(p.model->insert_column(index));});}
core::Result<bool> Table::delete_column(std::size_t index)const{return with_state<bool>([&](Impl& p){return take(p.model->delete_column(index));});}
core::Result<bool> Table::undo()const{return with_state<bool>([](Impl& p){return take(p.model->undo());});}
core::Result<bool> Table::redo()const{return with_state<bool>([](Impl& p){return take(p.model->redo());});}
core::Result<void> Table::open_context_menu(TablePosition pos,double x,double y)const{return with_state<void>([&](Impl& p){p.context(pos,x,y);});}
}
