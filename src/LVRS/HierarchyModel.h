#pragma once
#include <iiSDKCore/Runtime/Query.h>
#include <iiSDKCore/Core/Result/Result.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace ii::gui {class RasterImage;}
namespace lvrs {
inline constexpr std::size_t hierarchy_npos=std::numeric_limits<std::size_t>::max();
struct HierarchyRow final {
    std::string key,label,icon_name,icon_glyph;
    std::int64_t item_id=-1,count=-1;
    std::uint32_t depth=0;
    bool enabled=true,activatable=true,draggable=true,show_chevron=true,has_child_items=false,expanded=false,selected=false;
    std::shared_ptr<const ii::gui::RasterImage> icon;
    friend bool operator==(const HierarchyRow&,const HierarchyRow&)=default;
};
struct HierarchyModelOptions final {
    std::size_t maximum_rows=1000000,maximum_depth=256,maximum_text_bytes=65536;
    std::size_t maximum_data_bytes=256*1024*1024,maximum_metadata_bytes=1024*1024;
    bool editable=true;
};
struct HierarchyRelation final {
    std::size_t parent=hierarchy_npos,first_child=hierarchy_npos,next_sibling=hierarchy_npos,previous_sibling=hierarchy_npos;
    std::size_t subtree_end=0,sibling_index=0,sibling_count=0,child_count=0;
    std::uint32_t subtree_maximum_depth=0;
};
struct HierarchyMetadata final {
    std::size_t flat_index=0,parent_index=hierarchy_npos,sibling_index=0,sibling_count=0,child_count=0,descendant_count=0;
    std::optional<std::size_t> visible_index,visible_sibling_index;
    std::size_t visible_sibling_count=0,visible_child_count=0,visible_descendant_count=0;
    bool visible=false,has_children=false;
    std::string parent_key,parent_label,parent_path_label,path_label;
    std::vector<std::string> child_keys,child_labels,ancestor_keys,ancestor_labels,path_keys,path_labels;
};
enum class HierarchyDropMode {root,before,after,child};
struct HierarchyMovePlan final {
    std::uint64_t revision=0;
    std::size_t source=0,source_end=0,raw_insertion=0,insertion=0,parent=hierarchy_npos,anchor=hierarchy_npos;
    std::uint32_t depth=0,minimum_depth=0,maximum_depth=0;
    HierarchyDropMode mode=HierarchyDropMode::root;
    std::string source_key,parent_key,anchor_key;
    friend bool operator==(const HierarchyMovePlan&,const HierarchyMovePlan&)=default;
};
enum class HierarchyChangeKind {reset,data,move,capabilities,closed};
struct HierarchyChange final {
    std::uint64_t revision=0;
    HierarchyChangeKind kind=HierarchyChangeKind::reset;
    std::size_t count=0,first=0,end=0;
    bool visibility=false,activation=false;
    std::optional<HierarchyMovePlan> move;
};
using HierarchyChangePort=ii::runtime::QueryPort<HierarchyChange>;
struct HierarchyModelState final {std::uint64_t revision=1;std::size_t count=0;bool editable=true,closed=false;};
struct HierarchyModelStats final {std::uint64_t topology_builds=0,data_updates=0,move_plans=0,moves=0,metadata_queries=0,notifications=0;};
// Owner-thread typed preorder data. Views own activation/expansion/scrolling.
// Borrowed spans and references remain valid only until the next mutation.
class HierarchyModel final {
public:
    ~HierarchyModel();
    HierarchyModel(const HierarchyModel&)=delete;
    HierarchyModel& operator=(const HierarchyModel&)=delete;
    HierarchyModel(HierarchyModel&&)=delete;
    HierarchyModel& operator=(HierarchyModel&&)=delete;
    [[nodiscard]] static ii::core::Result<std::shared_ptr<HierarchyModel>> create(HierarchyModelOptions={});
    [[nodiscard]] ii::core::Result<bool> set_rows(std::vector<HierarchyRow>);
    [[nodiscard]] ii::core::Result<bool> update_row(std::string_view,HierarchyRow);
    [[nodiscard]] ii::core::Result<bool> set_editable(bool);
    [[nodiscard]] ii::core::Result<HierarchyModelState> state()const;
    [[nodiscard]] ii::core::Result<HierarchyModelStats> stats()const;
    [[nodiscard]] ii::core::Result<std::span<const HierarchyRow>> rows()const;
    [[nodiscard]] ii::core::Result<std::reference_wrapper<const HierarchyRow>> row(std::size_t)const;
    [[nodiscard]] ii::core::Result<std::reference_wrapper<const HierarchyRow>> row(std::string_view)const;
    [[nodiscard]] ii::core::Result<std::size_t> index_of_key(std::string_view)const;
    [[nodiscard]] ii::core::Result<std::size_t> index_of_id(std::int64_t)const;
    [[nodiscard]] ii::core::Result<HierarchyRelation> relation(std::size_t)const;
    [[nodiscard]] ii::core::Result<HierarchyMetadata> metadata(std::size_t)const;
    [[nodiscard]] ii::core::Result<HierarchyMovePlan> plan_move(std::string_view,std::size_t raw_insertion,std::uint32_t depth)const;
    [[nodiscard]] ii::core::Result<HierarchyMovePlan> resolve_drag_target(std::string_view,std::size_t raw_insertion,double x,double indent_step=8,double base_padding=8)const;
    [[nodiscard]] ii::core::Result<bool> commit_move(const HierarchyMovePlan&);
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const HierarchyChangePort>);
    [[nodiscard]] ii::core::Result<void> close();
private:
    struct Impl;explicit HierarchyModel(std::shared_ptr<Impl>);
    std::shared_ptr<Impl> impl_;
};
} // namespace lvrs
