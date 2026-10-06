# ContextMenu

위치: `src/qml/components/navigation/ContextMenu.qml`

`ContextMenu`는 런타임로 조정된 개방형 애니메이션과 전역 외부 해제 브리징이 포함된 팝업 메뉴입니다.

<a id="purpose"></a>

## 목적

- 이질적인 메뉴 모델(문자열/객체/분할기)을 렌더링합니다.
- 정규화된 이벤트 신호와 선택적 콜백 컨텍스트를 내보냅니다.
- 오버레이/글로벌 이벤트 경로 전반에 걸쳐 긴밀한 행동을 결정적으로 유지합니다.

<a id="core-api"></a>

## 코어 API

데이터 및 선택:

- `items`
- `selectedIndex`
- `entryCount`(읽기 전용)

열기/닫기 동작:

- `autoCloseOnTrigger`
- `dismissOnGlobalPress`
- `dismissOnGlobalContextRequest`
- `openAt(x, y)`
- `openFor(item, x, y)`
- `dismissIfOutsideGlobalEvent(eventData)`
- `triggerEntry(index)`

시각적/레이아웃:

- `itemWidth`, `itemSpacing`(기본값 `Theme.gap2`)
- `resolvedItemWidth`(읽기 전용)
- `showIconSlot`(기본 `true`, 기본 `ContextMenuItem` 행으로 전달됨)
- `itemDelegate`: 비분할 항목에 사용되는 선택적 구성 요소입니다.
- `dividerDelegate`: 구분선 항목에 사용되는 선택적 구성 요소입니다.
- `menuColor`, `menuOpacity`, `resolvedMenuColor`(기본 창 배경색, 12% 표면 색조)
- `menuBlurRadius`(64 논리 픽셀), `menuAccentStrength`(표준 방사형 액센트 코팅의 0.08)
- `primaryColor`, `backdropSource`, `backdropBackground`: 창 악센트 및 안전한 창 내 캡처 소스; 전경 내용은 불투명하게 유지됩니다.
- `dividerColor`(기본 `Theme.contextMenuDivider`, 30% 흰색)
- `edgeMargin`(자동 배치에 사용되는 뷰포트 삽입, 기본값 `Theme.gap4`)
- `openHorizontalDirection`, `openVerticalDirection`(마지막으로 확인된 배치 방향)

애니메이션 조정:

- `enableOpenBounce`, `autoTuneByBackend`
- `openBounceDuration`, `openSettleDuration`
- `openStartScale`, `openOvershootScale`
- 해결됨: `resolvedOpenBounceEnabled`, `resolvedOpenReboundDuration`, `resolvedOpenBackOvershoot`

신호:

- `itemTriggered(index, item)`
- `itemEventTriggered(eventName, payload, index, item)`

<a id="entry-object-contract"></a>

## 참가 개체 계약

지원되는 개체 필드는 다음과 같습니다.

- 표준 압축 항목 필드: `icon`, `label`, `keyVisible`, `key`
- 라벨/텍스트: `label`, `text`, `title`
- 아이콘: `iconName`/`icon`, `iconSource`/`source`
- 아이콘 슬롯 가시성: `showIconSlot` 또는 `iconSlotVisible`
- 핵심 텍스트: `key`, `shortcut`, `keyText`
- 주요 가시성 : `keyVisible`, `shortcutVisible`, `showShortcut`
- 상태: `enabled`, `state`, `selected`
- 쉐브론/어린이: `showChevron`, `hasChildItems`, `hasSubmenu`, `expanded`, `selectionDirection`
- 이벤트: `eventName`/`event`/`action`, `eventPayload`/`payload`, `events[]`
- 콜백: `onTriggered`, `onClicked`, `handler`
- 정책 닫기: `closeOnTrigger`, `autoClose`, `keepOpen`, `preventClose`
- 분배기: `type: "divider"` 또는 `divider: true`

콜백은 컨텍스트 `{ index, item, menu, eventName, payload, emit(), close() }`를 수신합니다.

체vron 렌더링 조건은 `showChevron && hasChildItems` (엔트리 필드에서 해결됨) 입니다. 단축키 가시성은 단축키 텍스트가 존재할 때만 `true` 로 기본값이며, `key` / `shortcut` / `keyText` 가 없는 엔트리는 명시적으로 요청되지 않는 한 후미 단축키 공간을 예약하지 않습니다. 아이콘 슬롯 가시성은 메뉴 레벨 `showIconSlot` 값으로 기본값입니다. 개별 엔트리는 `showIconSlot` 또는 호환성 별칭 `iconSlotVisible` 로 이를 덮어쓸 수 있습니다.

