#include "ViewModelRegistry.h"
#include "PresentationDetail.h"
#include <iiSDKCore/Core/Text/Number.h>
#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <span>

namespace lvrs {
using ii::core::Result;using ii::core::Value;
namespace pd=presentation_detail;
namespace {
struct Failure final {ii::core::Error error;};
std::string scalar_text(const Value& value){
    return std::visit([](const auto& item)->std::string{
        using T=std::decay_t<decltype(item)>;
        if constexpr(std::same_as<T,std::string>)return item;
        else if constexpr(std::same_as<T,bool>)return item?"true":"false";
        else if constexpr(std::integral<T>)return std::to_string(item);
        else if constexpr(std::floating_point<T>)return ii::core::format_decimal(item);
        else return {};
    },value.storage());
}
bool scalar_bool(const Value& value){
    return std::visit([](const auto& item)->bool{
        using T=std::decay_t<decltype(item)>;
        if constexpr(std::is_arithmetic_v<T>)return item!=0;
        else if constexpr(std::same_as<T,std::string>){auto lower=item;for(auto& c:lower)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return !lower.empty()&&lower!="0"&&lower!="false";}
        else return false;
    },value.storage());
}
std::string first_token(const Value::Object& values,std::initializer_list<std::string_view> names){
    for(auto name:names)if(auto found=values.find(name);found!=values.end()){auto token=pd::normalized_text(scalar_text(found->second));if(!token.empty())return token;}
    return {};
}
std::optional<bool> first_bool(const Value::Object& values){
    for(auto name:{"writable","modelWritable"})if(auto found=values.find(name);found!=values.end()&&found->second.kind()!=ii::core::ValueKind::null)return scalar_bool(found->second);
    return {};
}
template<class Map>std::vector<std::string> names(const Map& values){Value::Object keys;for(const auto& [key,value]:values){(void)value;keys.emplace(key,Value{});}return pd::ordered_keys(keys);}
}

struct ViewModelRegistry::Impl final:pd::Signals<ViewModelRegistryChange>,std::enable_shared_from_this<Impl> {
    struct Link final {
        std::weak_ptr<ModelProperties> model;
        std::shared_ptr<ModelProperties> retained;
        ModelProperties* identity=nullptr;
        ii::core::ObservableSubscription watch;
        ii::core::Observable<std::uint64_t> epoch{0};
        std::set<std::string,std::less<>> aliases;
        bool active=false,pending=false;
    };
    struct Entry final {std::shared_ptr<Link> link;std::string owner;std::set<std::string,std::less<>> views;};
    struct Removed final {std::shared_ptr<Link> link;bool views=false,owner=false;};
    std::map<std::string,Entry,std::less<>> entries;
    std::map<std::string,std::string,std::less<>> view_keys;
    std::map<ModelProperties*,std::weak_ptr<Link>> links;
    std::vector<std::shared_ptr<Link>> pending;
    std::set<std::string,std::less<>> retiring_views;
    ii::core::Observable<std::uint64_t> keys_epoch{0},views_epoch{0},owners_epoch{0},descriptors_epoch{0};
    ii::core::Observable<std::string> error{""};
    ViewModelRegistryLimits limits;
    std::optional<ii::core::Error> failure;
    bool draining=false;
    explicit Impl(ViewModelRegistryLimits value):limits(value){}
    void ensure_open()const{if(closed)throw Failure{{ii::core::ErrorCode::closed,"Registry owner retired before commit"}};}

