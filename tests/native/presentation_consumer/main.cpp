#include "InstalledRender.generated.h"
#include "InstalledRenderSelection.generated.h"
#include "InstalledPresentation.generated.h"
#include "InstalledScopedPresentation.generated.h"
#include "InstalledProgress.generated.h"
#include "InstalledProgressScoped.generated.h"
#include "InstalledProgressForwarded.generated.h"
#include "InstalledHistory.generated.h"
#include "InstalledHistoryScoped.generated.h"
#include "InstalledRegistry.generated.h"
#include "InstalledRegistryScoped.generated.h"
#include "InstalledRouteRegistry.generated.h"
#include "InstalledHierarchyList.generated.h"
#include "InstalledHierarchyItem.generated.h"
#include "InstalledHierarchyCount.generated.h"
#include "InstalledHierarchyDelegate.generated.h"
#include "InstalledOpacity.generated.h"
#include "InstalledPlatform.generated.h"
#include "InstalledPlatformSelection.generated.h"
#include "InstalledModelSource.generated.h"
#include "InstalledModelSourceSelection.generated.h"
#include "InstalledFont.generated.h"
#include "InstalledFontSelection.generated.h"
#include "InstalledSvg.generated.h"
#include "InstalledSvgSelection.generated.h"
#include <LVRS/ViewModel.h>
#include <LVRS/StateModel.h>
#include <LVRS/ProgressModel.h>
#include <iiSDKCore/Core/Property/ObservableReadScope.h>
#include <iostream>
auto presentation_factory=&lvui_generated::InstalledPresentation::build;
auto scoped_factory=&lvui_generated::InstalledScopedPresentation::build;
auto progress_factory=&lvui_generated::InstalledProgress::build;
auto scoped_progress_factory=&lvui_generated::InstalledProgressScoped::build;
auto forwarded_progress_factory=&lvui_generated::InstalledProgressForwarded::build;
auto history_factory=&lvui_generated::InstalledHistory::build;
auto scoped_history_factory=&lvui_generated::InstalledHistoryScoped::build;
auto registry_factory=&lvui_generated::InstalledRegistry::build;
auto scoped_registry_factory=&lvui_generated::InstalledRegistryScoped::build;
auto route_registry_factory=&lvui_generated::InstalledRouteRegistry::build;
auto hierarchy_list_factory=&lvui_generated::InstalledHierarchyList::build;
auto hierarchy_item_factory=&lvui_generated::InstalledHierarchyItem::build;
auto hierarchy_count_factory=&lvui_generated::InstalledHierarchyCount::build;
auto hierarchy_delegate_factory=&lvui_generated::InstalledHierarchyDelegate::build;
auto opacity_factory=&lvui_generated::InstalledOpacity::build;
auto platform_factory=&lvui_generated::InstalledPlatform::build;
auto platform_selection_factory=&lvui_generated::InstalledPlatformSelection::build;
auto model_source_factory=&lvui_generated::InstalledModelSource::build;
auto model_source_selection_factory=&lvui_generated::InstalledModelSourceSelection::build;
auto font_factory=&lvui_generated::InstalledFont::build;
auto font_selection_factory=&lvui_generated::InstalledFontSelection::build;
auto svg_factory=&lvui_generated::InstalledSvg::build;
auto svg_selection_factory=&lvui_generated::InstalledSvgSelection::build;
auto render_factory=&lvui_generated::InstalledRender::build;
auto render_selection_factory=&lvui_generated::InstalledRenderSelection::build;
int main(){
    if(!render_factory||!render_selection_factory)return 10;
    if(!svg_factory||!svg_selection_factory)return 9;
    auto state=std::make_shared<lvrs::StateModel>();lvrs::ProgressModel progress;
    if(!state->set_key("Installed")||!state->set_value("currentValue",ii::core::Value(75))||!progress.set_state_model(state))return 1;
    std::size_t reads=0;const auto amount=ii::core::ObservableReadScope::collect([&](auto){++reads;},[&]{return progress.progress();});
    if(amount!=.75||reads==0||state->state_snapshot().at("key")!=ii::core::Value("Installed")||!presentation_factory||!scoped_factory||!progress_factory||!scoped_progress_factory||!forwarded_progress_factory)return 2;
    state.reset();if(progress.using_state_model()||progress.progress()!=0)return 3;
    lvrs::ModelUndoStack history;
    if(!history.push_snapshot(ii::core::Value("Installed"))||!history_factory||!scoped_history_factory)return 4;
    auto transfer=history.take_undo_snapshot();if(!transfer||transfer->snapshot!=ii::core::Value("Installed")||transfer->observation_error)return 5;
    lvrs::ViewModelRegistry registry;auto registered=std::make_shared<lvrs::ViewModel>();
    if(!registry.set("Installed",registered)||!registry.bind_view("Editor","Installed",true)||!registry.update_property("Editor","busy",ii::core::Value(true))||!registered->busy()||!registry_factory||!scoped_registry_factory)return 6;
    NativeRouteRegistryBackend routes;if(!routes.router||!route_registry_factory||!routes.registry->views().empty())return 7;
    NativeHierarchyBackend hierarchy;if(!hierarchy_list_factory||!hierarchy_item_factory||!hierarchy_count_factory||!hierarchy_delegate_factory||hierarchy.active()!="root"||!hierarchy.controller->activate_by_key("image"))return 8;
    std::cout<<"Relocated LVRS presentation models and generated factories passed\n";
}