<a id="delegate-contract"></a>

## 계약위임

- 구분자가 아닌 각 항목은 `itemDelegate`를 인스턴스화합니다. 각 구분선 항목은 `dividerDelegate`를 인스턴스화합니다.
- 두 대리자 모두 주입된 `modelData` 개체 하나를 받고 `property var modelData`를 선언해야 합니다.
- `modelData`에는 `index`, `entry`, `divider`, `label`, `shortcut`, `keyVisible`, `showIconSlot`, `iconName`, `iconSource`, `showChevron`, `hasChildItems`, `expanded`, `selectionDirection`, `enabled`, `state` 및 `trigger()`.
- `modelData.trigger()`는 `triggerEntry(index)`를 호출하고, 정규화된 메뉴 신호를 내보내고, 콜백/이벤트를 실행하고, 닫기 정책을 적용합니다.
- 사용자 정의 대리자는 자신의 클릭/탭 동작을 `modelData.trigger()`에 연결하는 일을 담당합니다.

<a id="placement-contract"></a>

## 배치 계약

- `openAt(x, y)`는 오버레이 경계에 대해 내장된 가장자리 인식 배치를 수행합니다.
- 기본 설정은 기준점에서 오른쪽/아래쪽입니다.
- 공간이 부족하면 배치가 왼쪽 및/또는 위로 뒤집힙니다.
- 최종 위치는 `edgeMargin`를 사용하여 뷰포트에 고정됩니다.
- `resolveOpenPlacement(...)`는 결정론적 테스트 및 툴링을 위한 내부 배치 솔버를 공개합니다.

<a id="width-contract"></a>

## 폭 계약

- `itemWidth`는 기준 최소 행 너비입니다.
- `resolvedItemWidth`는 기본 `itemWidth`, 가장 넓은 대리자 암시적 너비 및 호출자가 제공한 명시적 팝업 너비 중 더 큰 너비로 확장됩니다.
- 팝업 프레임 자체는 최소한 `implicitWidth`로 승격되므로 좁은 명시적 `width`는 메뉴를 콘텐츠 중심 크기 아래로 고정할 수 없습니다.
- 대리자 행 및 구분선은 `resolvedItemWidth`를 사용하므로 콤보 트리거 메뉴는 콘텐츠 또는 발신자 크기 조정이 필요할 때 트리거 자체보다 더 넓어질 수 있습니다.
- 대리자 `MenuItem` 행은 `resolvedItemWidth` 내에서 응답 상태를 유지하므로 제한된 메뉴가 레이블/바로가기/갈매기 모양 콘텐츠를 팝업 경계 외부로 푸시하거나 내부 스페이서를 네거티브 형상으로 축소하지 않습니다.
- 너비 검색은 각 행의 제한되지 않은 자연 콘텐츠 너비를 사용하므로 표시되는 텍스트 제거는 팝업 크기 조정에 피드백되지 않습니다.
- 너비 프로빙은 렌더링된 행과 동일한 컴팩트/일반 글꼴 계약을 사용합니다. 더 넓은 크롬이 필요한 사용자 정의 대리자는 `itemWidth` 또는 명시적 팝업 `width`를 설정해야 합니다.
- 너비 프로빙은 렌더링된 대리자와 동일한 아이콘 슬롯 가시성을 전달하므로 `showIconSlot: false`는 숨겨진 선행 여백을 남기는 대신 콘텐츠 중심 메뉴 너비를 줄입니다.

<a id="visual-contract"></a>

## 시각적 계약

- Figma 소스: `331:9332`, `331:9282` 및 `331:9283`로 구성됩니다.
- 최소 항목 너비 141px, 모든 4 측면의 8px 패딩, 2px 행 간격입니다.
- 5개의 18px 행과 1개의 3px 구분선이 참조 157 × 119px 팝업을 생성합니다.
- 컴팩트 라벨은 Inter 일반 12를 사용하고, 바로가기는 Pretendard SemiBold 12를 사용합니다.
- 구분선은 4px 수평 삽입과 함께 30% 불투명도의 흰색을 사용합니다.
- 팝업에서는 메뉴 반경이 12px인 WindowMaterial, 색조 12%, 흐림 효과가 64px, 표준 악센트 그라데이션 강도의 8%가 사용됩니다.
- 별도의 Figma 메뉴 계열(110:857)의 경우 메뉴: 24px 행, 4px 수직 패딩 및 panel08 전폭 분할기를 사용합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.ContextMenu {
    id: menu
    showIconSlot: false
    items: [
        { label: "Copy", eventName: "menu.copy", showChevron: false },
        { type: "divider" },
        { label: "Inspect", showIconSlot: true, keepOpen: true, events: ["menu.inspect"] }
    ]
}
```

```qml
Component {
    id: destructiveMenuRow

    LV.MenuItem {
        property var modelData: ({})
        label: modelData.label || ""
        enabled: modelData.enabled === true
        onClicked: modelData.trigger()
    }
}

