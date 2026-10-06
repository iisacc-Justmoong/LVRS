# HierarchyItem

위치: `src/qml/components/navigation/HierarchyItem.qml`

`HierarchyItem`는 `HierarchyList`에서 사용되는 가장 작은 계층 구조 행 기본 요소이지만 이제 실제 디렉터리 트리, 아웃라이너 또는 계층 노드 계약 역할을 하기에 충분한 구조적 및 UX 상태를 노출합니다.

<a id="purpose"></a>

## 목적

- Figma 정렬 행 기준선(아이콘, 레이블, 선택적 갈매기형)을 렌더링합니다.
- 항목별 계층 구조 메타데이터를 행 자체에 직접 노출합니다.
- UX 상태 및 어포던스 상태를 행 자체에 직접 노출합니다.

<a id="implemented-api-inventory"></a>

## API 인벤토리 구현

ID 및 목록 컨텍스트:

- `modelData`(생성된 행에 대해 `HierarchyList.itemDelegate`에 의해 삽입된 설명자)
- `index`(`HierarchyList.itemDelegate`에 의해 생성된 설명자 인덱스, 기본값 `-1`)
- `itemId`, `itemKey`, `parentItemKey`
- `parentLabel`
- `pathLabel`, `parentPathLabel`
- `hierarchyList`
- `nodeData`
- `generatedByTreeModel`
- `resolvedItemId`(`flatIndex`를 사용하는 읽기 전용 대체 경로)
- `resolvedItemKey`(`itemId` 또는 `flatIndex`를 사용하는 읽기 전용 대체 경로)
- `resolvedLabel`, `resolvedPathLabel`(읽기 전용)

트리 구조 및 순서:

- `indentLevel`, `treeDepth`
- `flatIndex`
- `visibleIndex`
- `siblingIndex`
- `visibleSiblingIndex`
- `hasParentItem`(읽기 전용)
- `isRootItem`, `isBranchItem`, `isLeafItem`(읽기 전용)

하위 메타데이터:

- `hasChildItems`
- `childCount`
- `visibleChildCount`
- `hiddenChildCount`(읽기 전용)
- `descendantCount`
- `visibleDescendantCount`
- `hiddenDescendantCount`(읽기 전용)
- `childItemKeys`
- `childItemLabels`
- `childItemKeysText`(읽기 전용 쉼표로 구분된 문자열)
- `childItemLabelsText`(읽기 전용 쉼표로 구분된 문자열)
- `hasVisibleChildItems`, `hasHiddenChildItems`(읽기 전용)
- `hasVisibleDescendants`, `hasHiddenDescendants`(읽기 전용)
- `firstChildItemKey`, `firstChildItemLabel`(읽기 전용)
- `lastChildItemKey`, `lastChildItemLabel`(읽기 전용)

상위 및 경로 메타데이터:

- `ancestorItemKeys`
- `ancestorLabels`
- `ancestorItemKeysText`(읽기 전용 쉼표로 구분된 문자열)
- `ancestorLabelsText`(읽기 전용 쉼표로 구분된 문자열)
- `pathItemKeys`
- `pathItemLabels`
- `pathItemKeysText`(읽기 전용 쉼표로 구분된 문자열)
- `pathItemLabelsText`(읽기 전용 쉼표로 구분된 문자열)

형제 메타데이터:

- `siblingIndex`
- `visibleSiblingIndex`
- `siblingCount`
- `visibleSiblingCount`
- `isFirstSibling`, `isLastSibling`, `isOnlySibling`(읽기 전용)
- `isFirstVisibleSibling`, `isLastVisibleSibling`, `isOnlyVisibleSibling`(읽기 전용)
- `count`(기본값 `-1`, `0` 이상으로 설정될 때까지 숨겨짐)
- `countView`(`Component`; 선택적 사용자 정의 후행 카운트 보기)
- `countViewItem`(읽기 전용 로드 카운트 뷰 인스턴스)
- `effectiveShowCount`(읽기 전용)

확장 및 쉐브론 어포던스:

