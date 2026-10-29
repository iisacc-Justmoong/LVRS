#pragma once
#include "HierarchyModel.h"
namespace lvrs {
struct HierarchyProjectionOptions final {
    double row_extent=20;
    std::size_t maximum_rows=1000000,maximum_index_bytes=256*1024*1024;
};
struct HierarchyProjectionState final {
    std::uint64_t revision=1,model_revision=1,topology_resets=0,range_updates=0;
    std::size_t count=0,visible_count=0,eligible_count=0;
    double content_extent=0;
    bool closed=false,faulted=false;
};
struct HierarchyProjectionChange final {
    HierarchyProjectionState state;
    std::size_t first=0,end=0;
    bool structure=false,visibility=false;
};
using HierarchyProjectionPort=ii::runtime::QueryPort<HierarchyProjectionChange>;
// A view-local, owner-thread visibility/height index. Navigation is not a
// mutation of the source model. Public indices always refer to preorder rows.
class HierarchyProjection final {
public:
    ~HierarchyProjection();
    HierarchyProjection(const HierarchyProjection&)=delete;
    HierarchyProjection& operator=(const HierarchyProjection&)=delete;
    HierarchyProjection(HierarchyProjection&&)=delete;
    HierarchyProjection& operator=(HierarchyProjection&&)=delete;
    [[nodiscard]] static ii::core::Result<std::shared_ptr<HierarchyProjection>> create(std::shared_ptr<HierarchyModel>,HierarchyProjectionOptions={});
    [[nodiscard]] ii::core::Result<HierarchyProjectionState> state()const;
    [[nodiscard]] ii::core::Result<bool> expanded(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> visible(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> eligible(std::size_t)const;
    [[nodiscard]] ii::core::Result<std::size_t> visible_rank(std::size_t)const;
    [[nodiscard]] ii::core::Result<std::size_t> eligible_rank(std::size_t)const;
    [[nodiscard]] ii::core::Result<std::size_t> at_visible(std::size_t)const;
    [[nodiscard]] ii::core::Result<std::size_t> at_eligible(std::size_t)const;
    [[nodiscard]] ii::core::Result<std::size_t> at_offset(double)const;
    [[nodiscard]] ii::core::Result<double> offset(std::size_t)const;
    [[nodiscard]] ii::core::Result<double> row_extent(std::size_t)const;
    [[nodiscard]] ii::core::Result<HierarchyMetadata> metadata(std::size_t)const;
    [[nodiscard]] ii::core::Result<bool> set_expanded(std::size_t,bool);
    [[nodiscard]] ii::core::Result<bool> expand_ancestors(std::size_t);
    [[nodiscard]] ii::core::Result<bool> expand_all();
    [[nodiscard]] ii::core::Result<bool> collapse_all(bool keep_root_expanded=true);
    [[nodiscard]] ii::core::Result<bool> set_row_extent(std::size_t,double);
    [[nodiscard]] ii::core::Result<bool> set_default_row_extent(double);
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const HierarchyProjectionPort>);
    [[nodiscard]] ii::core::Result<void> close();
private:
    struct Impl;explicit HierarchyProjection(std::shared_ptr<Impl>);
    std::shared_ptr<Impl> impl_;
};
} // namespace lvrs
