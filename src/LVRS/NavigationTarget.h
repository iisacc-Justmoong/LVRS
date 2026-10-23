#pragma once
#include "NavigationStackModel.h"

namespace lvrs {
class PageComponent;
using PageComponentRef=std::shared_ptr<const PageComponent>;
struct NavigationPlacement final {
    std::weak_ptr<const void> sibling_group;
    std::uint64_t parent=std::uint64_t(-1),sibling_order=0;
};
struct NavigationTargetState final {
    NavigationState navigation;
    bool mounted=false,can_go_back=false;
    std::uint64_t content_generation=0;
};
struct NavigationTargetChange final {
    NavigationState navigation;
    PageComponentRef component;
    std::uint64_t content_generation=0;
};
using NavigationTargetPort=ii::runtime::QueryPort<NavigationTargetChange>;
// The global navigator depends on this replaceable owner-thread contract,
// rather than a concrete renderer, controller or source interpreter.
class NavigationTarget {
public:
    virtual ~NavigationTarget();
    [[nodiscard]] virtual ii::core::Result<NavigationPlacement> placement()const;
    // Optional logical presentation stack. Targets without stack descriptors
    // return an empty list; widget ownership is not inferred from state().
    [[nodiscard]] virtual ii::core::Result<std::vector<NavigationViewEntry>> view_tracking_entries()const;
    [[nodiscard]] virtual ii::core::Result<NavigationTargetState> state()const=0;
    [[nodiscard]] virtual ii::core::Result<bool> go(std::string_view,ii::core::Value={},NavigationOperation=NavigationOperation::push)=0;
    [[nodiscard]] virtual ii::core::Result<bool> go_to(PageComponentRef,ii::core::Value={},NavigationOperation=NavigationOperation::push)=0;
    [[nodiscard]] virtual ii::core::Result<bool> back()=0;
    [[nodiscard]] virtual ii::core::Result<bool> pop_to_root()=0;
    [[nodiscard]] virtual ii::core::Result<void> observe(std::weak_ptr<NavigationTargetPort>)=0;
};
}
