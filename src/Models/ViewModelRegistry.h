#pragma once
#include <LVRS/ModelProperties.h>
#include <iiSDKCore/Runtime/Query.h>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace lvrs {
enum class ModelRetention {borrow,retain};
enum class ViewModelRegistryEvent {keys,views,ownership,descriptors,last_error};
struct ViewModelRegistryChange final {ViewModelRegistryEvent event=ViewModelRegistryEvent::descriptors;};
using ViewModelRegistryPort=ii::runtime::QueryPort<ViewModelRegistryChange>;
struct ViewModelRegistryLimits final {
    std::size_t models=65536,views=65536,token_bytes=8192,pending_rounds=256;
};
struct RouteModelBinding final {
    std::string view_id,model_key;
    bool writable=false;
};
// One owner-thread domain. Default registrations borrow their existing owner;
// adoption is explicit and lasts until the last alias leaves this registry.
class ViewModelRegistry final {
public:
    explicit ViewModelRegistry(ViewModelRegistryLimits={});
    ~ViewModelRegistry();
    ViewModelRegistry(const ViewModelRegistry&)=delete;
    ViewModelRegistry& operator=(const ViewModelRegistry&)=delete;
    [[nodiscard]] std::shared_ptr<ModelProperties> get(std::string_view)const;
    [[nodiscard]] ii::core::Result<bool> set(std::string,std::shared_ptr<ModelProperties>,ModelRetention=ModelRetention::borrow);
    [[nodiscard]] ii::core::Result<bool> register_view_model(std::shared_ptr<ModelProperties>,std::string fallback_key={},ModelRetention=ModelRetention::borrow);
    [[nodiscard]] ii::core::Result<bool> remove(std::string_view);
    [[nodiscard]] ii::core::Result<bool> clear();
    [[nodiscard]] ii::core::Result<bool> bind_view(std::string_view,std::string_view,bool writable=false);
    [[nodiscard]] ii::core::Result<bool> unbind_view(std::string_view);
    // Command preflight only: owner, busy state and normalized ID budget.
    // Model existence/ownership are still resolved when the page is published.
    [[nodiscard]] ii::core::Result<void> validate_view_admission(std::string_view)const;
    // Lifecycle cleanup may be admitted from an observer. Actual removal is
    // drained after the current registry command; ordinary writes stay busy.
    [[nodiscard]] ii::core::Result<bool> retire_views(std::span<const std::string>);
    [[nodiscard]] std::shared_ptr<ModelProperties> get_for_view(std::string_view)const;
    [[nodiscard]] std::string key_for_view(std::string_view)const;
    [[nodiscard]] ii::core::Result<bool> claim_ownership(std::string_view,std::string_view);
    [[nodiscard]] ii::core::Result<bool> release_ownership(std::string_view,std::string_view key={});
    [[nodiscard]] bool can_write(std::string_view,std::string_view key={})const;
    [[nodiscard]] std::string owner_of(std::string_view)const;
    [[nodiscard]] ii::core::Result<bool> update_property(std::string_view,std::string_view,ii::core::Value);
    [[nodiscard]] ii::core::Result<bool> update_property_by_key(std::string_view,std::string_view,std::string_view,ii::core::Value);
    [[nodiscard]] std::optional<ii::core::Value> read_property(std::string_view,std::string_view)const;
    [[nodiscard]] ii::core::Value::Object descriptor(std::string_view)const;
    [[nodiscard]] ii::core::Result<bool> bind_route_view_model(std::string_view,ii::core::Value::Object,ii::core::Value::Object,std::ptrdiff_t fallback_index=0);
    [[nodiscard]] static RouteModelBinding route_binding(std::string_view,const ii::core::Value::Object&,const ii::core::Value::Object&,std::ptrdiff_t fallback_index=0);
    [[nodiscard]] std::vector<std::string> keys()const;
    [[nodiscard]] std::vector<std::string> views()const;
    [[nodiscard]] ii::core::Value::Object bindings()const;
    [[nodiscard]] ii::core::Value::Object owners()const;
    [[nodiscard]] ii::core::Value::Object descriptors()const;
    [[nodiscard]] std::string last_error()const;
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const ViewModelRegistryPort>);
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
using ViewModels=ViewModelRegistry;
}
