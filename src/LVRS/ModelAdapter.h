#pragma once
#include "ModelSource.h"
namespace lvrs {
// Stateless native equivalent of the original root item-model adapter.
class ModelAdapter final {
public:
    [[nodiscard]] bool isItemModel(const ModelInput&)const;
    [[nodiscard]] int count(const ModelInput&)const;
    [[nodiscard]] ii::core::Value::Object row(const ModelInput&,int row,int column=0)const;
};
}