- `showChevron`
- `effectiveShowChevron`(읽기 전용)
- `trailingChevronAnchorVisible`(읽기 전용)
- `chevronExpandable`(읽기 전용)
- `expanded`
- `collapsed`(읽기 전용)
- `canToggleExpanded`, `canExpand`, `canCollapse`(읽기 전용)
- 방향 상수: `directionRight`, `directionLeft`, `directionUp`, `directionDown`
- `selectionDirection`
- `resolvedSelectionDirection`(읽기 전용)
- `resolvedChevronRotation`(읽기 전용)
- `resolvedChevronIconName`(읽기 전용)

활성화 및 선택:

- `selected`
- `activatable`
- `selectable`(`activatable`의 별칭)
- `active`(읽기 전용)
- `inactive`(읽기 전용)
- `canBecomeActive`(읽기 전용)

UX 상태:

- 열거 상수: `uxStateIdle` , `uxStateHover` , `uxStateActive` , `uxStateInactive` , `uxStatePressed` , `uxStateDrag`
- `uxState`(읽기 전용)
- `uxStateName`(읽기 전용)
- 호환성 상태 필드: `stateIdle` , `stateHover` , `stateActive`
- 호환성 상호작용 필드: `interactionState` , `interactionStateName`
- 주 깃발: `isHoverState` , `isActiveState` , `isInactiveState` , `isPressedState` , `isDragState`

드래그 어포던스:

- `dragAllowed`
- `dragPreviewActive`
- `dragPreviewOpacity`
- `draggable`(읽기 전용, 목록 편집 가능성 반영)
- `dragEnabled`(읽기 전용, 항목 수준 드래그 API 가용성)
- `pointerDragRequiresLongPress`(읽기 전용)
- `immediatePointerDragEnabled`(읽기 전용)
- `mobileDragHoldInterval`(기본값 `1000`)
- `dragging`(읽기 전용)
- `dragActive`, `dragTargetValid`, `dragSourceItem`, `dragAnchorItem`, `dragParentTargetItem`(읽기 전용)
- `dragSourceIndex`, `dragSourceEndIndex`(읽기 전용)
- `dragTargetIndex`, `dragTargetDepth`(읽기 전용)
- 드롭 모드 상수: `dropModeNone` , `dropModeBefore` , `dropModeAfter` , `dropModeChild` , `dropModeRoot`
- `dragTargetMode`, `dragTargetModeName`(읽기 전용)
- `dragTargetParentItemKey`, `dragTargetParentLabel`, `dragTargetParentPathLabel`(읽기 전용)
- `dragTargetAnchorItemKey`, `dragTargetAnchorLabel`(읽기 전용)
- `dropBefore`, `dropAfter`, `dropAsChild`, `dropAsRoot`(읽기 전용)

드래그/이동 방법:

- `beginDrag(localX, localY)`
- `updateDrag(localX, localY)`
- `endDrag(commitMove)`
- `commitDrag()`
- `cancelDrag()`
- `moveTo(targetIndex, targetDepth)`
- `moveBefore(targetItem)`
- `moveAfter(targetItem)`
- `moveAsChildOf(targetItem)`
- `moveToRoot()`

드래그 신호:

- `dragStarted(sourceIndex, sourceEndIndex, sourceDepth)`
- `dragUpdated(targetIndex, targetDepth, modeName, parentItemKey, anchorItemKey)`
- `dragEnded(committed, fromIndex, toIndex, targetDepth, modeName, parentItemKey, anchorItemKey)`

Figma의 레이아웃 기본값:

- `rowHeight`(기본값 `20`)
- `itemWidth`(기본값 `200`)
- `iconSize`(데스크탑 및 모바일의 경우 기본 `Theme.iconSm`: `18`)
- `chevronSize`(데스크톱 `16`, 모바일 `32`)
- `baseLeftPadding`(기본값 `8`)
- `rowRightPadding`(기본값 `8`)
- `leadingSpacing`(기본값 `2`)
- `computedLeftPadding`(읽기 전용)

