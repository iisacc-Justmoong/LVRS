#include "NavigationTarget.h"
namespace lvrs {
NavigationTarget::~NavigationTarget()=default;
ii::core::Result<NavigationPlacement> NavigationTarget::placement()const{return NavigationPlacement{};}
ii::core::Result<std::vector<NavigationViewEntry>> NavigationTarget::view_tracking_entries()const{return std::vector<NavigationViewEntry>{};}
}