    std::string token(std::string_view value)const{
        auto result=pd::normalized_text(value);
        if(result.size()>limits.token_bytes)throw Failure{{ii::core::ErrorCode::full,"Registry token exceeds its byte limit"}};
        return result;
    }
    void remember(ii::core::Error value){if(!failure)failure=std::move(value);}
    void emit(std::span<const ViewModelRegistryEvent> events){
        pd::Flag guard{notifying};
        for(auto event:events){
            if(closed)break;
            if(event!=ViewModelRegistryEvent::last_error){
                auto& epoch=event==ViewModelRegistryEvent::keys?keys_epoch:event==ViewModelRegistryEvent::views?views_epoch:event==ViewModelRegistryEvent::ownership?owners_epoch:descriptors_epoch;
                tick(epoch,failure);
            }
            send({event},failure);
        }
    }
    void emit(std::initializer_list<ViewModelRegistryEvent> events){emit(std::span(events.begin(),events.size()));}
    void set_error(std::string value){
        if(error.get()==value)return;
        const auto revision=error.revision();
        try{error.set(std::move(value));}
        catch(...){ii::core::Error cause{ii::core::ErrorCode::failed,"Registry error observation failed",std::current_exception()};remember(cause);if(error.revision()==revision)throw Failure{std::move(cause)};}
        emit({ViewModelRegistryEvent::last_error});
    }
    bool reject(std::string message){set_error(std::move(message));return false;}
    void retire(const std::shared_ptr<Link>& link){
        if(!link->aliases.empty())return;
        link->active=false;link->watch={};links.erase(link->identity);
    }
    Removed erase_entry(std::string_view key){
        auto found=entries.find(key);if(found==entries.end())return {};
        auto& entry=found->second;Removed result{entry.link,!entry.views.empty(),!entry.owner.empty()};
        for(const auto& view:entry.views)view_keys.erase(view);
        entry.link->aliases.erase(found->first);entries.erase(found);retire(result.link);return result;
    }
    void removed_events(bool had_keys,bool had_views,bool had_owner){
        if(had_keys)emit({ViewModelRegistryEvent::keys,ViewModelRegistryEvent::descriptors});
        if(had_views)emit({ViewModelRegistryEvent::views});
        if(had_owner)emit({ViewModelRegistryEvent::ownership});
        if(had_views||had_owner)emit({ViewModelRegistryEvent::descriptors});
    }
    void process_source(const std::shared_ptr<Link>& link){
        if(!link->active)return;
        if(link->model.expired()){
            link->active=false;link->watch={};links.erase(link->identity);
            auto aliases=std::move(link->aliases);bool had_views=false,had_owner=false;
            for(const auto& key:aliases){auto result=erase_entry(key);had_views|=result.views;had_owner|=result.owner;}
            removed_events(!aliases.empty(),had_views,had_owner);
        }else{
            tick(link->epoch,failure);emit({ViewModelRegistryEvent::descriptors});
        }
    }
    void queue(const std::shared_ptr<Link>& link){if(!link->pending){pending.push_back(link);link->pending=true;}}
    bool unbind(std::string_view view){
        auto bound=view_keys.find(view);if(bound==view_keys.end())return false;
        auto entry=entries.find(bound->second);bool owned=false;
        if(entry!=entries.end()){entry->second.views.erase(bound->first);if(entry->second.owner==view){entry->second.owner.clear();owned=true;}}
        view_keys.erase(bound);emit({ViewModelRegistryEvent::views});if(owned)emit({ViewModelRegistryEvent::ownership});emit({ViewModelRegistryEvent::descriptors});return true;
    }
    void drain(){
        if(draining||(pending.empty()&&retiring_views.empty())||closed)return;
        pd::Flag guard{draining};
        for(std::size_t round=0;(!pending.empty()||!retiring_views.empty())&&!closed;++round){
            // Reentrant cleanup only removes existing bindings, so it cannot
            // replenish itself indefinitely while ordinary mutations are busy.
            auto retired=std::move(retiring_views);retiring_views.clear();
            {pd::Flag writing{changing};for(const auto& view:retired)if(!closed)unbind(view);}
            if(round>=limits.pending_rounds&&!pending.empty()){for(auto& link:pending)link->pending=false;pending.clear();remember({ii::core::ErrorCode::full,"Registry source notification cycle exceeded its bound"});}
            const auto count=pending.size();for(std::size_t i=0;i<count;++i)pending[i]->pending=false;
            {pd::Flag writing{changing};for(std::size_t i=0;i<count&&!closed;++i)process_source(pending[i]);}
            if(closed)break;
            pending.erase(pending.begin(),pending.begin()+std::ptrdiff_t(count));
        }
    }
    template<class F>Result<bool> run(F action,bool clear_error=true){
        if(auto ready=writable();!ready)return std::unexpected(ready.error());
        return ii::core::ObservableReadScope::pause([&]()->Result<bool>{
            failure.reset();Result<bool> result=false;
            {pd::Flag guard{changing};try{if(clear_error)set_error("");ensure_open();result=action();}catch(const Failure& e){result=std::unexpected(e.error);}catch(...){result=ii::core::fail(ii::core::ErrorCode::failed,"Registry operation failed",std::current_exception());}}
            try{drain();}catch(...){remember({ii::core::ErrorCode::failed,"Registry source delivery failed",std::current_exception()});}
            if(!result)return result;if(failure)return std::unexpected(*failure);return result;
        });
    }
    void source_changed(const std::weak_ptr<Link>& weak){
        auto link=weak.lock();if(!link||!link->active||closed)return;
        if(changing||notifying||draining){queue(link);return;}
        auto result=run([&]{process_source(link);return true;},false);
        if(!result){if(result.error().cause)std::rethrow_exception(result.error().cause);throw std::runtime_error(result.error().message);}
    }
    std::shared_ptr<Link> prepare_link(const std::shared_ptr<ModelProperties>& model){
        if(auto found=links.find(model.get());found!=links.end())if(auto link=found->second.lock();link&&link->active)return link;
        pending.reserve(2*(links.size()+1));
        auto link=std::make_shared<Link>();link->model=model;link->identity=model.get();
        link->watch=model->observe_descriptor([weak=weak_from_this(),source=std::weak_ptr(link)]{if(auto registry=weak.lock())registry->source_changed(source);});
        return link;
    }
    bool put(std::string key,const std::shared_ptr<ModelProperties>& model,ModelRetention retention){
        key=token(key);if(key.empty())return reject("Empty view model key");
        if(unsigned(retention)>1)throw Failure{{ii::core::ErrorCode::invalid_argument,"Unknown model retention policy"}};
        if(!model){auto removed=erase_entry(key);removed_events(bool(removed.link),removed.views,removed.owner);return bool(removed.link);}
        auto existing=entries.find(key);
        if(existing==entries.end()&&entries.size()==limits.models)throw Failure{{ii::core::ErrorCode::full,"Registry model capacity exhausted"}};
        auto link=prepare_link(model);
        // Preallocate all map/set nodes before changing registry indices.
        std::map<std::string,Entry,std::less<>> next_entry;next_entry.emplace(key,Entry{.link=link});
        std::set<std::string,std::less<>> next_alias;next_alias.emplace(key);
        std::map<ModelProperties*,std::weak_ptr<Link>> next_index;next_index.emplace(model.get(),link);
        if(model->is_view_model()){
            auto current=model->read_property("key");
            if(!current||scalar_text(*current).empty()){
                auto result=model->write_property("key",Value(key));
                if(!result){auto value=model->read_property("key");if(!value||scalar_text(*value)!=key)throw Failure{std::move(result.error())};remember(std::move(result.error()));}
                else if(!*result)return reject("Failed to update model property");
            }
        }
        ensure_open();
        const bool adopted=retention==ModelRetention::retain&&!link->retained;
        if(adopted)link->retained=model;
        if(existing!=entries.end()&&existing->second.link==link)return adopted;
        auto previous=existing==entries.end()?std::shared_ptr<Link>{}:existing->second.link;
        if(existing==entries.end())entries.insert(next_entry.extract(next_entry.begin()));else existing->second.link=link;
        link->aliases.insert(next_alias.extract(next_alias.begin()));
        if(!link->active){links.erase(model.get());links.insert(next_index.extract(next_index.begin()));link->active=true;}
        if(previous){previous->aliases.erase(key);retire(previous);}
        emit({ViewModelRegistryEvent::keys,ViewModelRegistryEvent::descriptors});return true;
    }
    bool bind(std::string_view view_value,std::string_view key_value,bool writable){
        const auto view=token(view_value),key=token(key_value);
        if(view.empty())return reject("Empty view id");if(key.empty())return reject("Empty view model key");
        auto target=entries.find(key);if(target==entries.end()||target->second.link->model.expired())return reject("ViewModel key is not registered");
        auto& entry=target->second;
        if(writable&&!entry.owner.empty()&&entry.owner!=view)return reject("ViewModel is already owned by another view");
        auto old=view_keys.find(view);const bool binding_changed=old==view_keys.end()||old->second!=key;
        if(old==view_keys.end()&&view_keys.size()==limits.views)throw Failure{{ii::core::ErrorCode::full,"Registry view capacity exhausted"}};
        std::map<std::string,std::string,std::less<>> next_binding;next_binding.emplace(view,key);
        std::set<std::string,std::less<>> next_view;next_view.emplace(view);
        std::string next_owner=writable?view:entry.owner==view?std::string{}:entry.owner;
        bool ownership_changed=next_owner!=entry.owner;
        if(binding_changed){
            if(old!=view_keys.end()){
                auto previous=entries.find(old->second);
                if(previous!=entries.end()){previous->second.views.erase(view);if(previous->second.owner==view){previous->second.owner.clear();ownership_changed=true;}}
                old->second.swap(next_binding.begin()->second);
            }else view_keys.insert(next_binding.extract(next_binding.begin()));
            entry.views.insert(next_view.extract(next_view.begin()));
        }
        entry.owner.swap(next_owner);
        if(binding_changed)emit({ViewModelRegistryEvent::views});
        if(ownership_changed)emit({ViewModelRegistryEvent::ownership});
        if(binding_changed||ownership_changed)emit({ViewModelRegistryEvent::descriptors});return true;
    }
    std::string resolve_key(std::string_view view,std::string_view key)const{
        auto chosen=token(key);if(!chosen.empty())return chosen;
        auto found=view_keys.find(view);return found==view_keys.end()?std::string{}:found->second;
    }
    void close()noexcept{
        closed=true;receivers.clear();for(auto& [key,entry]:entries){(void)key;entry.link->active=false;entry.link->watch={};}
        entries.clear();view_keys.clear();links.clear();pending.clear();retiring_views.clear();
    }
};

ViewModelRegistry::ViewModelRegistry(ViewModelRegistryLimits limits):impl_(std::make_shared<Impl>(limits)){
    if(!limits.models||limits.models>65536||!limits.views||limits.views>65536||!limits.token_bytes||limits.token_bytes>1048576||!limits.pending_rounds||limits.pending_rounds>65536)throw std::invalid_argument("Invalid registry limits");
}
ViewModelRegistry::~ViewModelRegistry(){impl_->close();}
std::shared_ptr<ModelProperties> ViewModelRegistry::get(std::string_view value)const{auto m=impl_;m->readable();(void)m->keys_epoch.get();auto found=m->entries.find(m->token(value));return found==m->entries.end()?nullptr:found->second.link->model.lock();}
Result<bool> ViewModelRegistry::set(std::string key,std::shared_ptr<ModelProperties> model,ModelRetention retention){auto m=impl_;return m->run([&]{return m->put(std::move(key),model,retention);});}
Result<bool> ViewModelRegistry::register_view_model(std::shared_ptr<ModelProperties> model,std::string fallback,ModelRetention retention){auto m=impl_;return m->run([&]{
    if(!model)return m->reject("ViewModel object is null");auto key=m->token(fallback);
    if(model->is_view_model()){
        if(key.empty())if(auto current=model->read_property("key"))key=m->token(scalar_text(*current));
        if(key.empty())return m->reject("ViewModel key is empty");auto changed=model->write_property("key",Value(key));
        if(!changed){auto current=model->read_property("key");if(!current||scalar_text(*current)!=key)throw Failure{std::move(changed.error())};m->remember(std::move(changed.error()));}
        else if(!*changed)return m->reject("Failed to update model property");
    }
    if(key.empty())return m->reject("ViewModel key is empty");m->put(std::move(key),model,retention);return true;
});}
Result<bool> ViewModelRegistry::remove(std::string_view value){auto m=impl_;return m->run([&]{auto removed=m->erase_entry(m->token(value));m->removed_events(bool(removed.link),removed.views,removed.owner);return bool(removed.link);});}
Result<bool> ViewModelRegistry::clear(){auto m=impl_;return m->run([&]{
    const bool had_keys=!m->entries.empty(),had_views=!m->view_keys.empty();bool had_owners=false;
    for(auto& [key,entry]:m->entries){(void)key;had_owners|=!entry.owner.empty();entry.link->active=false;entry.link->watch={};}
    auto old=std::move(m->entries);m->entries.clear();m->view_keys.clear();m->links.clear();
    if(had_keys)m->emit({ViewModelRegistryEvent::keys});if(had_views)m->emit({ViewModelRegistryEvent::views});if(had_owners)m->emit({ViewModelRegistryEvent::ownership});if(had_keys||had_views||had_owners)m->emit({ViewModelRegistryEvent::descriptors});return had_keys||had_views||had_owners;
},false);}
Result<bool> ViewModelRegistry::bind_view(std::string_view view,std::string_view key,bool writable){auto m=impl_;return m->run([&]{return m->bind(view,key,writable);});}
Result<bool> ViewModelRegistry::unbind_view(std::string_view value){auto m=impl_;return m->run([&]{
    return m->unbind(m->token(value));
},false);}
Result<void> ViewModelRegistry::validate_view_admission(std::string_view value)const{
    auto m=impl_;if(auto ready=m->writable();!ready)return ready;
    return pd::attempt([&]()->Result<void>{
        try{if(m->token(value).empty())return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Empty view id");}
        catch(const Failure& error){return std::unexpected(error.error);}
        return {};
    });
}
Result<bool> ViewModelRegistry::retire_views(std::span<const std::string> values){
    auto m=impl_;
    if(m->owner!=std::this_thread::get_id())return ii::core::fail(ii::core::ErrorCode::wrong_executor,"Registry retirement requires its owner thread");
    if(m->closed)return ii::core::fail(ii::core::ErrorCode::closed,"Registry is closed");
    return pd::attempt([&]()->Result<bool>{
        if(values.size()>65536)return ii::core::fail(ii::core::ErrorCode::full,"Too many retiring registry views");
        std::set<std::string,std::less<>> prepared;
        try{for(const auto& value:values){auto view=m->token(value);if(m->view_keys.contains(view))prepared.insert(std::move(view));}}
        catch(const Failure& error){return std::unexpected(error.error);}
        const bool admitted=!prepared.empty();
        const auto enqueue=[&]{m->retiring_views.merge(prepared);return admitted;};
        if(m->changing||m->notifying||m->draining)return enqueue();
        return m->run(enqueue,false);
    });
}
std::string ViewModelRegistry::key_for_view(std::string_view value)const{auto m=impl_;m->readable();(void)m->views_epoch.get();auto found=m->view_keys.find(m->token(value));return found==m->view_keys.end()?std::string{}:found->second;}
std::shared_ptr<ModelProperties> ViewModelRegistry::get_for_view(std::string_view view)const{return get(key_for_view(view));}
Result<bool> ViewModelRegistry::claim_ownership(std::string_view view,std::string_view key){return bind_view(view,key,true);}
Result<bool> ViewModelRegistry::release_ownership(std::string_view value,std::string_view key_value){auto m=impl_;return m->run([&]{
    const auto view=m->token(value);if(view.empty())return m->reject("Empty view id");const auto key=m->resolve_key(view,key_value);auto entry=m->entries.find(key);
    if(entry==m->entries.end()||entry->second.owner!=view)return m->reject("Ownership not found");entry->second.owner.clear();m->emit({ViewModelRegistryEvent::ownership,ViewModelRegistryEvent::descriptors});return true;
});}
bool ViewModelRegistry::can_write(std::string_view value,std::string_view key_value)const{auto m=impl_;m->readable();(void)m->keys_epoch.get();(void)m->views_epoch.get();(void)m->owners_epoch.get();auto view=m->token(value);if(view.empty())return false;auto entry=m->entries.find(m->resolve_key(view,key_value));return entry!=m->entries.end()&&!entry->second.link->model.expired()&&entry->second.owner==view;}
std::string ViewModelRegistry::owner_of(std::string_view value)const{auto m=impl_;m->readable();(void)m->keys_epoch.get();(void)m->owners_epoch.get();auto entry=m->entries.find(m->token(value));return entry==m->entries.end()?std::string{}:entry->second.owner;}
Result<bool> ViewModelRegistry::update_property(std::string_view view,std::string_view name,Value value){return update_property_by_key(view,{},name,std::move(value));}
Result<bool> ViewModelRegistry::update_property_by_key(std::string_view view_value,std::string_view key_value,std::string_view property_value,Value value){auto m=impl_;return m->run([&]{
    auto view=m->token(view_value);if(view.empty())return m->reject("Empty view id");auto key=m->resolve_key(view,key_value);if(key.empty())return m->reject("View is not bound to a model");auto entry=m->entries.find(key);
    auto model=entry==m->entries.end()?nullptr:entry->second.link->model.lock();if(!model||entry->second.owner!=view)return m->reject("View has no write permission for the model");auto property=m->token(property_value);if(property.empty())return m->reject("Empty property name");
    auto changed=model->write_property(property,std::move(value));if(!changed){m->set_error("Failed to update model property");throw Failure{std::move(changed.error())};}return *changed?true:m->reject("Failed to update model property");
});}
std::optional<Value> ViewModelRegistry::read_property(std::string_view view,std::string_view name)const{
    auto m=impl_;m->readable();auto property=m->token(name);if(property.empty())return {};const auto key=key_for_view(view);(void)m->keys_epoch.get();auto entry=m->entries.find(key);if(entry==m->entries.end())return {};auto model=entry->second.link->model.lock();if(!model)return {};(void)entry->second.link->epoch.get();return ii::core::ObservableReadScope::pause([&]{return model->read_property(property);});
}
Value::Object ViewModelRegistry::descriptor(std::string_view value)const{
    auto m=impl_;m->readable();(void)m->keys_epoch.get();(void)m->views_epoch.get();(void)m->owners_epoch.get();const auto key=m->token(value);auto entry=m->entries.find(key);if(entry==m->entries.end())return {};auto model=entry->second.link->model.lock();if(!model)return {};(void)entry->second.link->epoch.get();
    auto result=ii::core::ObservableReadScope::pause([&]{return model->model_descriptor();});(void)Value(result);
    result.try_emplace("className",Value("ModelProperties"));result.try_emplace("viewModel",Value(false));result.try_emplace("stateModel",Value(false));result.insert_or_assign("key",Value(key));result.insert_or_assign("owner",Value(entry->second.owner));Value::Array views;
    Value::Object ordered;for(const auto& view:entry->second.views)ordered.emplace(view,Value{});for(auto& view:pd::ordered_keys(ordered))views.emplace_back(std::move(view));result.insert_or_assign("views",Value(std::move(views)));return result;
}
Result<bool> ViewModelRegistry::bind_route_view_model(std::string_view path,Value::Object route,Value::Object params,std::ptrdiff_t fallback){auto m=impl_;return m->run([&]{
    auto binding=route_binding(path,route,params,fallback);if(binding.model_key.empty())return false;
    m->set_error("");return m->bind(binding.view_id,binding.model_key,binding.writable);
},false);}
RouteModelBinding ViewModelRegistry::route_binding(std::string_view path,const Value::Object& route,const Value::Object& params,std::ptrdiff_t fallback){
    auto key=first_token(route,{"viewModelKey","modelKey"});if(key.empty())key=first_token(params,{"viewModelKey","modelKey"});
    auto view=first_token(route,{"viewId"});if(view.empty())view=first_token(params,{"viewId"});if(view.empty())view=pd::normalized_text(path);if(view.empty())view="_component_"+std::to_string(std::max<std::ptrdiff_t>(0,fallback));
    auto writable=first_bool(route);if(!writable)writable=first_bool(params);return {std::move(view),std::move(key),writable.value_or(false)};
}
std::vector<std::string> ViewModelRegistry::keys()const{auto m=impl_;m->readable();(void)m->keys_epoch.get();return names(m->entries);}
std::vector<std::string> ViewModelRegistry::views()const{auto m=impl_;m->readable();(void)m->views_epoch.get();return names(m->view_keys);}
Value::Object ViewModelRegistry::bindings()const{auto m=impl_;m->readable();(void)m->views_epoch.get();Value::Object result;for(const auto& [view,key]:m->view_keys)result.emplace(view,Value(key));return result;}
Value::Object ViewModelRegistry::owners()const{auto m=impl_;m->readable();(void)m->keys_epoch.get();(void)m->owners_epoch.get();Value::Object result;for(const auto& [key,entry]:m->entries)if(!entry.owner.empty())result.emplace(key,Value(entry.owner));return result;}
Value::Object ViewModelRegistry::descriptors()const{auto m=impl_;m->readable();(void)m->descriptors_epoch.get();return ii::core::ObservableReadScope::pause([&]{Value::Object result;for(const auto& [key,entry]:m->entries){(void)entry;auto value=descriptor(key);if(!value.empty())result.emplace(key,Value(std::move(value)));}return result;});}
std::string ViewModelRegistry::last_error()const{auto m=impl_;m->readable();return m->error.get();}
Result<void> ViewModelRegistry::observe(std::shared_ptr<const ViewModelRegistryPort> receiver){auto m=impl_;return m->observe(std::move(receiver));}
}
