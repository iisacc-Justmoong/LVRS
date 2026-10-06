# NavigationStackModel

위치: `src/backend/navigation/navigationstackmodel.h`, `src/backend/navigation/navigationstackmodel.cpp`

`NavigationStackModel`는 `PageRouter.qml`에 대한 경로 스택 돌연변이 및 뷰 추적 파생을 소유합니다.

<a id="purpose"></a>

## 목적

- 경로 항목이 페이지 스택에 들어가기 전에 정규화합니다.
- 푸시, 교체, 루트 설정, 팝 및 팝-루트 작업에 대한 스택 결과를 빌드합니다.
- 경로 기반 항목과 함께 구성 요소 기반 스택 항목을 유지합니다.
- 커밋된 스택에서 현재 경로/매개변수 및 뷰 추적 메타데이터를 파생합니다.

## API

입력:

- `path`

읽기 전용:

- `currentPath`
- `currentParams`
- `viewTrackingEntries`
- `trackedViewIds`
- `depth`

방법:

- `normalizePath(pathValue)`
- `createPathEntry(pathValue, params)`
- `createComponentPathEntry(component, params)`
- `stackAfterPathOperation(pathValue, params, mode)`
- `stackAfterComponentOperation(component, params, mode)`
- `stackAfterPop()`
- `stackAfterPopToRoot()`
- `applyPathOperation(pathValue, params, mode)`
- `applyComponentOperation(component, params, mode)`
- `pop()`
- `popToRoot()`
- `currentEntryDescriptor()`
- `createViewTrackingEntry(entry, index)`
- `buildViewTrackingEntries(pathValue?)`
- `updateTrackedViewIds(entries)`

<a id="stack-entry-contract"></a>

## 스택 진입 계약

경로 항목:

- `path`
- `params`

구성 요소 항목:

- 빈 문자열인 `path`
- `params`
- `component`

조회 추적 항목:

- `viewId`
- `path`
- `enabled`

<a id="how-it-works"></a>

## 동작 원리

- 경로 정규화는 `RouteMatcher`에 위임됩니다.
- `mode == "set"`는 전체 스택을 하나의 항목으로 대체합니다.
- `mode == "replace"`는 현재 항목을 대체합니다.
- 다른 모드에서는 새 항목이 추가됩니다.
- `currentPath` 및 `currentParams`는 마지막 스택 항목에서 파생됩니다.
- 구성 요소 항목은 `viewId`가 제공되지 않는 한 `_component_<index>` 형식으로 생성된 보기 ID를 받습니다.
- `enabled: false`, `disabled: true` 또는 `params.disabled: true`가 포함된 항목은 보기 추적이 비활성화된 것으로 표시됩니다.
- `updateTrackedViewIds`는 최신 추적 세트에서 사라진 ID를 반환하므로 QML는 스냅샷을 릴리스할 수 있습니다.

<a id="qml-boundary"></a>

## QML 경계

`PageRouter.qml`는 여전히 `StackView` 작업, 경로 확인 및 대화형 전환 오케스트레이션을 소유하고 있습니다. `NavigationStackModel`는 커밋된 스택 수학, 정규화된 항목 생성, 현재 항목 파생 및 보기 추적 항목 생성을 소유합니다.
