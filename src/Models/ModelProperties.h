#pragma once
#include <iiSDKCore/Core/Value/Value.h>
#include <iiSDKCore/Core/Property/ObservableSubscription.h>
#include <functional>
#include <memory>
#include <optional>

namespace lvrs {
// Live model access, not a second property store. Unknown properties are absent;
// an accepted unchanged write is true. Read-only/unknown writes return false.
class ModelProperties {
    struct Impl;
public:
    class DescriptorSignal final {
    public:
        [[nodiscard]] ii::core::Result<void> notify()const;
    private:
        friend class ModelProperties;
        std::weak_ptr<Impl> state_;
    };
    ModelProperties();
    virtual ~ModelProperties();
    ModelProperties(const ModelProperties&)=delete;
    ModelProperties& operator=(const ModelProperties&)=delete;
    [[nodiscard]] virtual std::optional<ii::core::Value> read_property(std::string_view)const=0;
    [[nodiscard]] virtual ii::core::Result<bool> write_property(std::string_view,ii::core::Value)=0;
    [[nodiscard]] virtual ii::core::Value::Object model_descriptor()const=0;
    [[nodiscard]] virtual bool is_view_model()const noexcept{return false;}
    [[nodiscard]] ii::core::ObservableSubscription observe_descriptor(std::function<void()>)const;
protected:
    // Implementations notify only after their native fields commit. The weak
    // token is safe to keep in an implementation surviving an in-flight call.
    [[nodiscard]] DescriptorSignal descriptor_signal()const noexcept;
private:
    std::shared_ptr<Impl> properties_;
};
}
