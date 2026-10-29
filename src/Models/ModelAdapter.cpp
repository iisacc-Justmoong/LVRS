#include <LVRS/ModelAdapter.h>
#include <algorithm>
namespace lvrs {
bool ModelAdapter::isItemModel(const ModelInput& source)const{return bool(source.itemModel());}
int ModelAdapter::count(const ModelInput& source)const{
    if(auto model=source.itemModel()){model->requireOwner();return std::max(0,model->rowCount());}
    return 0;
}
ii::core::Value::Object ModelAdapter::row(const ModelInput& source,int index,int column)const{
    auto model=source.itemModel();if(!model)return {};
    model->requireOwner();if(index<0||index>=model->rowCount())return {};
    column=std::max(0,column);if(column>=model->columnCount())column=0;
    if(!model->validIndex(index,column))return {};
    ii::core::Value::Object result;
    for(const auto& [role,name]:model->roleNames())if(!name.empty())if(auto value=model->data(index,column,role))result.insert_or_assign(name,std::move(*value));
    for(const auto& [role,name]:{std::pair{0,"display"},std::pair{2,"edit"}})
        if(!result.contains(name))if(auto value=model->data(index,column,role))result.emplace(name,std::move(*value));
    result.insert_or_assign("index",ii::core::Value(index));
    result.insert_or_assign("row",ii::core::Value(index));
    result.insert_or_assign("column",ii::core::Value(column));
    return result;
}
}
