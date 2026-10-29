#include <LVRS/PageMonitor.h>
#include <LVRS/TrackingDetail.h>

namespace lvrs {
using namespace ii;
struct PageMonitor::Impl final {
    tracking_detail::Owner owner;
    tracking_detail::Receivers<PageMonitorPort> receivers;
    std::vector<std::string> history;
    std::size_t bytes=0;
    PageMonitorState state()const{return {.revision=owner.revision,.count=history.size(),.current=history.empty()?std::string{}:history.back(),.can_undo=history.size()>1};}
    core::Result<void> publish(){++owner.revision;return receivers.notify(owner,state());}
};
PageMonitor::PageMonitor():impl_(std::make_shared<Impl>()){}
PageMonitor::~PageMonitor()=default;
std::shared_ptr<PageMonitor> PageMonitor::current(){thread_local auto model=std::make_shared<PageMonitor>();return model;}
core::Result<PageMonitorState> PageMonitor::state()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return p->state();}
core::Result<std::vector<std::string>> PageMonitor::history()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return p->history;}
core::Result<std::span<const std::string>> PageMonitor::history_view()const{auto p=impl_;if(auto result=p->owner.ready();!result)return std::unexpected(result.error());return std::span<const std::string>{p->history};}
core::Result<bool> PageMonitor::record(std::string_view path){
    auto p=impl_;if(auto result=p->owner.ready(true);!result)return std::unexpected(result.error());
    auto next=tracking_detail::text(path);if(!next)return std::unexpected(next.error());
    if(next->empty()||(!p->history.empty()&&p->history.back()==*next))return false;
    const auto cost=next->size()+sizeof(std::string);
    if(p->history.size()==NavigationLimits::depth||cost>NavigationLimits::total_bytes-p->bytes)return core::fail(core::ErrorCode::full,"Page monitor history budget exceeded");
    if(auto result=p->owner.can_publish();!result)return std::unexpected(result.error());
    p->history.push_back(std::move(*next));p->bytes+=cost;if(auto result=p->publish();!result)return std::unexpected(result.error());return true;
}
core::Result<std::string> PageMonitor::undo(){
    auto p=impl_;if(auto result=p->owner.ready(true);!result)return std::unexpected(result.error());
    auto current=p->history.empty()?std::string{}:p->history[p->history.size()>1?p->history.size()-2:0];
    if(p->history.size()<=1)return current;if(auto result=p->owner.can_publish();!result)return std::unexpected(result.error());
    p->bytes-=p->history.back().size()+sizeof(std::string);p->history.pop_back();if(auto result=p->publish();!result)return std::unexpected(result.error());return current;
}
core::Result<bool> PageMonitor::clear(){
    auto p=impl_;if(auto result=p->owner.ready(true);!result)return std::unexpected(result.error());if(p->history.empty())return false;
    if(auto result=p->owner.can_publish();!result)return std::unexpected(result.error());p->history.clear();p->bytes=0;if(auto result=p->publish();!result)return std::unexpected(result.error());return true;
}
core::Result<void> PageMonitor::observe(std::weak_ptr<PageMonitorPort> receiver){auto p=impl_;if(auto result=p->owner.ready(true);!result)return result;return p->receivers.observe(std::move(receiver));}
}
