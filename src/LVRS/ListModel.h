#pragma once
#include <iiSDKCore/Core/Property/Observable.h>
#include <iiSDKCore/Core/Result/Result.h>
#include <map>
#include <set>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace lvrs {
struct ListRowData final {
    std::string key,text,parent;
    bool enabled=true,editable=true,checked=true;
    friend bool operator==(const ListRowData&,const ListRowData&)=default;
};
struct ListVisibleRow final {std::size_t index=0,depth=0;bool has_children=false,expanded=false;};
// Presentation model. Selection/expansion never mutate the consumer's document.
// Consumers durably apply the explicit rename/move/check callbacks themselves.
class ListModel final {
public:
    ListModel()=default;
    [[nodiscard]] ii::core::Result<bool> set_rows(std::vector<ListRowData>);
    [[nodiscard]] ii::core::Result<bool> rename(std::string_view,std::string);
    [[nodiscard]] ii::core::Result<bool> move(std::string_view key,std::string_view parent,std::string_view before={});
    [[nodiscard]] ii::core::Result<bool> set_checked(std::string_view,bool);
    [[nodiscard]] ii::core::Result<bool> set_expanded(std::string_view,bool);
    [[nodiscard]] ii::core::Result<bool> select(std::string);
    [[nodiscard]] ii::core::Result<std::vector<ListVisibleRow>> visible_rows(bool hierarchy=true)const;
    [[nodiscard]] ii::core::Result<ListRowData> row(std::string_view)const;
    [[nodiscard]] std::span<const ListRowData> rows()const;
    [[nodiscard]] ii::core::Observable<std::uint64_t>& changes()noexcept{return changes_;}
    [[nodiscard]] ii::core::Observable<std::string>& selection()noexcept{return selected_;}
private:
    [[nodiscard]] ii::core::Result<void> ready()const;
    [[nodiscard]] ii::core::Result<void> validate(const std::vector<ListRowData>&)const;
    [[nodiscard]] ii::core::Result<bool> changed();
    std::vector<ListRowData> rows_;
    std::map<std::string,std::size_t,std::less<>> indices_;
    std::set<std::string,std::less<>> collapsed_;
    const std::thread::id owner_=std::this_thread::get_id();
    ii::core::Observable<std::uint64_t> changes_{0};
    ii::core::Observable<std::string> selected_{""};
};
}
