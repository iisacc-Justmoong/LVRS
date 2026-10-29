#pragma once
#include <LVRS/ModelProperties.h>
#include <iiSDKCore/Core/Value/Value.h>
#include <iiSDKCore/Runtime/Query.h>
#include <memory>
namespace lvrs {
enum class ViewModelField {key,display_name,busy,error,metadata};
struct ViewModelChange final {ViewModelField field=ViewModelField::key;};
using ViewModelChangePort=ii::runtime::QueryPort<ViewModelChange>;
// Owner-thread presentation state. Values are owned Core values; snapshots never
// alias mutable storage. Observation failures describe an already committed edit.
class ViewModel:public ModelProperties {
public:
    ViewModel();
    virtual ~ViewModel();
    ViewModel(const ViewModel&)=delete;ViewModel& operator=(const ViewModel&)=delete;
    [[nodiscard]] std::string key()const;
    [[nodiscard]] std::string display_name()const;
    [[nodiscard]] bool busy()const;
    [[nodiscard]] std::string error()const;
    [[nodiscard]] bool has_error()const;
    [[nodiscard]] ii::core::Value::Object metadata()const;
    [[nodiscard]] ii::core::Value::Object snapshot()const;
    [[nodiscard]] std::optional<ii::core::Value> read_property(std::string_view)const override;
    [[nodiscard]] ii::core::Result<bool> write_property(std::string_view,ii::core::Value)override;
    [[nodiscard]] ii::core::Value::Object model_descriptor()const override;
    [[nodiscard]] bool is_view_model()const noexcept override{return true;}
    [[nodiscard]] ii::core::Result<bool> set_key(std::string);
    [[nodiscard]] ii::core::Result<bool> set_display_name(std::string);
    [[nodiscard]] ii::core::Result<bool> set_busy(bool);
    [[nodiscard]] ii::core::Result<bool> set_error(std::string);
    [[nodiscard]] ii::core::Result<bool> set_metadata(ii::core::Value::Object);
    [[nodiscard]] ii::core::Result<bool> clear_error();
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const ViewModelChangePort>);
private:
    friend class StateModel;
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
