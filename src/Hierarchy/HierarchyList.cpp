#include <LVRS/HierarchyList.h>
#include <LVRS/SurfaceDetail.h>
#include <LVRS/Theme.h>
#include <algorithm>
#include <cmath>
#include <exception>
#include <map>
namespace lvrs {
using namespace ii;
namespace {
struct Failure final:std::exception {
    core::Error error;
    explicit Failure(core::Error value):error(std::move(value)){}
    const char* what()const noexcept override{return error.message.c_str();}
};
template<class T>T take(core::Result<T> r){if(!r)throw Failure{r.error()};return std::move(*r);}
void good(core::Result<void> r){if(!r)throw Failure{r.error()};}
template<class F>auto protect(F f)->decltype(f()){try{return f();}catch(const Failure& e){return std::unexpected(e.error);}catch(...){return core::fail(core::ErrorCode::failed,"Hierarchy list failed",std::current_exception());}}
gui::Rect box(double x,double y,double w,double h){return {float(x),float(y),float(w),float(h)};}
core::Result<void> validate(const HierarchyListOptions& options){
    if(options.model&&options.controller)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy model and controller cannot both be supplied");
    if(!surface_detail::metric(options.width)||!surface_detail::metric(options.height)||options.width<=0||options.height<=0||options.overscan>128||options.maximum_mounted_rows<1||options.maximum_mounted_rows>2048)return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy viewport geometry or admission budget");
    if(auto r=HierarchyItem::validate_options(options.item);!r)return r;
    if(options.row_extent&&(!surface_detail::metric(*options.row_extent)||*options.row_extent<=0))return core::fail(core::ErrorCode::invalid_argument,"Invalid hierarchy default row extent");return {};
}
struct Changing final{bool& value;explicit Changing(bool& v):value(v){value=true;}~Changing(){value=false;}};
}
struct HierarchyListPresentation final:std::enable_shared_from_this<HierarchyListPresentation> {
    struct Entry final {HierarchyItem item;gui::Rect frame;std::optional<HierarchyDelegateContext> context;};
    HierarchyListOptions options;HierarchyListEvents events;HierarchyItemFactory factory;HierarchyDelegateFactory delegate;
    std::shared_ptr<HierarchyController> controller;std::shared_ptr<HierarchyControllerPort> observer;
    gui::Widget root,viewport,rows,indicator;
    std::map<std::string,Entry,std::less<>> entries;
    double width=200,height=400,offset=0,press_x=0,press_y=0,last_x=0,last_y=0;
    std::string press_key;
    bool rendering=false,pending=false,retired=false,drag_started=false,published=false,constructing=true;
    HierarchyItemOptions item_options(std::string_view key){
        const auto index=take(controller->model()->index_of_key(key));auto o=options.item;o.row=take(controller->row(index));
        o.row.has_child_items|=take(controller->model()->relation(index)).child_count>0;o.row.expanded=take(controller->expanded(key));
        const auto state=take(controller->state());o.row.enabled&=options.enabled;o.row.selected=state.active_key==key;o.controller=controller;o.width=std::max(.000001,width);o.row_height=take(controller->row_extent(index));o.drag_preview=state.dragging&&state.drag_key==key;return o;
    }
    void draw_indicator(){
        const auto state=take(controller->state());if(!state.dragging||!state.drag_target){good(indicator.set_drawing({}));return;}
        const auto& plan=*state.drag_target;const auto y=(plan.raw_insertion>=state.count?state.content_extent:take(controller->offset(plan.raw_insertion)))-offset;
        const auto x=std::min(width,options.item.left_padding+plan.depth*options.item.indent_step);const auto ink=surface_detail::linear(Theme::accent_color(Accent::blue));
        gui::WidgetDrawing d;if(y>=0&&y<=height)d.panels.push_back({box(x,std::clamp(y-1.,0.,std::max(0.,height-2)),std::max(0.,width-x),2),ink,ink,0});good(indicator.set_drawing(std::move(d)));
    }
    void render(){
        if(retired)return;if(constructing||rendering){pending=true;return;}rendering=true;struct Reset{bool& b;~Reset(){b=false;}}reset{rendering};if(!rows.snapshot())return;
        for(unsigned round=0;round<8;++round){
            pending=false;const auto state=take(controller->state());
            offset=std::clamp(offset,0.,std::max(0.,state.content_extent-height));good(viewport.set_virtual_scroll_content({width,state.content_extent},0,offset));
            std::vector<std::pair<std::string,gui::Rect>> visible;
            if(state.visible_count&&!state.closed&&!state.faulted&&width>0&&height>0){
                auto index=take(controller->at_offset(std::min(offset,std::max(0.,state.content_extent-.0001))));
                auto rank=take(controller->visible_rank(index));rank=rank>options.overscan?rank-options.overscan:0;
                std::size_t after=0;
                for(;rank<state.visible_count;++rank){auto i=take(controller->at_visible(rank));auto row=take(controller->row(i));const auto y=take(controller->offset(i))-offset,extent=take(controller->row_extent(i));
                    if(y>=height&&after++>=options.overscan)break;
                    if(visible.size()>=options.maximum_mounted_rows)throw Failure{{core::ErrorCode::full,"Hierarchy viewport exceeds its mounted-row budget"}};
                    visible.emplace_back(row.key,box(0,y,width,extent));
                }
                if(state.dragging&&!state.drag_key.empty()&&std::ranges::none_of(visible,[&](const auto& v){return v.first==state.drag_key;})){
                    if(visible.size()>=options.maximum_mounted_rows)throw Failure{{core::ErrorCode::full,"Hierarchy drag pin exceeds its mounted-row budget"}};
                    auto i=take(controller->model()->index_of_key(state.drag_key));const auto extent=take(controller->row_extent(i));visible.emplace_back(state.drag_key,box(0,-extent,width,extent));
                }
            }
            std::vector<gui::KeyedContentItem> cells;cells.reserve(visible.size());auto weak=weak_from_this();
            for(const auto& [key,frame]:visible)cells.push_back({key,[weak,key](gui::WidgetBuilder& widgets)->core::Result<void>{return protect([&]()->core::Result<void>{auto p=weak.lock();if(!p||p->retired)return core::fail(core::ErrorCode::closed,"Hierarchy view retired during row construction");
                HierarchyItemEvents e{.activated=[weak,key]{if(auto p=weak.lock())take(p->controller->activate_by_key(key));},.expansion_changed=[weak,key](bool expanded){if(auto p=weak.lock())take(p->controller->set_expanded(key,expanded));},.pointer=[weak,key](const auto& input)->core::Result<bool>{if(auto p=weak.lock())return p->pointer(key,input);return false;}};
                auto options=p->item_options(key);std::optional<HierarchyDelegateContext> context;HierarchyItem item;
                if(p->factory||p->delegate){
                    context=take(HierarchyDelegateContext::create(options,e));good(widgets.on_retire([context=*context]{(void)context.close();}));
                    if(p->delegate){item=take(p->delegate(widgets,{},"item",*context));auto produced=take(context->item());if(take(produced.root().snapshot()).id!=take(item.root().snapshot()).id)return core::fail(core::ErrorCode::invalid_argument,"Hierarchy delegate returned a different native item");}
                    else{item=take(p->factory(widgets,{},"item",std::move(options),std::move(e)));good(context->adopt_legacy(item));}
                }else item=take(HierarchyItem::create(widgets,{},"item",std::move(options),std::move(e)));
                p->entries.insert_or_assign(key,Entry{item,{},std::move(context)});return {};
            });},frame});
            auto applied=rows.set_keyed_content(std::move(cells),{.viewport=gui::Size{width,height}});
            if(!applied){std::erase_if(entries,[](const auto& e){return !e.second.item.root().snapshot();});throw Failure{applied.error()};}
            for(const auto& [key,frame]:visible){auto found=entries.find(key);if(found==entries.end())throw Failure{{core::ErrorCode::failed,"Retained hierarchy row has no native handle"}};found->second.frame=frame;if(found->second.context)take(found->second.context->update_source(item_options(key)));else good(found->second.item.set_options(item_options(key)));}
            std::erase_if(entries,[&](const auto& e){return std::ranges::none_of(visible,[&](const auto& cell){return cell.first==e.first;});});
            draw_indicator();if(!pending)return;
        }
        throw Failure{{core::ErrorCode::busy,"Hierarchy viewport updates did not settle"}};
    }
    void ensure_active(bool notify=true){
        auto state=take(controller->state());if(state.active_index==hierarchy_npos||state.closed||state.faulted)return;
        const auto y=take(controller->offset(state.active_index)),extent=take(controller->row_extent(state.active_index));
        auto next=offset;if(y<offset)next=y;else if(y+extent>offset+height)next=extent>height?y:y+extent-height;
        good(viewport.set_scroll_offset(next));offset=std::clamp(next,0.,std::max(0.,state.content_extent-height));if(notify&&events.ensure_visible)events.ensure_visible(y,extent);
    }
    void changed(const HierarchyControllerChange& e){
        if(retired)return;if(!published){pending=true;return;}if(e.drag&&!e.state.dragging){drag_started=false;press_key.clear();}if(e.activation)ensure_active();if(retired)return;render();if(retired)return;
        if(e.activation&&options.keyboard_navigation){auto found=entries.find(e.state.active_key);if(found!=entries.end())good(found->second.item.root().focus());}
        auto callbacks=events;
        std::exception_ptr failed;
        auto deliver=[&](auto action){if(retired)return;try{action();}catch(...){if(!failed)failed=std::current_exception();}};
        if(callbacks.expansion_changed)for(const auto& change:e.expansions)deliver([&]{callbacks.expansion_changed(change.key,change.expanded);});
        if(e.activation&&callbacks.active_changed)deliver([&]{callbacks.active_changed(e.state.active_key,e.state.active_id,e.state.active_index);});
        if(e.moved&&callbacks.item_moved)deliver([&]{callbacks.item_moved(*e.moved);});
        if(failed)std::rethrow_exception(failed);
    }
    core::Result<bool> key(const gui::WidgetInput& e){return protect([&]()->core::Result<bool>{
        if(!options.enabled||!options.keyboard_navigation||e.kind!=gui::WidgetInputKind::key_down||e.text_input_active||e.native_navigation_active||e.control||e.meta||e.alt)return false;
        switch(e.key){case gui::WidgetKey::up:take(controller->activate_relative(-1));return true;case gui::WidgetKey::down:take(controller->activate_relative(1));return true;case gui::WidgetKey::left:take(controller->navigate_left());return true;case gui::WidgetKey::right:take(controller->navigate_right());return true;case gui::WidgetKey::escape:if(drag_started){take(controller->end_drag(false));drag_started=false;press_key.clear();return true;}return false;default:return false;}
    });}
    void update_drag(double x,double y){
        const auto state=take(controller->state());if(!state.dragging)return;
        const auto logical=std::clamp(y+offset,0.,state.content_extent);auto raw=state.count;
        if(logical<state.content_extent&&state.visible_count){auto index=take(controller->at_offset(logical));auto top=take(controller->offset(index)),extent=take(controller->row_extent(index));raw=index;
            if(logical>=top+extent/2){auto rank=take(controller->visible_rank(index))+1;raw=rank<state.visible_count?take(controller->at_visible(rank)):state.count;}}
        take(controller->update_drag(raw,x,options.item.indent_step,options.item.left_padding));
    }
    core::Result<bool> pointer(std::string_view key,const gui::WidgetInput& e){return protect([&]()->core::Result<bool>{
        auto entry=entries.find(key);const auto y=e.y+(entry==entries.end()?0:entry->second.frame.y);last_x=e.x;last_y=y;
        if(e.kind==gui::WidgetInputKind::pointer_down){press_key=key;press_x=e.x;press_y=y;drag_started=false;if(e.device!=gui::PointerDevice::touch)take(controller->activate_by_key(key));return false;}
        if(e.kind==gui::WidgetInputKind::pointer_move&&!press_key.empty()){
            if(e.device==gui::PointerDevice::touch)return false;
            if(!drag_started&&std::hypot(e.x-press_x,y-press_y)>=4){drag_started=take(controller->begin_drag(press_key));if(retired){take(controller->end_drag(false));drag_started=false;press_key.clear();return true;}}
            if(drag_started)update_drag(e.x,y);return drag_started;
        }
        if(e.kind==gui::WidgetInputKind::pointer_up){const bool consumed=drag_started;if(consumed){update_drag(e.x,y);take(controller->end_drag(!retired));}drag_started=false;press_key.clear();return consumed;}
        if(e.kind==gui::WidgetInputKind::focus_lost||e.kind==gui::WidgetInputKind::close_popup){const bool consumed=drag_started;drag_started=false;press_key.clear();if(consumed)take(controller->end_drag(false));return consumed;}
        return false;
    });}
    void retire(){
        if(retired)return;retired=true;published=false;observer.reset();
        auto state=controller->state();if(state&&state->dragging&&(drag_started||(!press_key.empty()&&state->drag_key==press_key)))(void)controller->end_drag(false);
        drag_started=false;press_key.clear();entries.clear();
    }
    static core::Result<std::shared_ptr<HierarchyListPresentation>> build(gui::WidgetBuilder& b,HierarchyListOptions options,HierarchyListEvents events,HierarchyItemFactory factory,HierarchyDelegateFactory delegate,std::string seed){
        return protect([&]()->core::Result<std::shared_ptr<HierarchyListPresentation>>{
            auto p=std::make_shared<HierarchyListPresentation>();p->options=std::move(options);p->events=std::move(events);p->factory=std::move(factory);p->delegate=std::move(delegate);p->width=p->options.width;p->height=p->options.height;
            if(p->options.controller){p->controller=p->options.controller;if(p->options.auto_expand_ancestors)take(p->controller->set_auto_expand_ancestors(*p->options.auto_expand_ancestors));if(p->options.editable)take(p->controller->set_editable(*p->options.editable));if(p->options.row_extent)take(p->controller->set_default_row_extent(*p->options.row_extent));}
            else{auto model=p->options.model?p->options.model:take(HierarchyModel::create());p->controller=take(HierarchyController::create(model,{.auto_expand_ancestors=p->options.auto_expand_ancestors.value_or(true),.editable=p->options.editable.value_or(false),.projection={.row_extent=p->options.row_extent.value_or(p->options.item.row_height)},.initial_active_key=std::move(seed)}));}
            const auto weak=std::weak_ptr(p);p->root=take(b.absolute_stack({},"content",{p->width,p->height}));good(p->root.set_clip_content(true));good(p->root.set_enabled(p->options.enabled));
            p->viewport=take(b.scroll_view(p->root,"content.viewport",{.width=p->width,.height=p->height,.virtual_content_height=0}));p->rows=take(b.content_host(p->viewport,"content.rows",{p->width,p->height}));p->indicator=take(b.absolute_stack(p->root,"content.indicator",{p->width,p->height}));
            good(p->root.set_layout_handler([weak](gui::Rect bounds){if(auto p=weak.lock()){p->width=std::max(0.f,bounds.width);p->height=std::max(0.f,bounds.height);good(p->viewport.set_frame(box(0,0,p->width,p->height)));good(p->rows.set_extent({p->width,p->height}));good(p->indicator.set_frame(box(0,0,p->width,p->height)));p->render();}}));
            good(p->viewport.set_scroll_handler([weak](const auto& viewport){if(auto p=weak.lock()){p->offset=viewport.offset;p->width=viewport.bounds.width;p->height=viewport.bounds.height;p->render();}}));
            good(p->root.set_input_handler([weak](const auto& e)->core::Result<bool>{if(auto p=weak.lock())return p->key(e);return false;}));
            good(b.retain(p));good(b.on_retire([weak]{if(auto p=weak.lock())p->retire();}));
            p->observer=std::make_shared<HierarchyControllerPort>([weak](const auto& e){if(auto p=weak.lock())p->changed(e);});good(p->controller->observe(p->observer));
            // Settle geometry/scroll before constructing the first row domain.
            // A staged domain cannot replace an already-built nested activation.
            (void)take(p->root.snapshot());const auto state=take(p->controller->state());good(p->viewport.set_virtual_scroll_content({p->width,state.content_extent},0,0));p->ensure_active(false);p->constructing=false;p->render();return p;
        });
    }
};
struct HierarchyList::Impl final {
    gui::Widget root,content;
    std::shared_ptr<HierarchyListPresentation> current;
    HierarchyListEvents events;HierarchyItemFactory factory;HierarchyDelegateFactory delegate;
    core::Observable<std::uint64_t> source_epoch{0};std::uint64_t source_revision=0;
    bool changing=false,retired=false;
    core::Result<void> replace(HierarchyListOptions options){
        if(changing)return core::fail(core::ErrorCode::busy,"Hierarchy source replacement is already in progress");Changing guard{changing};
        if(source_revision==UINT64_MAX)return core::fail(core::ErrorCode::full,"Hierarchy source revision exhausted");
        return protect([&]()->core::Result<void>{
            const auto seed=current?take(current->controller->state()).active_key:std::string{};
            std::shared_ptr<HierarchyListPresentation> candidate;
            auto applied=content.set_content([&](gui::WidgetBuilder& b)->core::Result<void>{auto result=HierarchyListPresentation::build(b,options,events,factory,delegate,seed);if(!result)return std::unexpected(result.error());candidate=std::move(*result);return {};},[&]()->core::Result<void>{
                if(retired)return core::fail(core::ErrorCode::closed,"Hierarchy list retired before source publication");
                current=candidate;candidate->published=true;return {};
            });
            if(!applied)return applied;
            good(root.set_extent({options.width,options.height}));good(root.set_enabled(options.enabled));source_epoch.set(++source_revision);return {};
        });
    }
};
core::Result<HierarchyList> HierarchyList::create(gui::WidgetBuilder& b,gui::Widget parent,std::string_view id,HierarchyListOptions options,HierarchyListEvents events,HierarchyItemFactory factory,HierarchyDelegateFactory delegate){
    if(auto r=validate(options);!r)return std::unexpected(r.error());
    if(factory&&delegate)return core::fail(core::ErrorCode::invalid_argument,"Use one hierarchy item factory");
    return protect([&]()->core::Result<HierarchyList>{auto p=std::make_shared<Impl>();p->events=std::move(events);p->factory=std::move(factory);p->delegate=std::move(delegate);
        p->root=take(b.absolute_stack(parent,id,{options.width,options.height}));good(p->root.set_clip_content(true));
        p->content=take(b.content_host(p->root,std::string(id)+".source",{options.width,options.height}));const auto weak=std::weak_ptr(p);
        good(p->root.set_layout_handler([weak](gui::Rect bounds){if(auto p=weak.lock()){good(p->content.set_frame(box(0,0,bounds.width,bounds.height)));if(p->current)good(p->current->root.set_extent({bounds.width,bounds.height}));}}));
        good(p->root.set_input_handler([weak](const auto& e)->core::Result<bool>{if(auto p=weak.lock();p&&p->current)return p->current->key(e);return false;}));
        good(b.retain(p));good(b.on_retire([weak]{if(auto p=weak.lock()){p->retired=true;p->current.reset();}}));good(p->replace(std::move(options)));
        HierarchyList result;result.root_=p->root;result.state_=p;return result;
    });
}
core::Result<std::shared_ptr<HierarchyList::Impl>> HierarchyList::lock()const{return core::ObservableReadScope::pause([&]()->core::Result<std::shared_ptr<Impl>>{if(auto r=root_.snapshot();!r)return std::unexpected(r.error());auto p=state_.lock();if(!p||p->retired)return core::fail(core::ErrorCode::closed,"Hierarchy list retired");return p;});}
core::Result<HierarchyListState> HierarchyList::state()const{auto live=lock();if(!live)return std::unexpected(live.error());(void)(*live)->source_epoch.get();auto p=(*live)->current;return protect([&]()->core::Result<HierarchyListState>{HierarchyListState value{take(p->controller->state()),p->offset,p->height,p->entries.size(),{}};for(const auto& key:take(p->rows.keyed_content_keys()))value.mounted_keys.push_back(key);return value;});}
core::Result<void> HierarchyList::set_options(HierarchyListOptions options)const{
    auto live=lock();if(!live)return std::unexpected(live.error());auto owner=*live;if(owner->changing)return core::fail(core::ErrorCode::busy,"Hierarchy configuration is already in progress");auto p=owner->current;
    if(auto r=validate(options);!r)return r;
    const bool same=(options.model==p->options.model&&options.controller==p->options.controller)||(options.controller&&options.controller==p->controller);
    if(!same)return owner->replace(std::move(options));Changing guard{owner->changing};
    return protect([&]()->core::Result<void>{
        const bool height=options.item.row_height!=p->options.item.row_height||options.row_extent!=p->options.row_extent,editable=options.editable!=p->options.editable,automatic=options.auto_expand_ancestors!=p->options.auto_expand_ancestors;
        p->options=std::move(options);
        if(height)take(p->controller->set_default_row_extent(p->options.row_extent.value_or(p->options.item.row_height)));
        if(editable&&p->options.editable)take(p->controller->set_editable(*p->options.editable));if(automatic&&p->options.auto_expand_ancestors)take(p->controller->set_auto_expand_ancestors(*p->options.auto_expand_ancestors));
        good(owner->root.set_extent({p->options.width,p->options.height}));good(owner->root.set_enabled(p->options.enabled));good(p->root.set_enabled(p->options.enabled));p->render();return {};
    });
}
core::Result<std::shared_ptr<HierarchyController>> HierarchyList::controller()const{auto p=lock();if(!p)return std::unexpected(p.error());(void)(*p)->source_epoch.get();return (*p)->current->controller;}
core::Result<HierarchyItem> HierarchyList::mounted_item(std::string_view key)const{auto live=lock();if(!live)return std::unexpected(live.error());auto p=(*live)->current;auto found=p->entries.find(key);if(found==p->entries.end())return core::fail(core::ErrorCode::not_found,"Hierarchy row is outside the retained viewport");return found->second.item;}
core::Result<void> HierarchyList::set_scroll_offset(double offset)const{auto live=lock();if(!live)return std::unexpected(live.error());auto p=(*live)->current;return protect([&]()->core::Result<void>{good(p->viewport.set_scroll_offset(offset));p->render();return {};});}
core::Result<void> HierarchyList::ensure_active_visible()const{auto live=lock();if(!live)return std::unexpected(live.error());auto p=(*live)->current;return protect([&]()->core::Result<void>{p->ensure_active();return {};});}
core::Result<void> HierarchyList::set_enabled(bool enabled)const{auto live=lock();if(!live)return std::unexpected(live.error());auto p=(*live)->current;return protect([&]()->core::Result<void>{p->options.enabled=enabled;good(root_.set_enabled(enabled));good(p->root.set_enabled(enabled));p->render();return {};});}
}
