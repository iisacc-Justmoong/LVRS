# ModelUndoStack

Location: `backend/model/modelundostack.h`, `backend/model/modelundostack.cpp`

`ModelUndoStack` is a bounded C++ snapshot stack for model mutations.

## Purpose

- Keep undo/redo history outside QML view code.
- Store model snapshots before destructive or typed edits.
- Provide a reusable stack for model controllers that restore their own snapshots.

## API

Properties:

- `limit`
- `undoDepth` (readonly)
- `redoDepth` (readonly)
- `canUndo` (readonly)
- `canRedo` (readonly)

Methods:

- `pushSnapshot(snapshot)`
- `takeUndoSnapshot(currentSnapshot)`
- `takeRedoSnapshot(currentSnapshot)`
- `clear()`

Signals:

- `limitChanged()`
- `stackChanged()`

## Behavior Contract

- `pushSnapshot(...)` appends an undo snapshot and clears redo history.
- `takeUndoSnapshot(current)` returns the previous snapshot and pushes `current` onto redo.
- `takeRedoSnapshot(current)` returns the redo snapshot and pushes `current` onto undo.
- `limit` is clamped to at least `1`; older snapshots are trimmed when the limit is exceeded.

## Consumers

`TableModel` uses `ModelUndoStack` internally for cell edits, merge/split, row/column structure edits, and row/column resize edits. `Table.qml` exposes the resulting `undo()`, `redo()`, `canUndo`, and `canRedo` API.
