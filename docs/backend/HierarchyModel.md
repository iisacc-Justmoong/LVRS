# HierarchyModel

위치: `src/backend/model/hierarchymodel.h`, `src/backend/model/hierarchymodel.cpp`

`HierarchyModel`는 `HierarchyList`에서 사용하는 C++ 투영 레이어입니다.

<a id="purpose"></a>

## 목적

- 주입된 평면 모델을 생성된 계층 구조 행 설명자로 변환합니다.
- 대체 경로 역할, 유형 강제, `QAbstractItemModel` 읽기를 `HierarchyList.qml`에서 유지합니다.
- 소스 모델 행 또는 역할 매핑 속성이 변경되면 설명자를 다시 작성합니다.

## API

입력:

- `source`
- `column`
- `itemIdRole`, `itemKeyRole`, `labelRole`
- `iconNameRole`, `iconSourceRole`, `iconGlyphRole`
- `countRole`, `enabledRole`, `expandedRole`, `selectedRole`
- `activatableRole`, `draggableRole`, `showChevronRole`, `depthRole`

읽기 전용:

- `descriptors`
- `count`
- `revision`
- `hasSource`

방법:

- `descriptorAt(index)`
- `roleValue(entry, roleName, fallbackValue)`
- `depthArraySupportsEditing(nodes)`
- `sourceSupportsEditing()`
- `projectInteractionState(items)`
- `descendantRangeEnd(items, itemIndex)`
- `resolveDragTarget(items, sourceStart, sourceEnd, rawInsertionIndex, localX, indentStep, basePadding)`
- `moveDescriptors(items, sourceStart, sourceEnd, targetIndex, targetDepth)`
- `moveSourceRows(sourceStart, sourceEnd, targetIndex, targetDepth)`
- `invalidate()`

<a id="descriptor-contract"></a>

## 설명자 계약

각 설명자에는 다음이 포함됩니다.

- `itemId`, `itemKey`, `parentItemKey`
- `label`, `iconName`, `iconSource`, `iconGlyph`
- `count`, `showChevron`, `hasChildren`
- `expanded`, `selected`, `enabled`, `activatable`, `draggable`
- `indentLevel`, `pathLabel`, `nodeData`

<a id="how-it-works"></a>

## 동작 원리

- 입력 행은 `ModelSource`를 통해 읽습니다.
- 라벨 대체 경로 순서는 `labelRole`, `text`, `title`, `name`, `display`, `edit`입니다.
- `indentLevel`가 `depthRole`를 이겼습니다. 누락된 깊이의 기본값은 `0`입니다.
- `itemKey`는 명시적 키 역할을 사용한 다음 숫자 `itemId`, 행 인덱스를 사용합니다.
- `activatableRole`는 `selectable`/`activatable`를 통해 폴백됩니다. `draggableRole`는 `dragAllowed`/`draggable`를 통해 대체됩니다.
- `projectInteractionState`는 가시성, 가시적 인덱스, 조회 맵, 상위/경로 메타데이터, 형제 수, 하위 수 및 하위 항목 수를 소유합니다.
- 경로 배열은 중첩된 `QVariantList` 값으로 반환되므로 QML 행은 평면화된 문자열이 아닌 배열로 `ancestorItemKeys`, `pathItemKeys` 및 `pathItemLabels`를 받습니다.
- `resolveDragTarget`는 소스 블록 제거, 삽입 인덱스 조정, 깊이 경계, 상위 조회 및 삭제 모드 파생을 적용합니다.
- `moveDescriptors`는 재정렬된 설명자와 편집 가능한 깊이 모델에 대한 이동/삭제 메타데이터를 반환합니다.
- `moveSourceRows` 는 소스가 변경 가능한 `QAbstractItemModel` 또는 C++ 에 노출된 리스트와 같은 객체일 때 직접 백업 모델에 동일한 이동을 적용합니다. 항목 모델에는 `moveRows` 를 호출하고, 리스트와 같은 모델에는 `move(...)` 를 호출한 다음, 깊이와 부모 키 역할을 `setData`/`setProperty` 를 통해 다시 작성합니다.

<a id="qml-boundary"></a>

## QML 경계

`HierarchyList.qml` 는 여전히 시각적 `HierarchyItem` 인스턴스를 생성/파괴하고 포인터/키보드 이벤트를 전달합니다. `HierarchyModel` 는 설명자 투영, 메타데이터 투영, 편집 가능 모델 자격 확인, 후손 범위 계산, 드래그 타겟 계산, 설명자 재순열 및 편집 가능 모델 소스에 대한 직접 모델 작성에 대한 소유권을 가집니다.
