#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
namespace lvrs::hierarchy_detail {
// Range-add coverage index: visible leaves have no collapsed ancestor.
// Minimum/count/extent/eligible aggregates avoid subtree walks on expansion.
class VisibilityIndex final {
public:
    struct Leaf final {int hidden=0;double extent=20;bool eligible=true;};
    void reset(std::span<const Leaf>);
    void add(std::size_t first,std::size_t end,int delta);
    void extent(std::size_t,double);
    void eligible(std::size_t,bool);
    [[nodiscard]] std::size_t count()const;
    [[nodiscard]] std::size_t eligible_count()const;
    [[nodiscard]] double extent()const;
    [[nodiscard]] std::size_t prefix_count(std::size_t end,bool eligible=false)const;
    [[nodiscard]] double prefix_extent(std::size_t end)const;
    [[nodiscard]] bool visible(std::size_t)const;
    [[nodiscard]] std::size_t at(std::size_t rank,bool eligible=false)const;
    [[nodiscard]] std::size_t at_offset(double)const;
private:
    struct Node final {int minimum=0,lazy=0;std::size_t count=0,eligible=0;double extent=0;};
    std::vector<Node> nodes_;std::size_t size_=0;
    void build(std::size_t,std::size_t,std::size_t,std::span<const Leaf>);
    void pull(std::size_t);
    void add(std::size_t,std::size_t,std::size_t,std::size_t,std::size_t,int);
    void update(std::size_t,std::size_t,std::size_t,std::size_t,double,bool,bool);
    [[nodiscard]] Node range(std::size_t,std::size_t,std::size_t,std::size_t,int)const;
};
} // namespace lvrs::hierarchy_detail
