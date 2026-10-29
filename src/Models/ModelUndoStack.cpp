#include <LVRS/ModelUndoStack.h>
#include <LVRS/PresentationDetail.h>
#include <array>
#include <deque>
#include <limits>

namespace lvrs {
namespace pd=presentation_detail;
using ii::core::Result;
using Value=ii::core::Value;

struct ModelUndoStack::Impl final:pd::Signals<ModelUndoChange> {
    static_assert(std::is_nothrow_move_constructible_v<Value>);
    static_assert(std::is_nothrow_move_constructible_v<SnapshotTransfer>);
    std::deque<Value> undo,redo;
    std::int64_t limit=128;
    ii::core::Observable<std::uint64_t> limit_epoch{0},stack_epoch{0};

    bool trim(){
        const auto bound=std::size_t(limit);
        const bool changed=undo.size()>bound||redo.size()>bound;
        while(undo.size()>bound)undo.pop_front();
        while(redo.size()>bound)redo.pop_front();
        return changed;
    }
    Result<void> publish(std::span<const ModelUndoEvent> events){
        std::optional<ii::core::Error> failure;
        pd::Flag guard{notifying};
        for(auto event:events){
            if(closed)break;
            tick(event==ModelUndoEvent::limit?limit_epoch:stack_epoch,failure);
            send({event,undo.size(),redo.size()},failure);
        }
        if(failure)return std::unexpected(*failure);
        return {};
    }
    Result<void> changed(){const std::array events{ModelUndoEvent::stack};return publish(events);}
    Result<SnapshotTransfer> transfer(bool backwards,std::optional<Value> current){
        if(auto ready=writable();!ready)return std::unexpected(ready.error());
        return pd::attempt([&]()->Result<SnapshotTransfer>{
            auto& source=backwards?undo:redo;
            auto& destination=backwards?redo:undo;
            if(source.empty())return SnapshotTransfer{};
            // All throwing copies and allocations precede the first mutation.
            SnapshotTransfer result{.snapshot=source.back()};
            if(current)destination.push_back(std::move(*current));
            source.pop_back();
            if(!backwards)trim();
            pd::Flag guard{changing};
            try{
                if(auto notified=changed();!notified)result.observation_error=std::move(notified.error());
            }catch(...){result.observation_error=ii::core::Error{ii::core::ErrorCode::failed,{},std::current_exception()};}
            return result;
        });
    }
};

ModelUndoStack::ModelUndoStack():impl_(std::make_shared<Impl>()){}
ModelUndoStack::~ModelUndoStack(){auto model=impl_;model->closed=true;model->receivers.clear();}
std::int64_t ModelUndoStack::limit()const{auto model=impl_;model->readable();(void)model->limit_epoch.get();return model->limit;}
std::size_t ModelUndoStack::undo_depth()const{auto model=impl_;model->readable();(void)model->stack_epoch.get();return model->undo.size();}
std::size_t ModelUndoStack::redo_depth()const{auto model=impl_;model->readable();(void)model->stack_epoch.get();return model->redo.size();}
bool ModelUndoStack::can_undo()const{return undo_depth()!=0;}
bool ModelUndoStack::can_redo()const{return redo_depth()!=0;}

Result<bool> ModelUndoStack::set_limit(std::int64_t value){
    auto model=impl_;if(auto ready=model->writable();!ready)return std::unexpected(ready.error());
    if(value>std::numeric_limits<std::int32_t>::max())return ii::core::fail(ii::core::ErrorCode::invalid_argument,"Snapshot limit exceeds the supported integer range");
    const auto next=std::max<std::int64_t>(1,value);
    if(next==model->limit)return false;
    return pd::attempt([&]()->Result<bool>{
        model->limit=next;const bool trimmed=model->trim();pd::Flag guard{model->changing};
        const std::array events{ModelUndoEvent::limit,ModelUndoEvent::stack};
        if(auto notified=model->publish(std::span(events).first(trimmed?2:1));!notified)return std::unexpected(notified.error());
        return true;
    });
}
Result<bool> ModelUndoStack::clear(){
    auto model=impl_;if(auto ready=model->writable();!ready)return std::unexpected(ready.error());
    if(model->undo.empty()&&model->redo.empty())return false;
    return pd::attempt([&]()->Result<bool>{
        model->undo.clear();model->redo.clear();pd::Flag guard{model->changing};
        if(auto notified=model->changed();!notified)return std::unexpected(notified.error());
        return true;
    });
}
Result<bool> ModelUndoStack::push_snapshot(std::optional<Value> snapshot){
    auto model=impl_;if(auto ready=model->writable();!ready)return std::unexpected(ready.error());
    if(!snapshot||(!model->undo.empty()&&model->undo.back()==*snapshot))return false;
    return pd::attempt([&]()->Result<bool>{
        model->undo.push_back(std::move(*snapshot));model->trim();model->redo.clear();pd::Flag guard{model->changing};
        if(auto notified=model->changed();!notified)return std::unexpected(notified.error());
        return true;
    });
}
Result<SnapshotTransfer> ModelUndoStack::take_undo_snapshot(std::optional<Value> current){auto model=impl_;return model->transfer(true,std::move(current));}
Result<SnapshotTransfer> ModelUndoStack::take_redo_snapshot(std::optional<Value> current){auto model=impl_;return model->transfer(false,std::move(current));}
Result<void> ModelUndoStack::observe(std::shared_ptr<const ModelUndoChangePort> receiver){auto model=impl_;return model->observe(std::move(receiver));}
}
