#pragma once
#include <iiSDKCore/Core/Property/ObservableSubscription.h>
#include <iiSDKCore/Core/Value/Value.h>
#include <functional>
namespace lvrs {
// Typed replacement for the original valueOr/revisionChanged protocol. Reads
// and callbacks use the source's owner domain. A final callback invalidates a
// source being destroyed; observers borrow the source and never keep it alive.
class StateSource {
public:
    virtual ~StateSource();
    [[nodiscard]] virtual ii::core::Value value_or(std::string_view,ii::core::Value fallback={})const=0;
    [[nodiscard]] virtual std::uint64_t revision()const=0;
    [[nodiscard]] virtual ii::core::ObservableSubscription observe_revision(std::function<void()>)const=0;
};
}
