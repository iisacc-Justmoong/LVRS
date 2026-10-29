#pragma once
#include <LVRS/NavigationStackModel.h>
#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <thread>

namespace lvrs::tracking_detail {
struct Owner final {
    std::thread::id thread=std::this_thread::get_id();
    std::uint64_t revision=1;
    bool notifying=false;
    [[nodiscard]] ii::core::Result<void> ready(bool mutate=false)const;
    [[nodiscard]] ii::core::Result<void> can_publish()const;
};
[[nodiscard]] ii::core::Result<std::string> text(std::string_view);
template<class Port>struct Receivers final {
    static constexpr std::size_t capacity=256;
    std::vector<std::weak_ptr<Port>> ports;
    ii::core::Result<void> observe(std::weak_ptr<Port> receiver){
        auto live=receiver.lock();if(!live)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Tracking receiver has expired");
        std::erase_if(ports,[](const auto& port){return port.expired();});
        for(const auto& port:ports)if(port.lock()==live)return {};
        if(ports.size()==capacity)return ii::core::fail(ii::core::ErrorCode::full,"Tracking receiver limit is 256");
        ports.push_back(std::move(receiver));return {};
    }
    template<class Event>ii::core::Result<void> notify(Owner& owner,const Event& event){
        std::array<std::shared_ptr<Port>,capacity> pending{};
        std::array<const Port*,capacity> closed{};std::size_t count=0,closed_count=0;
        std::erase_if(ports,[](const auto& port){return port.expired();});
        for(const auto& port:ports)if(auto live=port.lock())pending[count++]=std::move(live);
        owner.notifying=true;struct Reset final{bool& flag;~Reset(){flag=false;}}reset{owner.notifying};
        std::optional<ii::core::Error> failure;
        for(std::size_t i=0;i<count;++i){auto reply=pending[i]->send(event);if(!reply){
            if(reply.error().code==ii::runtime::QueryErrorCode::receiver_expired)closed[closed_count++]=pending[i].get();
            else if(!failure)failure=ii::core::Error{ii::core::ErrorCode::failed,"Tracking state committed; a receiver failed",reply.error().cause};
        }}
        std::erase_if(ports,[&](const auto& weak){auto port=weak.lock();return !port||std::find(closed.begin(),closed.begin()+closed_count,port.get())!=closed.begin()+closed_count;});
        if(failure)return std::unexpected(*failure);return {};
    }
};
}
