#pragma once
#include <LVRS/ModelProperties.h>
#include <iiSDKCore/Runtime/Query.h>
#include <map>
#include <variant>

namespace lvrs {
// Missing entries and present null values are distinct. Object entries borrow
// an existing native property provider and never create a second property store.
class ModelEntry final {
public:
    ModelEntry()=default;
    ModelEntry(ii::core::Value value):data_(std::move(value)){}
    explicit ModelEntry(std::shared_ptr<const ModelProperties> object):data_(std::weak_ptr<const ModelProperties>(object)){}
    [[nodiscard]] bool valid()const noexcept;
    [[nodiscard]] std::optional<ii::core::Value> value()const;
    [[nodiscard]] std::optional<ii::core::Value> property(std::string_view)const;
private:
    friend class ModelSource;
    std::variant<std::monostate,ii::core::Value,std::weak_ptr<const ModelProperties>> data_;
};
enum class ItemModelEvent {rows_inserted,rows_removed,rows_moved,reset,layout,data,retired};
using ItemModelChangePort=ii::runtime::QueryPort<ItemModelEvent>;
// Root-level, read-only model protocol. A provider owns its data and publishes
// only after committing it. No QObject, JS engine, widget or document mutation.
class ItemModel {
    struct Impl;
public:
    ItemModel();
    virtual ~ItemModel();
    ItemModel(const ItemModel&)=delete;
    ItemModel& operator=(const ItemModel&)=delete;
    [[nodiscard]] virtual int rowCount()const=0;
    [[nodiscard]] virtual int columnCount()const=0;
    [[nodiscard]] virtual std::map<int,std::string> roleNames()const=0;
    [[nodiscard]] virtual std::optional<ii::core::Value> data(int row,int column,int role)const=0;
    [[nodiscard]] virtual bool validIndex(int row,int column)const;
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const ItemModelChangePort>)const;
    void requireOwner()const;
protected:
    [[nodiscard]] ii::core::Result<void> publish(ItemModelEvent);
private:
    std::shared_ptr<Impl> impl_;
};
// Explicit replacement for an invokable get()/at() sequence. Like the original
// object sequence, changes require ModelSource::invalidate().
class ModelSequence {
public:
    virtual ~ModelSequence()=default;
    [[nodiscard]] virtual int count()const=0;
    [[nodiscard]] virtual ModelEntry at(int)const=0;
};
class ModelInput final {
public:
    ModelInput()=default;
    ModelInput(ii::core::Value value):data_(std::move(value)){}
    ModelInput(std::shared_ptr<const ItemModel> model):data_(std::weak_ptr<const ItemModel>(model)){}
    ModelInput(std::shared_ptr<const ModelSequence> sequence):data_(std::weak_ptr<const ModelSequence>(sequence)){}
    [[nodiscard]] std::shared_ptr<const ItemModel> itemModel()const;
    [[nodiscard]] std::shared_ptr<const ModelSequence> sequence()const;
    [[nodiscard]] const ii::core::Value* value()const noexcept;
    friend bool operator==(const ModelInput&,const ModelInput&);
private:
    std::variant<ii::core::Value,std::weak_ptr<const ItemModel>,std::weak_ptr<const ModelSequence>> data_;
};
enum class ModelSourceEvent {source,column,revision,count};
struct ModelSourceChange final {
    ModelSourceEvent event=ModelSourceEvent::revision;
    std::uint64_t revision=0;
};
using ModelSourceChangePort=ii::runtime::QueryPort<ModelSourceChange>;
class ModelSource final {
    struct Impl;
public:
    ModelSource();
    ~ModelSource();
    ModelSource(const ModelSource&)=delete;
    ModelSource& operator=(const ModelSource&)=delete;
    [[nodiscard]] const ModelInput& source()const;
    [[nodiscard]] int column()const;
    [[nodiscard]] int count()const;
    [[nodiscard]] std::uint64_t revision()const;
    [[nodiscard]] bool itemModel()const;
    [[nodiscard]] ii::core::Result<bool> setSource(ModelInput);
    [[nodiscard]] ii::core::Result<bool> setColumn(int);
    [[nodiscard]] ModelEntry at(int)const;
    [[nodiscard]] ii::core::Value::Object row(int)const;
    [[nodiscard]] std::optional<ii::core::Value> roleValue(const ModelEntry&,std::string_view,std::optional<ii::core::Value> fallback={})const;
    [[nodiscard]] std::string textValue(const ModelEntry&,const ii::core::Value& roles,std::string fallback={})const;
    [[nodiscard]] bool boolValue(const ModelEntry&,std::string_view,bool fallback=false)const;
    [[nodiscard]] int intValue(const ModelEntry&,std::string_view,int fallback=0)const;
    [[nodiscard]] ii::core::Result<void> invalidate();
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const ModelSourceChangePort>);
private:
    std::shared_ptr<Impl> impl_;
};
}
