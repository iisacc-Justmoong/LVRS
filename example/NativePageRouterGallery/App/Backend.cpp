#include "Backend.h"
#include "HomePage.generated.h"
#include "ReportPage.generated.h"
#include <stdexcept>
namespace {
void check(ii::core::Result<bool> value){if(!value)throw std::runtime_error(value.error().message);}
}
PageRouterGalleryBackend::PageRouterGalleryBackend(){
    auto navigation=std::make_shared<std::weak_ptr<lvrs::Navigator>>(navigator);
    auto home=std::make_shared<const lvrs::PageComponent>([navigation,pages=pages](auto& widgets,auto parent,const auto& entry){std::shared_ptr<lvui_generated::HomePage::Scope> owner;auto result=lvui_generated::HomePage::build(widgets,parent,[&](auto& scope){scope.backend_0.navigator=navigation->lock();scope.backend_0.title.set("Home · independent LVUI page");scope.backend_0.context.set("Route: "+entry.path);},&owner);if(result&&owner)pages->home=std::shared_ptr<PageBackend>(owner,&owner->backend_0);return result;});
    auto report=std::make_shared<const lvrs::PageComponent>([navigation](auto& widgets,auto parent,const auto& entry){return lvui_generated::ReportPage::build(widgets,parent,[&](auto& scope){scope.backend_0.navigator=navigation->lock();scope.backend_0.title.set("Report · independent LVUI page");scope.backend_0.context.set("Route: "+entry.path+" · fresh C++ scope");});});
    auto created=lvrs::PageRouter::create({.routes={{"/",std::move(home)},{"/reports/[id]",std::move(report)}},.navigator=navigator});
    if(!created)throw std::runtime_error(created.error().message);router=std::move(*created);
}
void PageRouterGalleryBackend::home(){check(navigator->set_root("/"));}
void PageRouterGalleryBackend::report(){check(navigator->go("/reports/42"));}
void PageRouterGalleryBackend::back(){check(navigator->back());}
void PageRouterGalleryBackend::alternate(){if(auto home=pages->home.lock())home->href.set("/reports/43");}
void PageRouterGalleryBackend::undo_visit(){if(on_undo_visit)on_undo_visit();}
void PageRouterGalleryBackend::close(){if(on_close)on_close();}
