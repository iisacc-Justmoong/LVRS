#pragma once
#include "StateSource.h"
#include <iiSDKCore/Runtime/Query.h>
namespace lvrs {
struct ProgressValues final {double minimum=0,maximum=100,start=0,current=0;};
struct ProgressStateKeys final {
    std::string minimum="minimumValue",maximum="maximumValue",start="startValue",current="currentValue";
    bool operator==(const ProgressStateKeys&)const=default;
};
struct ProgressSnapshot final {
    ProgressValues effective;
    double value_range=100,normalized_start=0,normalized_current=0,fill_start=0,fill_progress=0;
};
enum class ProgressEvent {values,state_model,keys,revision,effective};
struct ProgressChange final {ProgressEvent event=ProgressEvent::values;std::uint64_t state_revision=0;};
using ProgressChangePort=ii::runtime::QueryPort<ProgressChange>;
class ProgressModel final {
public:
    ProgressModel();~ProgressModel();
    ProgressModel(const ProgressModel&)=delete;ProgressModel& operator=(const ProgressModel&)=delete;
    [[nodiscard]] double minimum_value()const;
    [[nodiscard]] double maximum_value()const;
    [[nodiscard]] double start_value()const;
    [[nodiscard]] double current_value()const;
    [[nodiscard]] ii::core::Result<bool> set_values(ProgressValues);
    [[nodiscard]] ii::core::Result<bool> set_configuration(ProgressValues,ProgressStateKeys,std::shared_ptr<StateSource>);
    [[nodiscard]] ii::core::Result<bool> set_minimum_value(double);
    [[nodiscard]] ii::core::Result<bool> set_maximum_value(double);
    [[nodiscard]] ii::core::Result<bool> set_start_value(double);
    [[nodiscard]] ii::core::Result<bool> set_current_value(double);
    [[nodiscard]] std::shared_ptr<StateSource> state_model()const;
    [[nodiscard]] ii::core::Result<bool> set_state_model(std::shared_ptr<StateSource>);
    [[nodiscard]] ProgressStateKeys state_keys()const;
    [[nodiscard]] std::string minimum_value_state_key()const;
    [[nodiscard]] std::string maximum_value_state_key()const;
    [[nodiscard]] std::string start_value_state_key()const;
    [[nodiscard]] std::string current_value_state_key()const;
    [[nodiscard]] ii::core::Result<bool> set_state_keys(ProgressStateKeys);
    [[nodiscard]] ii::core::Result<bool> set_minimum_value_state_key(std::string);
    [[nodiscard]] ii::core::Result<bool> set_maximum_value_state_key(std::string);
    [[nodiscard]] ii::core::Result<bool> set_start_value_state_key(std::string);
    [[nodiscard]] ii::core::Result<bool> set_current_value_state_key(std::string);
    [[nodiscard]] bool using_state_model()const;
    [[nodiscard]] std::uint64_t state_revision()const;
    [[nodiscard]] ProgressSnapshot snapshot()const;
    [[nodiscard]] double effective_minimum_value()const;
    [[nodiscard]] double effective_maximum_value()const;
    [[nodiscard]] double effective_start_value()const;
    [[nodiscard]] double effective_current_value()const;
    [[nodiscard]] double value_range()const;
    [[nodiscard]] double normalized_start()const;
    [[nodiscard]] double normalized_current()const;
    [[nodiscard]] double fill_start()const;
    [[nodiscard]] double fill_progress()const;
    [[nodiscard]] double progress()const;
    [[nodiscard]] double state_number(std::string_view,double fallback)const;
    [[nodiscard]] double normalized_value(double)const;
    [[nodiscard]] double radius_for(int shape_style,double corner_radius,double width,double height)const;
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const ProgressChangePort>);
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
