#include "TableHeader.h"
#include "TableDetail.h"
namespace lvrs {
struct TableHeader::Impl final {
    TableHeaderOptions options;TableHeaderModel model;TableHeaderEvents events;ii::gui::Widget root,separator;std::vector<ii::gui::Widget> buttons,labels;
    ii::core::Result<void> apply(TableHeaderOptions o){
        using namespace ii;using namespace table_detail;TableHeaderModel next;auto validation=next.set_options(o.layout);if(!validation)return validation;
        if(next.descriptors().size()!=buttons.size()||!metric(o.separator_height)||!color(o.text_color)||!color(o.separator_color))return core::fail(core::ErrorCode::invalid_argument,"TableHeader replacement requires valid same-count columns");
        try{
            good(root.set_extent({o.layout.width,o.layout.row_height+o.separator_height}));good(separator.set_image_options(ink(std::max(1.,o.layout.width),std::max(1.,o.separator_height),o.separator_color)));good(separator.set_frame({0,float(o.layout.row_height),float(o.layout.width),float(o.separator_height)}));good(separator.set_visible(o.separator_height>0));
            for(const auto& d:next.descriptors()){
                const auto i=d.index;good(buttons[i].set_frame({float(d.x),0,float(d.width),float(d.height)}));good(buttons[i].set_accessibility_name(d.source.text));good(buttons[i].set_enabled(o.enabled&&o.interactive));
                good(labels[i].set_label_options({.text=d.source.text,.font_size=12,.role=gui::TextRole::description,.color=o.text_color,.line_height=12,.elide_right=true}));
                good(labels[i].set_frame({float(d.x+d.padding),float((d.height-12)/2),float(std::max(0.,d.width-d.padding*2)),12}));
            }
            good(model.set_options(o.layout));options=std::move(o);return {};
        }catch(const BuildError& e){return std::unexpected(e.error);}
    }
};
ii::core::Result<TableHeader> TableHeader::create(ii::gui::WidgetBuilder& b,ii::gui::Widget parent,std::string_view id,TableHeaderOptions o,TableHeaderEvents events){
    using namespace ii;using namespace table_detail;TableHeaderModel model;if(auto r=model.set_options(o.layout);!r)return std::unexpected(r.error());
    if(!metric(o.separator_height)||!color(o.text_color)||!color(o.separator_color))return core::fail(core::ErrorCode::invalid_argument,"Invalid TableHeader style");
    try {
        auto s=std::make_shared<Impl>();s->events=std::move(events);s->root=take(b.absolute_stack(parent,id,{o.layout.width,o.layout.row_height+o.separator_height}));good(s->root.set_clip_content(true));std::weak_ptr<Impl> weak=s;
        for(const auto& d:model.descriptors()){
            auto button=take(b.button(s->root,std::string(id)+".column"+std::to_string(d.index),{.text="",.tone=gui::ButtonTone::borderless,.accessibility_name=d.source.text,.corner_radius=0,.style=gui::ButtonStyle{.fill=gui::WidgetColor{0,0,0,0},.hover_fill=gui::WidgetColor{0,0,0,0},.pressed_fill=gui::WidgetColor{0,0,0,0},.disabled_fill=gui::WidgetColor{0,0,0,0}}},[weak,i=d.index]{if(auto p=weak.lock()){auto data=p->model.descriptor_at(i);if(!data)return;auto callback=p->events.column_clicked;if(callback)callback(i,data->source);}}));s->buttons.push_back(button);s->labels.push_back(take(b.label(s->root,std::string(id)+".label"+std::to_string(d.index))));
        }
        s->separator=take(b.image(s->root,std::string(id)+".separator",ink(std::max(1.,o.layout.width),1,o.separator_color)));good(s->apply(std::move(o)));good(b.retain(s));TableHeader header;header.root_=s->root;header.state_=s;return header;
    }catch(const BuildError& e){return std::unexpected(e.error);}
}
ii::core::Result<ii::gui::Widget> TableHeader::column(std::size_t i)const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Header closed");if(i>=s->buttons.size())return ii::core::fail(ii::core::ErrorCode::out_of_range,"TableHeader column out of range");return s->buttons[i];}
ii::core::Result<void> TableHeader::set_options(TableHeaderOptions o)const{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto s=state_.lock();if(!s)return ii::core::fail(ii::core::ErrorCode::closed,"Header closed");return s->apply(std::move(o));}
}
