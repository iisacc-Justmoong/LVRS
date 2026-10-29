#include <LVRS/TrackingDetail.h>
#include <LVRS/RoutingDetail.h>
#include <iiSDKCore/Core/Text/Utf8.h>

namespace lvrs::tracking_detail {
ii::core::Result<void> Owner::ready(bool mutate)const{
    if(thread!=std::this_thread::get_id())return ii::core::fail(ii::core::ErrorCode::wrong_executor,"Tracking model requires its owner thread");
    if(mutate&&notifying)return ii::core::fail(ii::core::ErrorCode::busy,"Tracking model is notifying a committed change");return {};
}
ii::core::Result<void> Owner::can_publish()const{
    if(auto result=ready(true);!result)return result;
    if(revision==std::numeric_limits<std::uint64_t>::max())return ii::core::fail(ii::core::ErrorCode::full,"Tracking revision exhausted");return {};
}
ii::core::Result<std::string> text(std::string_view value){
    if(value.size()>RouteLimits::path_bytes)return ii::core::fail(ii::core::ErrorCode::full,"Tracking text exceeds its UTF-8 byte budget");
    if(value.find('\0')!=value.npos||!ii::core::valid_utf8(value))return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Tracking text requires UTF-8 without NUL");
    return std::string(routing_detail::trim(value));
}
}
