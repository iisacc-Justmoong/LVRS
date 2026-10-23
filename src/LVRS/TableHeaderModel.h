#pragma once
#include <iiSDKCore/Core/Result/Result.h>
#include <cstdint>
#include <span>
#include <optional>
#include <string>
#include <vector>
namespace lvrs {
struct TableHeaderColumn final {
    std::string text="Column",value_type="string";
    std::optional<double> padding;
    friend bool operator==(const TableHeaderColumn&,const TableHeaderColumn&)=default;
};
struct TableHeaderModelOptions final {
    double width=717,row_height=24,horizontal_padding=8,fallback_cell_width=0,minimum_column_width=32;
    std::vector<double> column_widths;
    std::vector<TableHeaderColumn> columns=std::vector<TableHeaderColumn>(3);
    friend bool operator==(const TableHeaderModelOptions&,const TableHeaderModelOptions&)=default;
};
struct TableHeaderDescriptor final {
    std::size_t index=0;
    TableHeaderColumn source;
    double x=0,width=0,height=24,padding=8;
};
// Value-owned header geometry. Revisions rebuild once; column lookup is O(1).
// The owner synchronizes mutation. Returned spans expire on options replacement.
class TableHeaderModel final {
public:
    TableHeaderModel();
    [[nodiscard]] ii::core::Result<void> set_options(TableHeaderModelOptions);
    [[nodiscard]] const TableHeaderModelOptions& options()const noexcept{return options_;}
    [[nodiscard]] std::span<const TableHeaderDescriptor> descriptors()const noexcept{return descriptors_;}
    [[nodiscard]] ii::core::Result<TableHeaderDescriptor> descriptor_at(std::size_t)const;
    [[nodiscard]] std::uint64_t revision()const noexcept{return revision_;}
private:
    TableHeaderModelOptions options_;
    std::vector<TableHeaderDescriptor> descriptors_;
    std::uint64_t revision_=0;
};
}
