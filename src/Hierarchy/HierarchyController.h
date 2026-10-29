#pragma once
#include <LVRS/HierarchyProjection.h>
#include <iiSDKCore/Core/Property/Observable.h>
namespace lvrs {
struct HierarchyControllerOptions final {
    bool auto_expand_ancestors=true,editable=false;
    HierarchyProjectionOptions projection;
    std::string initial_active_key;
};
struct HierarchyControllerState final {
    std::uint64_t revision=1,model_revision=1;
    std::string active_key;
    std::int64_t active_id=-1;
    std::size_t active_index=hierarchy_npos,count=0,visible_count=0,eligible_count=0;
    double content_extent=0;
    bool editable=false,dragging=false,closed=false,faulted=false;
    std::string drag_key;
    std::optional<HierarchyMovePlan> drag_target;
};
struct HierarchyExpansionChange final {
    std::string key;
    std::size_t index=hierarchy_npos;
    bool expanded=false;
    bool operator==(const HierarchyExpansionChange&)const=default;
};
struct HierarchyControllerChange final {
    HierarchyControllerState state;
    bool activation=false,expansion=false,structure=false,data=false,drag=false;
    std::string key;
    std::optional<bool> expanded;
    std::optional<HierarchyMovePlan> moved;
    // Actual transitions, in command order. key/expanded retain the final
    // expansion for single-change consumers; state is the committed snapshot.
    std::vector<HierarchyExpansionChange> expansions;
};
using HierarchyControllerPort=ii::runtime::QueryPort<HierarchyControllerChange>;
// Logical view state. Activation, expansion and drag preview never write rows;
// only explicit move commit changes this presentation model.
class HierarchyController final {
public:
    [[nodiscard]] static ii::core::Result<std::shared_ptr<HierarchyController>> create(std::shared_ptr<HierarchyModel>,HierarchyControllerOptions={});
    ~HierarchyController();
    HierarchyController(const HierarchyController&)=delete;
    HierarchyController& operator=(const HierarchyController&)=delete;
    [[nodiscard]] ii::core::Result<HierarchyControllerState> state()const;
    [[nodiscard]] std::shared_ptr<HierarchyModel> model()const;
    [[nodiscard]] ii::core::Result<HierarchyRow> row(std::size_t)const;
    [[nodiscard]] ii::core::Result<HierarchyMetadata> metadata(std::string_view)const;
    [[nodiscard]] ii::core::Result<bool> expanded(std::string_view)const;
    [[nodiscard]] ii::core::Result<std::size_t> at_visible(std::size_t)const;
    [[nodiscard]] ii::core::Result<std::size_t> visible_rank(std::size_t)const;
    [[nodiscard]] ii::core::Result<std::size_t> at_offset(double)const;
    [[nodiscard]] ii::core::Result<double> offset(std::size_t)const;
    [[nodiscard]] ii::core::Result<double> row_extent(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> activate_by_key(std::string_view,bool force_signal=false);
    [[nodiscard]] ii::core::Result<bool> activate_by_id(std::int64_t,bool force_signal=false);
    [[nodiscard]] ii::core::Result<bool> activate_relative(int);
    [[nodiscard]] ii::core::Result<bool> navigate_left();
    [[nodiscard]] ii::core::Result<bool> navigate_right();
    [[nodiscard]] ii::core::Result<bool> set_expanded(std::string_view,bool);
    [[nodiscard]] ii::core::Result<bool> toggle_expanded(std::string_view);
    [[nodiscard]] ii::core::Result<bool> expand_all();
    [[nodiscard]] ii::core::Result<bool> collapse_all(bool keep_root_expanded=true);
    [[nodiscard]] ii::core::Result<bool> set_row_extent(std::string_view,double);
    [[nodiscard]] ii::core::Result<bool> set_default_row_extent(double);
    [[nodiscard]] ii::core::Result<bool> set_auto_expand_ancestors(bool);
    [[nodiscard]] ii::core::Result<bool> set_editable(bool);
    [[nodiscard]] ii::core::Result<bool> begin_drag(std::string_view);
    [[nodiscard]] ii::core::Result<bool> update_drag(std::size_t raw_insertion,double x,double indent_step=8,double base_padding=8);
    [[nodiscard]] ii::core::Result<bool> end_drag(bool commit=false);
    [[nodiscard]] ii::core::Result<bool> move(std::string_view,std::size_t raw_insertion,std::uint32_t depth);
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const HierarchyControllerPort>);
    [[nodiscard]] ii::core::Result<void> close();
private:
    struct Impl;explicit HierarchyController(std::shared_ptr<Impl>);
    std::shared_ptr<Impl> impl_;
};
}
