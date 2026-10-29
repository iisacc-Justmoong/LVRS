#pragma once
#include <LVRS/ViewModel.h>
#include <LVRS/StateSource.h>
namespace lvrs {
enum class StateEvent {revision,values,keys,value};
struct StateChange final {
    StateEvent event=StateEvent::revision;
    std::uint64_t revision=0;
    std::string key;
    ii::core::Value value,previous;
    bool present=false,previously_present=false;
};
using StateChangePort=ii::runtime::QueryPort<StateChange>;
class StateModel final:public ViewModel,public StateSource {
public:
    StateModel();~StateModel()override;
    [[nodiscard]] ii::core::Value::Object values()const;
    [[nodiscard]] std::vector<std::string> state_keys()const;
    [[nodiscard]] std::uint64_t revision()const override;
    [[nodiscard]] bool empty()const;
    [[nodiscard]] ii::core::Value value(std::string_view,ii::core::Value fallback={})const;
    [[nodiscard]] ii::core::Value value_or(std::string_view,ii::core::Value fallback={})const override;
    [[nodiscard]] bool has_value(std::string_view)const;
    [[nodiscard]] ii::core::Value::Object state_snapshot()const;
    [[nodiscard]] std::optional<ii::core::Value> read_property(std::string_view)const override;
    [[nodiscard]] ii::core::Result<bool> write_property(std::string_view,ii::core::Value)override;
    [[nodiscard]] ii::core::Value::Object model_descriptor()const override;
    [[nodiscard]] ii::core::Result<bool> set_values(ii::core::Value::Object);
    // Like the original: unchanged set_value/apply_patch returns true; an empty
    // key returns false and sets the ViewModel error. Empty patch returns false.
    [[nodiscard]] ii::core::Result<bool> set_value(std::string,ii::core::Value);
    [[nodiscard]] ii::core::Result<bool> remove_value(std::string);
    [[nodiscard]] ii::core::Result<bool> apply_patch(ii::core::Value::Object);
    [[nodiscard]] ii::core::Result<bool> clear_values();
    using ViewModel::observe;
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const StateChangePort>);
    [[nodiscard]] ii::core::ObservableSubscription observe_revision(std::function<void()>)const override;
private:
    struct Impl;
    std::shared_ptr<Impl> state_;
};
}
