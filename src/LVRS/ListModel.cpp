#include "ListModel.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <functional>
namespace lvrs {
using namespace ii;
core::Result<void> ListModel::ready()const{if(owner_!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"ListModel requires its owner thread");return {};}
core::Result<void> ListModel::validate(const std::vector<ListRowData>& rows)const{
    if(rows.size()>100000)return core::fail(core::ErrorCode::out_of_range,"ListModel row limit is 100000");
    std::map<std::string,std::size_t,std::less<>> ids;
    const auto text=[](std::string_view value){return value.size()<=65536&&core::valid_utf8(value)&&value.find('\0')==value.npos;};
    for(std::size_t i=0;i<rows.size();++i){const auto& row=rows[i];if(row.key.empty()||row.key.size()>1024||!text(row.key)||!text(row.text)||!text(row.parent)||!ids.emplace(row.key,i).second)return core::fail(core::ErrorCode::invalid_argument,"Invalid or duplicate ListModel row key/text");}
    for(const auto& row:rows){auto parent=row.parent;unsigned depth=0;while(!parent.empty()){
        const auto found=ids.find(parent);if(found==ids.end()||parent==row.key||++depth>64)return core::fail(core::ErrorCode::invalid_argument,"Invalid, cyclic or too deep list hierarchy");parent=rows[found->second].parent;}}
    return {};
}
core::Result<bool> ListModel::changed(){try{changes_.set(changes_.get()+1);return true;}catch(...){return core::fail(core::ErrorCode::failed,"ListModel observer failed after committing the change",std::current_exception());}}
std::span<const ListRowData> ListModel::rows()const{if(auto result=ready();!result)throw std::logic_error(result.error().message);return rows_;}
core::Result<ListRowData> ListModel::row(std::string_view key)const{if(auto result=ready();!result)return std::unexpected(result.error());auto found=indices_.find(key);if(found==indices_.end())return core::fail(core::ErrorCode::not_found,"List row was not found");return rows_[found->second];}
core::Result<bool> ListModel::set_rows(std::vector<ListRowData> rows){
    if(auto result=ready();!result)return std::unexpected(result.error());if(auto result=validate(rows);!result)return std::unexpected(result.error());if(rows==rows_)return false;
    std::map<std::string,std::size_t,std::less<>> ids;for(std::size_t i=0;i<rows.size();++i)ids.emplace(rows[i].key,i);
    rows_=std::move(rows);indices_=std::move(ids);std::erase_if(collapsed_,[&](const auto& key){return !indices_.contains(key);});
    if(!selected_.get().empty()&&!indices_.contains(selected_.get()))selected_.set("");return changed();
}
core::Result<bool> ListModel::rename(std::string_view key,std::string text){
    if(auto result=ready();!result)return std::unexpected(result.error());auto found=indices_.find(key);if(found==indices_.end())return core::fail(core::ErrorCode::not_found,"List row was not found");
    if(!rows_[found->second].editable||!rows_[found->second].enabled)return core::fail(core::ErrorCode::invalid_argument,"List row is not editable");
    auto rows=rows_;rows[found->second].text=std::move(text);return set_rows(std::move(rows));
}
core::Result<bool> ListModel::move(std::string_view key,std::string_view parent,std::string_view before){
    if(auto result=ready();!result)return std::unexpected(result.error());auto found=indices_.find(key);
    if(found==indices_.end()||(!parent.empty()&&!indices_.contains(parent)))return core::fail(core::ErrorCode::not_found,"List move source/parent was not found");
    if(key==parent)return core::fail(core::ErrorCode::invalid_argument,"A list row cannot parent itself");if(key==before)return false;
    if(!before.empty()){auto dest=indices_.find(before);if(dest==indices_.end()||rows_[dest->second].parent!=parent)return core::fail(core::ErrorCode::invalid_argument,"List move must name a sibling insertion point");}
    auto rows=rows_;auto moved=rows[found->second];rows.erase(rows.begin()+std::ptrdiff_t(found->second));moved.parent=parent;
    auto destination=before.empty()?rows.end():std::ranges::find(rows,before,&ListRowData::key);rows.insert(destination,std::move(moved));return set_rows(std::move(rows));
}
core::Result<bool> ListModel::set_checked(std::string_view key,bool value){
    if(auto result=ready();!result)return std::unexpected(result.error());auto found=indices_.find(key);if(found==indices_.end())return core::fail(core::ErrorCode::not_found,"List row was not found");
    auto rows=rows_;rows[found->second].checked=value;return set_rows(std::move(rows));
}
core::Result<bool> ListModel::set_expanded(std::string_view key,bool value){
    if(auto result=ready();!result)return std::unexpected(result.error());if(!indices_.contains(key))return core::fail(core::ErrorCode::not_found,"List row was not found");
    if(value){if(!collapsed_.erase(std::string(key)))return false;}else if(!collapsed_.emplace(key).second)return false;return changed();
}
core::Result<bool> ListModel::select(std::string key){
    if(auto result=ready();!result)return std::unexpected(result.error());if(!key.empty()){auto found=indices_.find(key);if(found==indices_.end()||!rows_[found->second].enabled)return core::fail(core::ErrorCode::not_found,"Enabled list selection was not found");}
    try{return selected_.set(std::move(key));}catch(...){return core::fail(core::ErrorCode::failed,"List selection observer failed",std::current_exception());}
}
core::Result<std::vector<ListVisibleRow>> ListModel::visible_rows(bool hierarchy)const{
    if(auto result=ready();!result)return std::unexpected(result.error());std::vector<ListVisibleRow> visible;visible.reserve(rows_.size());
    if(!hierarchy){for(std::size_t i=0;i<rows_.size();++i)visible.push_back({i,0,false,false});return visible;}
    std::map<std::string,std::vector<std::size_t>,std::less<>> children;for(std::size_t i=0;i<rows_.size();++i)children[rows_[i].parent].push_back(i);
    const auto walk=[&](auto&& self,const std::string& parent,std::size_t depth)->void{
        auto found=children.find(parent);if(found==children.end())return;
        for(auto i:found->second){const bool has=children.contains(rows_[i].key),expanded=!collapsed_.contains(rows_[i].key);visible.push_back({i,depth,has,expanded});if(has&&expanded)self(self,rows_[i].key,depth+1);}
    };walk(walk,"",0);return visible;
}
}
