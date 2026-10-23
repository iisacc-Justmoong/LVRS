#include "TableRow.h"
#include "TableDetail.h"
namespace lvrs {
namespace {
bool valid_row(const TableRowOptions& o){using namespace table_detail;return metric(o.width,1)&&metric(o.cell_width,1)&&metric(o.cell_height,1)&&metric(o.content_spacing)&&color(o.divider_color)&&color(o.text_color)&&!o.cells.empty()&&o.cells.size()<=512;}
}
struct TableRow::Impl final {
    TableRowOptions options;TableRowEvents events;ii::gui::Widget root;std::vector<TableCellItem> cells;
    TableCellOptions resolve(std::size_t i,const TableRowOptions& o)const{auto c=o.cells[i];c.width=o.cell_width;c.cell_height=o.cell_height;c.content_spacing=o.content_spacing;c.divider_color=o.divider_color;c.text_color=o.text_color;c.inputable=c.inputable.value_or(o.inputable);c.enabled=c.enabled&&o.enabled;return c;}
    ii::core::Result<void> apply(TableRowOptions o){
        using namespace ii;using namespace table_detail;
        if(!valid_row(o)||o.cells.size()!=cells.size())return core::fail(core::ErrorCode::invalid_argument,"TableRow replacement requires valid same-count cells");
        const double spacing=cells.size()>1?std::max(0.,(o.width-cells.size()*o.cell_width)/(cells.size()-1)):0.;
        for(std::size_t i=0;i<cells.size();++i)if(!valid_cell(resolve(i,o)))return core::fail(core::ErrorCode::invalid_argument,"Invalid TableRow cell data");
        if(o.cell_width*cells.size()+spacing*(cells.size()-1)>32767)return core::fail(core::ErrorCode::out_of_range,"TableRow extent exceeds native coordinates");
        try{good(root.set_extent({o.width,o.cell_height}));for(std::size_t i=0;i<cells.size();++i){good(cells[i].set_options(resolve(i,o)));good(cells[i].root().set_frame({float(i*(o.cell_width+spacing)),0,float(o.cell_width),float(o.cell_height)}));}options=std::move(o);return {};}catch(const BuildError& e){return std::unexpected(e.error);}
    }
};
ii::core::Result<TableRow> TableRow::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,std::string_view id,TableRowOptions o,TableRowEvents events){
    using namespace ii;using namespace table_detail;if(!valid_row(o))return core::fail(core::ErrorCode::invalid_argument,"Invalid TableRow options");
    try {
        auto s=std::make_shared<Impl>();s->options=o;s->events=std::move(events);s->root=take(b.absolute_stack(parent,id,{o.width,o.cell_height}));std::weak_ptr<Impl> weak=s;
        for(std::size_t i=0;i<o.cells.size();++i)s->cells.push_back(take(TableCellItem::create(b,s->root,std::string(id)+".cell"+std::to_string(i),s->resolve(i,o),{.edited=[weak,i](std::string text){if(auto p=weak.lock()){auto callback=p->events.edited;if(callback)callback(i,std::move(text));}},.submitted=[weak,i](std::string text){if(auto p=weak.lock()){auto callback=p->events.submitted;if(callback)callback(i,std::move(text));}}})));
        good(s->apply(std::move(o)));good(b.retain(s));TableRow row;row.root_=s->root;row.state_=s;return row;
    }catch(const BuildError& e){return std::unexpected(e.error);}
}
ii::core::Result<TableCellItem> TableRow::cell(std::size_t i)const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Row closed");if(i>=s->cells.size())return ii::core::fail(ii::core::ErrorCode::out_of_range,"TableRow cell out of range");return s->cells[i];}
ii::core::Result<void> TableRow::set_options(TableRowOptions o)const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Row closed");return s->apply(std::move(o));}
}
