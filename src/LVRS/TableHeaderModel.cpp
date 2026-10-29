#include "TableHeaderModel.h"
#include "TableDetail.h"
#include "Icons.h"
namespace lvrs {
TableHeaderModel::TableHeaderModel(){(void)set_options(options_);}
ii::core::Result<void> TableHeaderModel::set_options(TableHeaderModelOptions o){
    using namespace table_detail;
    if(!metric(o.width)||!metric(o.row_height,1)||!metric(o.horizontal_padding)||!metric(o.fallback_cell_width)||!metric(o.minimum_column_width,1)||o.columns.size()>512||o.column_widths.size()>512)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid TableHeaderModel geometry/count");
    for(auto w:o.column_widths)if(!std::isfinite(w)||std::abs(w)>8192)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid TableHeaderModel column width");
    for(const auto& c:o.columns)if(!text(c.text)||!text(c.value_type)||!text(c.leading_icon)||!text(c.trailing_icon)||(c.padding&&!metric(*c.padding)))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Invalid TableHeaderModel column data");
    for(const auto& c:o.columns)if((!c.leading_icon.empty()&&!Icons::data(c.leading_icon))||(!c.trailing_icon.empty()&&!Icons::data(c.trailing_icon)))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Unknown table header icon");
    if(revision_&&o==options_)return {};
    std::vector<TableHeaderDescriptor> next;next.reserve(std::max(std::size_t(1),o.columns.size()));
    const auto automatic=std::max(o.minimum_column_width,std::floor(o.width/std::max(std::size_t(1),o.columns.size())));
    const auto fallback=o.fallback_cell_width>0?std::max(o.minimum_column_width,std::round(o.fallback_cell_width)):automatic;
    double x=0;
    for(std::size_t i=0;i<std::max(std::size_t(1),o.columns.size());++i){
        const auto width=i<o.column_widths.size()&&o.column_widths[i]>0?std::max(o.minimum_column_width,std::round(o.column_widths[i])):fallback;
        if(x+width>32767)return ii::core::fail(ii::core::ErrorCode::out_of_range,"Table header extent exceeds native coordinates");
        const auto c=i<o.columns.size()?o.columns[i]:TableHeaderColumn{};next.push_back({i,c,x,width,o.row_height,c.padding.value_or(o.horizontal_padding)});x+=width;
    }
    options_=std::move(o);descriptors_=std::move(next);++revision_;return {};
}
ii::core::Result<TableHeaderDescriptor> TableHeaderModel::descriptor_at(std::size_t i)const{if(i>=descriptors_.size())return ii::core::fail(ii::core::ErrorCode::out_of_range,"Table header index out of range");return descriptors_[i];}
}
