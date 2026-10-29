#include "HierarchyController.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iiSDKCore/Core/Property/ObservableReadScope.h>
#include <thread>
namespace lvrs {
using namespace ii;
namespace {
struct Failure final{core::Error error;};
template<class T>T take(core::Result<T> r){if(!r)throw Failure{r.error()};return std::move(*r);}
void good(core::Result<void> r){if(!r)throw Failure{r.error()};}
struct Guard final{bool& flag;explicit Guard(bool& b):flag(b){flag=true;}~Guard(){flag=false;}};
template<class F>auto protect(F f)->decltype(f()){try{return f();}catch(const Failure& e){return std::unexpected(e.error);}catch(const std::bad_alloc&){return core::fail(core::ErrorCode::full,"Hierarchy controller allocation failed");}catch(...){return core::fail(core::ErrorCode::failed,"Hierarchy controller failed",std::current_exception());}}
}
struct HierarchyController::Impl final {
    std::thread::id owner=std::this_thread::get_id();
    std::shared_ptr<HierarchyModel> model;
    std::shared_ptr<HierarchyProjection> projection;
    HierarchyControllerOptions options;
    HierarchyControllerState value;
    HierarchyControllerChange pending;
    core::Observable<std::uint64_t> epoch{0};
    std::shared_ptr<HierarchyProjectionPort> projection_port;
    std::shared_ptr<HierarchyChangePort> model_port;
    std::vector<std::weak_ptr<const HierarchyControllerPort>> ports;
    bool changing=false,notifying=false,dirty=false,cancel_requested=false;
    unsigned publication_depth=0;
    std::uint64_t drag_revision=0;
    core::Result<void> ready(bool write=false)const{
        if(owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Hierarchy controller requires its owner thread");
        if(value.closed)return core::fail(core::ErrorCode::closed,"Hierarchy controller is closed");
        if(write&&(changing||notifying))return core::fail(core::ErrorCode::busy,"Hierarchy controller is committing or notifying a command");
        auto source=model->state();if(!source)return std::unexpected(source.error());auto projected=projection->state();if(!projected)return std::unexpected(projected.error());
        if(source->revision!=projected->model_revision||source->revision!=value.model_revision)return core::fail(core::ErrorCode::busy,"Hierarchy controller has not received this source revision");
        if(projected->faulted)return core::fail(core::ErrorCode::failed,"Hierarchy projection is faulted");return {};
    }
    bool assign_active(std::size_t index,bool emit,bool force=false){
        std::string key;std::int64_t id=-1;
        if(index!=hierarchy_npos){const auto& row=take(model->row(index)).get();key=row.key;id=row.item_id>=0?row.item_id:std::int64_t(index);}
        const bool changed=key!=value.active_key||id!=value.active_id||index!=value.active_index;
        if(!changed&&!force)return false;
        value.active_key=std::move(key);value.active_id=id;value.active_index=index;dirty=true;
        if(emit){pending.activation=true;if(!pending.expansion)pending.key=value.active_key;}return true;
    }
    bool activate(std::size_t index,bool force=false){
        if(index==hierarchy_npos)return false;
        const auto& row=take(model->row(index)).get();if(!row.enabled||!row.activatable)return false;
        if(options.auto_expand_ancestors)expand_ancestors(index);
        if(!take(projection->visible(index)))return false;
        return assign_active(index,true,force);
    }
    std::size_t find(std::string_view key)const{const auto first=key.find_first_not_of(" \t\r\n");if(first==key.npos)return hierarchy_npos;key=key.substr(first,key.find_last_not_of(" \t\r\n")-first+1);auto index=model->index_of_key(key);if(index)return *index;if(index.error().code==core::ErrorCode::not_found)return hierarchy_npos;throw Failure{index.error()};}
    void metrics(){
        const auto source=take(model->state());const auto projected=take(projection->state());
        value.model_revision=source.revision;value.count=projected.count;value.visible_count=projected.visible_count;value.eligible_count=projected.eligible_count;value.content_extent=projected.content_extent;value.faulted=projected.faulted;value.editable=options.editable&&source.editable;
        if(projected.closed||source.closed){value.closed=true;value.count=value.visible_count=value.eligible_count=0;value.content_extent=0;assign_active(hierarchy_npos,false);}
        if(value.dragging&&(value.model_revision!=drag_revision||!value.editable||value.closed)){value.dragging=false;value.drag_key.clear();value.drag_target.reset();pending.drag=true;dirty=true;}
    }
    void normalize(){
        metrics();if(value.closed||value.faulted)return;
        auto current=find(value.active_key);
        if(current!=hierarchy_npos){const auto& row=take(model->row(current)).get();if(row.enabled&&row.activatable){if(options.auto_expand_ancestors)expand_ancestors(current);if(take(projection->visible(current))){assign_active(current,false);metrics();return;}}}
        for(const auto& row:take(model->rows()))if(row.selected&&row.enabled&&row.activatable){auto index=find(row.key);if(options.auto_expand_ancestors)expand_ancestors(index);if(take(projection->visible(index))){assign_active(index,false);metrics();return;}}
        const auto state=take(projection->state());assign_active(state.eligible_count?take(projection->at_eligible(0)):hierarchy_npos,false);metrics();
    }
    core::Result<void> publish(){
        if(!dirty)return {};if(publication_depth>=32)return core::fail(core::ErrorCode::full,"Hierarchy source notification cycle exceeded its bound");
        ++publication_depth;struct Reset{unsigned& value;~Reset(){--value;}}reset{publication_depth};dirty=false;++value.revision;pending.state=value;
        auto event=std::move(pending);pending={};std::optional<core::Error> failed;
        {
        Guard guard{notifying};
        try{epoch.set(value.revision);}catch(...){failed=core::Error{core::ErrorCode::failed,"Hierarchy state observation failed after commit",std::current_exception()};}
        std::erase_if(ports,[](const auto& p){return p.expired();});const auto targets=ports;
        for(const auto& weak:targets)if(auto port=weak.lock()){auto sent=port->send(event);if(!sent&&sent.error().code!=runtime::QueryErrorCode::receiver_expired&&!failed)failed=core::Error{core::ErrorCode::failed,"Hierarchy controller receiver failed after commit",sent.error().cause};}
        }
        if(cancel_requested){cancel_requested=false;if(value.dragging){value.dragging=false;value.drag_key.clear();value.drag_target.reset();dirty=pending.drag=true;}}
        if(dirty){
            {Guard guard{changing};normalize();}
            if(value.closed){projection_port.reset();model_port.reset();projection.reset();}
            auto next=publish();if(!next&&!failed)failed=next.error();
        }
        if(value.closed)ports.clear();if(failed)return std::unexpected(*failed);return {};
    }
    template<class F>core::Result<bool> command(F action){
        if(auto r=ready(true);!r)return std::unexpected(r.error());
        return core::ObservableReadScope::pause([&]()->core::Result<bool>{
            core::Result<bool> result=false;
            {Guard guard{changing};result=protect([&]()->core::Result<bool>{auto result=action();normalize();return result;});}
            auto sent=protect([&](){return publish();});if(!result)return result;if(!sent)return std::unexpected(sent.error());return result;
        });
    }
    void source_changed(const HierarchyChange& change){
        dirty=true;pending.data=true;pending.structure|=change.kind==HierarchyChangeKind::reset||change.kind==HierarchyChangeKind::move||change.kind==HierarchyChangeKind::closed;
        if(changing||notifying)return;
        auto result=protect([&]()->core::Result<void>{{Guard guard{changing};normalize();}if(value.closed){projection_port.reset();model_port.reset();projection.reset();}return publish();});
        if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}
    }
    bool expand(std::size_t index,bool expanded,bool interactive=true){
        if(index==hierarchy_npos)return false;const auto row=take(model->row(index)).get();auto relation=take(model->relation(index));
        if((interactive&&!row.enabled)||!row.show_chevron||!(row.has_child_items||relation.child_count))return false;
        if(take(projection->expanded(index))==expanded)return false;
        HierarchyExpansionChange event{row.key,index,expanded};
        if(pending.expansions.size()==pending.expansions.capacity())pending.expansions.reserve(std::max(std::size_t(8),pending.expansions.size()*2));
        const bool changed=take(projection->set_expanded(index,expanded));if(!changed)return false;
        pending.expansions.push_back(std::move(event));
        pending.expansion=true;pending.key=row.key;pending.expanded=expanded;dirty=true;
        if(!expanded&&value.active_index>index&&value.active_index<relation.subtree_end){
            if(!activate(index))assign_active(hierarchy_npos,false);
        }
        return true;
    }
    void expand_ancestors(std::size_t index){
        // QML walks from the nearest parent towards the root. Keep that
        // transition order while publishing one settled controller snapshot.
        for(auto parent=take(model->relation(index)).parent;parent!=hierarchy_npos;parent=take(model->relation(parent)).parent)expand(parent,true,false);
    }
    bool movable(std::size_t index)const{if(index==hierarchy_npos||!value.editable)return false;const auto& row=take(model->row(index)).get();return row.enabled&&row.draggable;}
    core::Result<bool> commit(const HierarchyMovePlan& plan){
        const auto revision=take(model->state()).revision;auto result=model->commit_move(plan);
        if(take(model->state()).revision!=revision){pending.moved=plan;pending.structure=true;dirty=true;}return result;
    }
};
HierarchyController::HierarchyController(std::shared_ptr<Impl> p):impl_(std::move(p)){}
HierarchyController::~HierarchyController(){impl_->projection_port.reset();impl_->model_port.reset();}
core::Result<std::shared_ptr<HierarchyController>> HierarchyController::create(std::shared_ptr<HierarchyModel> model,HierarchyControllerOptions options){
    if(!model)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy controller requires a model");
    return protect([&]()->core::Result<std::shared_ptr<HierarchyController>>{
        auto p=std::make_shared<Impl>();p->model=std::move(model);p->options=options;p->projection=take(HierarchyProjection::create(p->model,options.projection));
        const auto weak=std::weak_ptr(p);
        p->projection_port=std::make_shared<HierarchyProjectionPort>([weak](const auto& e){if(auto p=weak.lock()){p->dirty=true;p->pending.data=true;p->pending.structure|=e.structure;}});
        good(p->projection->observe(p->projection_port));
        // Projection's source observer was registered first. This second phase
        // can normalize selection/ancestors after the projection's read-only
        // notification has returned, without another event loop or source write.
        p->model_port=std::make_shared<HierarchyChangePort>([weak](const auto& e){if(auto p=weak.lock())p->source_changed(e);});good(p->model->observe(p->model_port));
        p->value.active_key=std::move(options.initial_active_key);
        {Guard guard{p->changing};p->normalize();}p->dirty=false;p->pending={};return std::shared_ptr<HierarchyController>(new HierarchyController(std::move(p)));
    });
}
core::Result<HierarchyControllerState> HierarchyController::state()const{auto p=impl_;if(p->owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Hierarchy controller requires its owner thread");if(!p->value.closed){auto source=p->model->state();auto projection=p->projection->state();if(!source)return std::unexpected(source.error());if(!projection)return std::unexpected(projection.error());if(source->revision!=projection->model_revision||source->revision!=p->value.model_revision)return core::fail(core::ErrorCode::busy,"Hierarchy controller has not received this source revision");}(void)p->epoch.get();return p->value;}
std::shared_ptr<HierarchyModel> HierarchyController::model()const{auto p=impl_;if(p->owner!=std::this_thread::get_id())throw std::logic_error("Hierarchy model access requires its owner thread");return p->model;}
core::Result<HierarchyRow> HierarchyController::row(std::size_t index)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());(void)p->epoch.get();auto row=p->model->row(index);if(!row)return std::unexpected(row.error());return row->get();}
core::Result<HierarchyMetadata> HierarchyController::metadata(std::string_view key)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return protect([&](){(void)p->epoch.get();return p->projection->metadata(p->find(key));});}
core::Result<bool> HierarchyController::expanded(std::string_view key)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return protect([&](){(void)p->epoch.get();return p->projection->expanded(p->find(key));});}
core::Result<std::size_t> HierarchyController::at_visible(std::size_t rank)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->projection->at_visible(rank);}
core::Result<std::size_t> HierarchyController::visible_rank(std::size_t index)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->projection->visible_rank(index);}
core::Result<std::size_t> HierarchyController::at_offset(double y)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->projection->at_offset(y);}
core::Result<double> HierarchyController::offset(std::size_t index)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->projection->offset(index);}
core::Result<double> HierarchyController::row_extent(std::size_t index)const{auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->projection->row_extent(index);}
core::Result<bool> HierarchyController::activate_by_key(std::string_view key,bool force){auto p=impl_;return p->command([&]()->core::Result<bool>{return p->activate(p->find(key),force);});}
core::Result<bool> HierarchyController::activate_by_id(std::int64_t id,bool force){auto p=impl_;return p->command([&]()->core::Result<bool>{auto i=p->model->index_of_id(id);if(!i&&i.error().code==core::ErrorCode::not_found)return false;return p->activate(take(std::move(i)),force);});}
core::Result<bool> HierarchyController::activate_relative(int step){auto p=impl_;return p->command([&]()->core::Result<bool>{const auto state=take(p->projection->state());if(!step||!state.eligible_count)return false;auto rank=p->projection->eligible_rank(p->value.active_index);const auto current=rank?std::int64_t(*rank):step>0?-1:std::int64_t(state.eligible_count);const auto target=std::clamp(current+step,std::int64_t(0),std::int64_t(state.eligible_count-1));return p->activate(take(p->projection->at_eligible(std::size_t(target))));});}
core::Result<bool> HierarchyController::navigate_left(){auto p=impl_;return p->command([&]()->core::Result<bool>{auto i=p->find(p->value.active_key);if(i==hierarchy_npos)return false;if(take(p->projection->expanded(i))&&p->expand(i,false))return true;auto parent=take(p->model->relation(i)).parent;return parent!=hierarchy_npos&&p->activate(parent);});}
core::Result<bool> HierarchyController::navigate_right(){auto p=impl_;return p->command([&]()->core::Result<bool>{auto i=p->find(p->value.active_key);if(i==hierarchy_npos)return false;if(!take(p->projection->expanded(i))&&p->expand(i,true))return true;for(auto child=take(p->model->relation(i)).first_child;child!=hierarchy_npos;child=take(p->model->relation(child)).next_sibling)if(p->activate(child))return true;return false;});}
core::Result<bool> HierarchyController::set_expanded(std::string_view key,bool value){auto p=impl_;return p->command([&]()->core::Result<bool>{return p->expand(p->find(key),value);});}
core::Result<bool> HierarchyController::toggle_expanded(std::string_view key){auto p=impl_;return p->command([&]()->core::Result<bool>{auto i=p->find(key);return i!=hierarchy_npos&&p->expand(i,!take(p->projection->expanded(i)));});}
core::Result<bool> HierarchyController::expand_all(){auto p=impl_;return p->command([&]()->core::Result<bool>{bool changed=false;const auto count=take(p->model->state()).count;for(std::size_t i=0;i<count;++i)changed=p->expand(i,true,false)||changed;return changed;});}
core::Result<bool> HierarchyController::collapse_all(bool root){auto p=impl_;return p->command([&]()->core::Result<bool>{bool changed=false;const auto count=take(p->model->state()).count;for(std::size_t i=0;i<count;++i)changed=p->expand(i,root&&take(p->model->row(i)).get().depth==0,false)||changed;return changed;});}
core::Result<bool> HierarchyController::set_row_extent(std::string_view key,double extent){auto p=impl_;return p->command([&](){return p->projection->set_row_extent(p->find(key),extent);});}
core::Result<bool> HierarchyController::set_default_row_extent(double extent){auto p=impl_;return p->command([&](){return p->projection->set_default_row_extent(extent);});}
core::Result<bool> HierarchyController::set_auto_expand_ancestors(bool value){auto p=impl_;return p->command([&]()->core::Result<bool>{if(p->options.auto_expand_ancestors==value)return false;p->options.auto_expand_ancestors=value;p->dirty=p->pending.data=true;return true;});}
core::Result<bool> HierarchyController::set_editable(bool value){auto p=impl_;return p->command([&]()->core::Result<bool>{if(p->options.editable==value)return false;p->options.editable=value;p->dirty=true;p->pending.data=true;return true;});}
core::Result<bool> HierarchyController::begin_drag(std::string_view key){auto p=impl_;return p->command([&]()->core::Result<bool>{auto i=p->find(key);if(!p->movable(i))return false;const auto resolved=take(p->model->row(i)).get().key;if(p->value.dragging&&p->value.drag_key==resolved)return false;p->value.drag_key=resolved;p->value.dragging=true;p->value.drag_target.reset();p->drag_revision=take(p->model->state()).revision;p->dirty=p->pending.drag=true;return true;});}
core::Result<bool> HierarchyController::update_drag(std::size_t raw,double x,double indent,double padding){auto p=impl_;return p->command([&]()->core::Result<bool>{if(!p->value.dragging)return false;auto plan=p->model->resolve_drag_target(p->value.drag_key,raw,x,indent,padding);if(!plan){if(p->value.drag_target){p->value.drag_target.reset();p->dirty=p->pending.drag=true;}if(plan.error().code==core::ErrorCode::invalid_argument||plan.error().code==core::ErrorCode::out_of_range)return false;return std::unexpected(plan.error());}if(p->value.drag_target==*plan)return false;p->value.drag_target=std::move(*plan);p->dirty=p->pending.drag=true;return true;});}
core::Result<bool> HierarchyController::end_drag(bool commit){auto p=impl_;if(!commit){if(p->owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Hierarchy controller requires its owner thread");if(p->value.closed)return false;if(p->changing||p->notifying){p->cancel_requested=p->value.dragging;return p->value.dragging;}}return p->command([&]()->core::Result<bool>{if(!p->value.dragging)return false;auto plan=p->value.drag_target;p->value.dragging=false;p->value.drag_key.clear();p->value.drag_target.reset();p->dirty=p->pending.drag=true;if(commit&&plan)return p->commit(*plan);return true;});}
core::Result<bool> HierarchyController::move(std::string_view key,std::size_t raw,std::uint32_t depth){auto p=impl_;return p->command([&]()->core::Result<bool>{if(!p->movable(p->find(key)))return false;return p->commit(take(p->model->plan_move(key,raw,depth)));});}
core::Result<void> HierarchyController::observe(std::shared_ptr<const HierarchyControllerPort> port){auto p=impl_;if(auto r=p->ready(true);!r)return r;if(!port)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy controller observation requires a receiver");std::erase_if(p->ports,[](const auto& p){return p.expired();});for(const auto& weak:p->ports)if(weak.lock()==port)return {};if(p->ports.size()>=4096)return core::fail(core::ErrorCode::full,"Too many hierarchy controller observers");p->ports.push_back(std::move(port));return {};}
core::Result<void> HierarchyController::close(){auto p=impl_;if(p->owner!=std::this_thread::get_id())return core::fail(core::ErrorCode::wrong_executor,"Hierarchy controller requires its owner thread");if(p->value.closed)return {};if(p->changing||p->notifying)return core::fail(core::ErrorCode::busy,"Hierarchy controller is committing or notifying a command");p->value.closed=true;p->value.count=p->value.visible_count=p->value.eligible_count=0;p->value.content_extent=0;p->value.dragging=false;p->value.drag_key.clear();p->value.drag_target.reset();p->assign_active(hierarchy_npos,false);p->dirty=p->pending.structure=true;p->projection_port.reset();p->model_port.reset();p->projection.reset();return protect([&](){return p->publish();});}
}