LV.ContextMenu {
    itemDelegate: destructiveMenuRow
    items: [{ label: "Delete", eventName: "delete" }]
}
```

<a id="shared-motion"></a>

## 공유 모션

메뉴는 리바운드를 통해 해결된 원점에서 축소된 다음 해제 시 페이드됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

Figma 컴팩트 기본값은 141px 최소 항목 너비, 18px 행, 모든 측면의 8px 패딩을 사용합니다. 5 참조 행과 하나의 구분자는 157 × 119px를 생성합니다. `compactItems` 는 기본값이 참이며 일반적으로 너비 탐사 타이포그래피도 제어합니다. `Menu` 를 사용하여 규칙적인 24px 행과 4px 수직 패딩을 보존합니다.

<a id="window-derived-frosted-menu"></a>

## 창 파생 프로스트 메뉴

2026-09-12 시각적 수정안은 사용자의 글래스 처리 요청을 따르며 이전 Figma 글래스 25 메뉴 코팅을 대체합니다. ContextMenu 와 메뉴는 WindowMaterial , 윈도우의 채움 색상과 주요 팔레트, 그리고 48px/16px/30% 높이를 공유합니다. 컴팩트/규칙 행 레이아웃은 이 소재와 독립적입니다.

`menuOpacity` 는 중성 톤만 제어하며 (기본값 0.12 ), `menuBlurRadius` 는 배경 블러를 제어하며 (64 로직 픽셀), `menuAccentStrength` 는 추가 원형 코팅을 확대합니다 (0.08 ). 포착된 창은 이미 강조 조명 정보를 포함하고 있으므로, 두 번째 풀 강도 레이어를 추가하면 메뉴가 과도하게 파랗게 변합니다. 유리 테두리와 내장 하이라이트는 그대로 표시됩니다. 팝업 불투명도는 1로 유지되어 레이블과 선택된 행이 완전히 불투명하게 유지됩니다. 명시적 menuColor , 배경 소스 및 해당 3 재료 값은 여전히 덮어쓸 수 있습니다.

네이티브 자료 테스트에서는 기존 25%/16px/풀 액센트 처리를 새로운 처리와 비교하고, 감소된 파란색 농도를 측정하고, 실제 배경 확산을 확인하고, 메뉴 유형과 재정의를 모두 테스트합니다.

기본 콘텐츠 포착은 `ApplicationWindow.materialBackdropSource` 를 사용하며, 이는 팝업 오버레이를 제외합니다. 전체 Qt 창 콘텐츠 조상을 포착하는 것은 피드백을 피하기 위해 MaterialSurface 에 의해 거부되며, 안전한 앱 콘텐츠 호스트를 사용하면 메뉴 뒤의 텍스트와 제어가 실제로 블러에 참여합니다. 다른 창 구현체는 명시적인 안전한 `backdropSource` 를 계속 제공할 수 있습니다.

포착은 테마 materialTint , 불투명한 중성 베이스 위에 합성된 후 확산됩니다. 이 베이스를 중성으로 유지하면 컬러 윈도우 필드가 두 번째 불투명한 강조 레이어로 변하는 것을 방지합니다. 이는 덮인 앱 텍스트의 두 번째, 블러되지 않은 사본이 포착 알파를 통해 보일 것을 방지합니다. 네이티브 테스트는 고립된 효과 텍스처뿐만 아니라 스트립 패턴 위의 최종 가시 윈도우 픽셀을 측정합니다.

macOS / Metal 에서 2026-09-12에서 확인되었으며: 모든 53 LVRS CTest 사례와 모든 9 네이티브 재료 검사가 통과했습니다. 설치된 런타임 는 83 소스 QML 파일과 일치합니다. Society ( 16 테스트) 및 Dreamscapes ( 5 테스트)가 재구성되어 테스트 수트를 통과한 후 다시 시작되었으며, 생성 횟수 메뉴는 기본 콘텐츠를 가시적으로 확산시키면서 명확한 레이블과 선택을 유지합니다. 두 실행 중인 앱이 업데이트된 워크스페이스 런타임 를 로드했습니다. VisualCatalog 도 동일한 런타임 로 재구성되어 다시 시작되었습니다.
