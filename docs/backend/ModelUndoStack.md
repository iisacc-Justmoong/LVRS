# ModelUndoStack

위치: `src/backend/model/modelundostack.h`, `src/backend/model/modelundostack.cpp`

`ModelUndoStack`는 모델 돌연변이를 위한 한계가 설정된 C++ 스냅샷 스택입니다.

<a id="purpose"></a>

## 목적

- QML 보기 코드 외부에서 실행 취소/다시 실행 기록을 유지합니다.
- 파괴적이거나 입력된 편집 전에 모델 스냅샷을 저장합니다.
- 자체 스냅샷을 복원하는 모델 컨트롤러에 재사용 가능한 스택을 제공합니다.

## API

속성:

- `limit`
- `undoDepth`(읽기 전용)
- `redoDepth`(읽기 전용)
- `canUndo`(읽기 전용)
- `canRedo`(읽기 전용)

방법:

- `pushSnapshot(snapshot)`
- `takeUndoSnapshot(currentSnapshot)`
- `takeRedoSnapshot(currentSnapshot)`
- `clear()`

신호:

- `limitChanged()`
- `stackChanged()`

<a id="behavior-contract"></a>

## 행동 계약

- `pushSnapshot(...)`는 실행 취소 스냅샷을 추가하고 다시 실행 기록을 지웁니다.
- `takeUndoSnapshot(current)`는 이전 스냅샷을 반환하고 `current`를 다시 실행하도록 푸시합니다.
- `takeRedoSnapshot(current)`는 다시 실행 스냅샷을 반환하고 `current`를 실행 취소로 푸시합니다.
- `limit`는 최소 `1`에 고정됩니다. 한도를 초과하면 이전 스냅샷이 잘립니다.

<a id="consumers"></a>

## 소비자

`TableModel`는 셀 편집, 병합/분할, 행/열 구조 편집 및 행/열 크기 조정 편집을 위해 내부적으로 `ModelUndoStack`를 사용합니다. `Table.qml`는 결과 `undo()`, `redo()`, `canUndo` 및 `canRedo` API를 노출합니다.