시각적 토큰:

- `iconPlaceholderColor`
- `textColorNormal`, `textColorDisabled`
- `rowBackgroundColorIdle`
- `rowBackgroundColor`(읽기 전용으로 확인된 호환성 색상)
- `rowBackgroundColorHover`
- `rowBackgroundColorPressed`
- `rowBackgroundColorActive`
- `rowBackgroundColorInactive`
- `rowBackgroundColorDrag`
- `rowVisible`(읽기 전용, `_rowVisibleInternal`에서)

<a id="behavior-contract"></a>

## 행동 계약

- Figma 구성 요소(`314:93`)는 `8px` 가로 패딩이 있는 `200x20` 행, `18px` 선행 아이콘, `2px` 선행 간격, `16px` 갈매기 모양 및 데스크톱 및 모바일의 `5px` 코너 반경.
- 레이블은 프로젝트 전체 본문 글꼴 크기를 `13`로 유지하고 계층 구조 행의 Figma 라인 상자를 데스크톱 및 모바일의 `16px`로 유지합니다.
- `count`는 기본 `-1`에서 숨겨진 상태로 유지됩니다. 후행 카운트 콘텐츠를 렌더링하려면 `count`를 `0` 이상으로 설정하세요.
- 후행 카운트 콘텐츠는 갈매기형 자체가 표시되지 않는 경우에도 `8px` 간격을 사용하여 갈매기형 앵커 바로 왼쪽에 렌더링됩니다.
- `countView`는 기본 후행 개수 레이블을 대체합니다. 사용자 정의 보기가 `count` 및/또는 `hierarchyItem` 속성을 노출하는 경우 `HierarchyItem`는 로드 후 해당 속성을 동기화합니다.
- 상세 Figma 컴포넌트 ( `314:93` ) 는 3 표준 시각적 채우기를 정의합니다: `Default` = 투명, `Inactive` = `Theme.panelBackground12` , `Active` = `Theme.accentBlueMuted` .
- `HierarchyList`는 상위/하위/주문/경로 메타데이터를 각 관리 행에 동기화합니다.
- 항목 수준 드래그/드롭은 `HierarchyItem`에서 시작되고 커밋됩니다. `HierarchyList`는 지원 프로젝션을 제공하고 소스 변형을 계층 구조 모델에 위임합니다.
- `childCount`는 직속 하위 수입니다. `descendantCount`는 행 아래의 전체 하위 트리 수입니다.
- `visibleDescendantCount`는 행 아래에 현재 표시되는 하위 항목만 계산하므로 축소된 하위 항목은 `hiddenDescendantCount`를 통해 측정 가능한 상태로 유지됩니다.
- `childItemKeysText` 및 `childItemLabelsText`는 배열을 직접 검사하고 싶지 않은 소비자가 요청한 문자열 형식 하위 요약을 제공합니다.
- `ancestor*`, `pathItem*` 및 형제 수 필드는 `HierarchyList`를 다시 조회하지 않고도 행에서 직접 계보 및 로컬 순서를 노출합니다.
- 생성된 행을 드래그하면 연결된 평면 깊이 모델이 제자리에 다시 작성됩니다. 행 순서 변경, 이동된 하위 트리 깊이 변경 및 `parentKey` / `parentItemKey`가 배열 개체 또는 쓰기 가능한 모델 역할에서 다시 계산됩니다.
- `dragAllowed`를 사용하면 호스트는 편집 가능한 계층 구조 내에서 보호된 노드에 대한 드래그 시작을 방지하면서 행을 선택 가능하고 표시할 수 있도록 유지합니다.
- 모바일 대상(`Theme.mobileTarget == true`)에서는 `1000ms`에 대한 행이 유지될 때까지 포인터 끌기 시작이 지연됩니다. 데스크탑 대상은 즉각적인 드래그 픽업을 유지합니다.
- 모바일 대상에서는 누르는 대신 손을 떼거나 클릭할 때 터치 활성화가 이루어지므로 주변 `Flickable` 표면은 행이 활성화되기 전에 스크롤 제스처를 훔칠 수 있습니다.
- 대화형 행 활성화는 행이 이미 활성화된 경우에도 목록 활성화를 다시 요청하므로 활성화 콜백을 수신하는 호스트는 반복적인 탭/클릭을 의도적인 작업 트리거로 처리할 수 있습니다.
- `dragTargetModeName`는 현재 삭제 의도를 `before`, `after`, `child` 또는 `root`로 확인합니다.
- `activatable`/`selectable` 행 클릭으로 갈매기형 기반 확장을 방해하지 않고 항목을 활성화할 수 있는지 여부를 제어합니다.
- `uxState`는 UX 처리를 위한 기본 열거형입니다. 우선순위는 `Drag` -> `Inactive` -> `Active` -> `Pressed` -> `Hover` -> `Idle`입니다.
- `indentStep`의 기본값은 `8`이므로 깊이 수준이 추가될 때마다 왼쪽 패딩이 `8`만큼 증가합니다.
- 행 클릭은 활성화만 요청합니다. 행 클릭은 확장을 전환하지 않습니다.
- 쉐브론 클릭은 `expanded`를 토글한 다음 활성화가 허용되면 활성화를 요청합니다.
- `selectionDirection: "auto"`는 `expanded=false`를 오른쪽으로, `expanded=true`를 아래쪽으로 매핑합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.HierarchyItem {
    label: "Camera"
    iconName: "toolwindowhierarchy"
    indentLevel: 1
    count: 12
    showChevron: true
    activatable: true
    onDragEnded: function(committed, fromIndex, toIndex, targetDepth, modeName, parentItemKey) {
        if (committed)
            console.log("moved", fromIndex, toIndex, targetDepth, modeName, parentItemKey)
    }
}
```

<a id="shared-motion"></a>

## 공유 모션

상속된 인스턴스별 `interactionPhase` / `interactionInput` 트랙 입력은 `interactionState`, 선택, 확장 및 `uxState` 와 독립적으로 수행됩니다. `interaction.enabled` 는 비활성화 가능 행과 드래그 미리보기에 대해 거짓입니다. 모바일 환경에서 장시간 누름 포인터 피드백과 네이티브 스페이스/엔터 누름 피드백은 모두 소유된 누름 상태를 공급하므로, 편집 가능한 행에서 키보드 활성화가 여전히 작동합니다. Figma 의 접이식/비접이식 집합에는 완전한 입력 변형이 포함되어 있으며, 참조하십시오.
[구성 요소 인스턴스 상태](../../instance-states.md).

기본/호버/누름 색상은 `rowBackgroundColorIdle`, `rowBackgroundColorHover` (surfaceGhost) 및 `rowBackgroundColorPressed` (surfaceAlt) 을 계속 사용합니다. 활성 선택은 accentMuted 를 유지하며, 비활성 상태는 panelBackground12 를 유지합니다. 이 모델 상태는 키보드 포커스와는 별개입니다. 릴리즈는 버튼/메뉴/목록 행과 동일한 명시적 180ms OutBack 정책을 사용하며, 추가 채우기 또는 테두리가 없습니다.

1.5px 기본 포커스 링은 고정 행 바깥에 3px 위치하며, 스페이스/엔터 누름 및 릴리즈 동안 지속됩니다. 비활성화 가능 행은 포커스 링이나 변형을 가지지 않으면서 기존 독립적인 공개 동작을 유지합니다. 모바일 장시간 누름 입력은 콘텐츠 및 표면 변환 모두에 `effectivePressedState` 를 사용하며, 비활성 및 활성 드래그 미리보기는 변형을 억제합니다. 모바일 포인터를 행 바깥에서 릴리스하면 릴리스 리바운드가 없이 행 동작이 취소됩니다. 선택, 확장 및 드래그/드롭 모델 변경은 동기적으로 유지됩니다.

행이 미묘하게 압축되고 갈매기형 모양이 새로 확장된 방향으로 회전합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
