#pragma once
#include <iiSDKCore/Core/Value/Value.h>
#include <iiSDKCore/Runtime/Query.h>
#include <memory>
#include <optional>

namespace lvrs {
enum class ModelUndoEvent {limit,stack};
struct ModelUndoChange final {
    ModelUndoEvent event=ModelUndoEvent::stack;
    std::size_t undo_depth=0,redo_depth=0;
};
using ModelUndoChangePort=ii::runtime::QueryPort<ModelUndoChange>;
// A committed transfer must return its snapshot even when an observer fails.
// Outer Result failures mean the transfer did not commit.
struct SnapshotTransfer final {
    std::optional<ii::core::Value> snapshot;
    std::optional<ii::core::Error> observation_error;
};
class ModelUndoStack final {
public:
    ModelUndoStack();
    ~ModelUndoStack();
    ModelUndoStack(const ModelUndoStack&)=delete;
    ModelUndoStack& operator=(const ModelUndoStack&)=delete;
    [[nodiscard]] std::int64_t limit()const;
    [[nodiscard]] std::size_t undo_depth()const;
    [[nodiscard]] std::size_t redo_depth()const;
    [[nodiscard]] bool can_undo()const;
    [[nodiscard]] bool can_redo()const;
    [[nodiscard]] ii::core::Result<bool> set_limit(std::int64_t);
    [[nodiscard]] ii::core::Result<bool> clear();
    [[nodiscard]] ii::core::Result<bool> push_snapshot(std::optional<ii::core::Value>);
    [[nodiscard]] ii::core::Result<SnapshotTransfer> take_undo_snapshot(std::optional<ii::core::Value> current={});
    [[nodiscard]] ii::core::Result<SnapshotTransfer> take_redo_snapshot(std::optional<ii::core::Value> current={});
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const ModelUndoChangePort>);
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
}
