#include "ModelSource.h"
#include "ModelAdapter.h"
#include "PresentationDetail.h"
#include <iiSDKCore/Core/Text/Number.h>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>

namespace lvrs {
namespace pd=presentation_detail;
using Value=ii::core::Value;
using ii::core::Result;
namespace {
bool null(const Value& value){return value.kind()==ii::core::ValueKind::null;}
std::string text(const Value& value){
    return std::visit([](const auto& item)->std::string{
        using T=std::remove_cvref_t<decltype(item)>;
        if constexpr(std::same_as<T,std::string>)return item;
        else if constexpr(std::same_as<T,bool>)return item?"true":"false";
        else if constexpr(std::same_as<T,double>)return ii::core::format_decimal(item);
        else if constexpr(std::integral<T>)return std::to_string(item);
        else if constexpr(std::same_as<T,ii::core::Bytes>){
            std::string result(reinterpret_cast<const char*>(item.data()),item.size());
            return ii::core::valid_utf8(result)?result:std::string{};
        }else return {};
    },value.storage());
}
std::optional<int> integer(const Value& value){
    return std::visit([](const auto& item)->std::optional<int>{
        using T=std::remove_cvref_t<decltype(item)>;
        if constexpr(std::same_as<T,bool>)return item?1:0;
        else if constexpr(std::integral<T>){static_assert(sizeof(int)==4);return std::bit_cast<std::int32_t>(std::uint32_t(item));}
        else if constexpr(std::same_as<T,double>){
            // QVariant rounds to a signed 64-bit integer, then narrows to int.
            const double rounded=std::round(item);
            if(rounded>=-0x1p63&&rounded<0x1p63)return std::bit_cast<std::int32_t>(std::uint32_t(std::int64_t(rounded)));
            return {};
        }else if constexpr(std::same_as<T,std::string>||std::same_as<T,ii::core::Bytes>){
            auto source=[&]{if constexpr(std::same_as<T,std::string>)return std::string_view(item);else return std::string_view(reinterpret_cast<const char*>(item.data()),item.size());}();
            if(!ii::core::valid_utf8(source))return {};
            auto clean=pd::normalized_text(source);std::string_view digits=clean;
            if(digits.starts_with('+')){digits.remove_prefix(1);if(digits.starts_with('-')||digits.starts_with('+'))return {};}
            if(digits.empty())return {};std::int64_t result=0;
            const auto parsed=std::from_chars(digits.data(),digits.data()+digits.size(),result,10);
            if(parsed.ec==std::errc{}&&parsed.ptr==digits.data()+digits.size())return std::bit_cast<std::int32_t>(std::uint32_t(result));
            return {};
        }else return {};
    },value.storage());
}
int source_count(const ModelInput& source){
    if(auto model=source.itemModel()){model->requireOwner();return std::max(0,model->rowCount());}
    if(auto sequence=source.sequence())return std::max(0,sequence->count());
    if(auto value=source.value())if(auto array=std::get_if<Value::Array>(&value->storage())){
        if(array->size()>std::size_t(std::numeric_limits<int>::max()))throw std::length_error("Model source exceeds the integer index range");
        return int(array->size());
    }
    return 0;
}
template<class T>bool same_weak(const std::weak_ptr<T>& a,const std::weak_ptr<T>& b){return !a.owner_before(b)&&!b.owner_before(a)&&a.lock().get()==b.lock().get();}
}

bool ModelEntry::valid()const noexcept{
    if(std::holds_alternative<Value>(data_))return true;
    if(auto object=std::get_if<std::weak_ptr<const ModelProperties>>(&data_))return !object->expired();
    return false;
}
std::optional<Value> ModelEntry::value()const{if(auto value=std::get_if<Value>(&data_))return *value;return {};}
std::optional<Value> ModelEntry::property(std::string_view key)const{
    if(auto value=std::get_if<Value>(&data_)){if(auto found=value->find(key))return found->get();}
    else if(auto object=std::get_if<std::weak_ptr<const ModelProperties>>(&data_))if(auto live=object->lock())return live->read_property(key);
    return {};
}
struct ItemModel::Impl final:pd::Signals<ItemModelEvent> {};
ItemModel::ItemModel():impl_(std::make_shared<Impl>()){}
ItemModel::~ItemModel(){
    auto p=impl_;p->closed=true;
    // Normal sends stop once closed. Retirement is a separate final delivery:
    // detach the receivers first and keep all reads/commands closed throughout.
    const auto receivers=std::move(p->receivers);
    if(p->owner==std::this_thread::get_id())for(const auto& weak:receivers)
        if(auto port=weak.lock())try{(void)port->send(ItemModelEvent::retired);}catch(...){}
}
void ItemModel::requireOwner()const{impl_->readable();}
bool ItemModel::validIndex(int row,int column)const{requireOwner();return row>=0&&column>=0&&row<rowCount()&&column<columnCount();}
Result<void> ItemModel::observe(std::shared_ptr<const ItemModelChangePort> port)const{return pd::attempt([&]{return impl_->observe(std::move(port));});}
Result<void> ItemModel::publish(ItemModelEvent event){
    auto p=impl_;if(auto ready=p->writable();!ready)return ready;
    if(event==ItemModelEvent::retired)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Retirement is owned by the item-model lifetime");
    return pd::attempt([&]()->Result<void>{pd::Flag guard{p->notifying};std::optional<ii::core::Error> failure;p->send(event,failure);if(failure)return std::unexpected(*failure);return {};});
}
std::shared_ptr<const ItemModel> ModelInput::itemModel()const{if(auto value=std::get_if<std::weak_ptr<const ItemModel>>(&data_))return value->lock();return {};}
std::shared_ptr<const ModelSequence> ModelInput::sequence()const{if(auto value=std::get_if<std::weak_ptr<const ModelSequence>>(&data_))return value->lock();return {};}
const Value* ModelInput::value()const noexcept{return std::get_if<Value>(&data_);}
bool operator==(const ModelInput& a,const ModelInput& b){
    if(a.data_.index()!=b.data_.index())return false;
    if(auto value=std::get_if<Value>(&a.data_))return *value==std::get<Value>(b.data_);
    if(auto value=std::get_if<std::weak_ptr<const ItemModel>>(&a.data_))return same_weak(*value,std::get<std::weak_ptr<const ItemModel>>(b.data_));
    return same_weak(std::get<std::weak_ptr<const ModelSequence>>(a.data_),std::get<std::weak_ptr<const ModelSequence>>(b.data_));
}

struct ModelSource::Impl final:pd::Signals<ModelSourceChange> {
    ModelInput source;
    int column=0;
    std::uint64_t revision=0;
    ii::core::Observable<std::uint64_t> epoch{0};
    std::shared_ptr<ItemModelChangePort> connection;
    bool pending=false;
    void read()const{readable();(void)epoch.get();}
    Result<void> admission()const{
        if(auto ready=writable();!ready)return ready;
        if(revision==UINT64_MAX)return ii::core::fail(ii::core::ErrorCode::full,"Model source revision exhausted");
        return {};
    }
    Result<void> publish(std::span<const ModelSourceEvent> events){
        pd::Flag guard{notifying};std::optional<ii::core::Error> failure;
        tick(epoch,failure);
        for(auto event:events){if(closed)break;send({event,revision},failure);}
        if(failure)return std::unexpected(*failure);return {};
    }
    Result<void> refresh(){
        if(closed)return {};
        if(changing||notifying){pending=true;return {};}
        if(auto ready=admission();!ready)return ready;
        return pd::attempt([&]()->Result<void>{
            std::optional<ii::core::Error> failure;
            for(unsigned round=0;round<256;++round){
                pending=false;
                if(revision==UINT64_MAX)return ii::core::fail(ii::core::ErrorCode::full,"Model source revision exhausted");
                ++revision;const std::array events{ModelSourceEvent::revision,ModelSourceEvent::count};
                if(auto sent=publish(events);!sent&&!failure)failure=sent.error();
                if(closed||!pending){if(failure)return std::unexpected(*failure);return {};}
            }
            return ii::core::fail(ii::core::ErrorCode::busy,"Model source notifications did not settle");
        });
    }
    Result<void> finish(std::span<const ModelSourceEvent> events){
        std::optional<ii::core::Error> failure;
        if(auto sent=publish(events);!sent)failure=sent.error();
        if(pending&&!closed)if(auto sent=refresh();!sent&&!failure)failure=sent.error();
        if(failure)return std::unexpected(*failure);return {};
    }
};
ModelSource::ModelSource():impl_(std::make_shared<Impl>()){}
ModelSource::~ModelSource(){auto p=impl_;p->closed=true;p->receivers.clear();if(p->connection)(void)p->connection->close();p->connection.reset();}
const ModelInput& ModelSource::source()const{impl_->read();return impl_->source;}
int ModelSource::column()const{impl_->read();return impl_->column;}
int ModelSource::count()const{impl_->read();return source_count(impl_->source);}
std::uint64_t ModelSource::revision()const{impl_->read();return impl_->revision;}
bool ModelSource::itemModel()const{impl_->read();return bool(impl_->source.itemModel());}
Result<bool> ModelSource::setSource(ModelInput value){
    auto p=impl_;if(auto ready=p->admission();!ready)return std::unexpected(ready.error());
    if(value==p->source)return false;
    return pd::attempt([&]()->Result<bool>{
        int previous=0,next=0;
        {
        pd::Flag preparing{p->changing};
        previous=source_count(p->source);next=source_count(value);
        std::shared_ptr<ItemModelChangePort> connection;
        if(auto model=value.itemModel()){
            const std::weak_ptr<Impl> weak=p;
            connection=std::make_shared<ItemModelChangePort>([weak](const auto&){if(auto state=weak.lock();state&&!state->closed)if(auto result=state->refresh();!result)throw std::runtime_error(result.error().message);});
            if(auto observed=model->observe(connection);!observed)return std::unexpected(observed.error());
        }
        if(p->connection)(void)p->connection->close();
        p->connection=std::move(connection);p->source=std::move(value);++p->revision;
        }
        const std::array events{ModelSourceEvent::source,ModelSourceEvent::revision,ModelSourceEvent::count};
        if(auto sent=p->finish(std::span(events).first(previous==next?2:3));!sent)return std::unexpected(sent.error());
        return true;
    });
}
Result<bool> ModelSource::setColumn(int value){
    auto p=impl_;if(auto ready=p->admission();!ready)return std::unexpected(ready.error());
    value=std::max(0,value);if(value==p->column)return false;
    return pd::attempt([&]()->Result<bool>{p->column=value;++p->revision;const std::array events{ModelSourceEvent::column,ModelSourceEvent::revision};if(auto sent=p->finish(events);!sent)return std::unexpected(sent.error());return true;});
}
ModelEntry ModelSource::at(int index)const{
    auto p=impl_;p->read();if(index<0)return {};
    if(p->source.itemModel())return ModelEntry(Value(row(index)));
    if(auto sequence=p->source.sequence())return sequence->at(index);
    if(auto value=p->source.value())if(auto array=std::get_if<Value::Array>(&value->storage());array&&std::size_t(index)<array->size())return ModelEntry((*array)[std::size_t(index)]);
    return {};
}
Value::Object ModelSource::row(int index)const{auto p=impl_;p->read();return ModelAdapter{}.row(p->source,index,p->column);}
std::optional<Value> ModelSource::roleValue(const ModelEntry& entry,std::string_view role,std::optional<Value> fallback)const{
    impl_->read();auto key=pd::normalized_text(role);if(key.empty())return fallback;
    if(auto value=entry.property(key))return *value;return fallback;
}
std::string ModelSource::textValue(const ModelEntry& entry,const Value& roles,std::string fallback)const{
    impl_->read();if(!entry.valid())return fallback;
    if(auto value=entry.value()){
        if(null(*value))return fallback;
        if(value->kind()!=ii::core::ValueKind::object)return text(*value);
    }
    const auto lookup=[&](const Value& role)->std::optional<std::string>{auto key=pd::normalized_text(text(role));if(key.empty())return {};if(auto value=entry.property(key);value&&!null(*value))return text(*value);return {};};
    if(auto array=std::get_if<Value::Array>(&roles.storage())){for(const auto& role:*array)if(auto value=lookup(role))return *value;}
    else if(auto value=lookup(roles))return *value;
    return fallback;
}
bool ModelSource::boolValue(const ModelEntry& entry,std::string_view role,bool fallback)const{
    const auto value=roleValue(entry,role);if(!value||null(*value))return fallback;
    if(auto boolean=std::get_if<bool>(&value->storage()))return *boolean;
    // The original tests canConvert<int>(), rather than conversion success:
    // a nonnumeric string therefore becomes false, even when fallback is true.
    if(value->kind()==ii::core::ValueKind::array||value->kind()==ii::core::ValueKind::object)return false;
    return integer(*value).value_or(0)!=0;
}
int ModelSource::intValue(const ModelEntry& entry,std::string_view role,int fallback)const{const auto value=roleValue(entry,role);return value?integer(*value).value_or(fallback):fallback;}
Result<void> ModelSource::invalidate(){auto p=impl_;if(auto ready=p->admission();!ready)return ready;return p->refresh();}
Result<void> ModelSource::observe(std::shared_ptr<const ModelSourceChangePort> port){auto p=impl_;return pd::attempt([&]{return p->observe(std::move(port));});}
}
