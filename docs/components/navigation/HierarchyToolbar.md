# HierarchyToolbar

위치: `src/qml/components/navigation/HierarchyToolbar.qml`

`HierarchyToolbar`는 `IconButton` 배열로 렌더링되는 계층 구조 패널의 상단 도구 모음입니다.

<a id="purpose"></a>

## 목적

- 외부 배열 모델에서 도구 모음 버튼을 렌더링합니다.
- 활성 슬롯 선택을 결정적으로 유지합니다(`activeIndex`, `activeButtonId`).
- 각 도구 모음 슬롯에서 ID 기반 및 이벤트 기반 작업을 전달합니다.

## API

모델 및 상태:

- `buttonItems`(어레이 또는 모델)
- `itemCount`
- `buttonCount`
- `activeButton`
- `activeButtonId`
- `activeIndex`

레이아웃 및 모양:

- `minimumToolbarWidth`
- `horizontalPadding`
- `verticalPadding`
- `spacing`
- `slotSize`
- `distributeSpacing`
- `backgroundColor`
- `backgroundOpacity`
- `visibleButtonCount`
- `distributedSpacing`

신호:

- `activeChanged(button, buttonId, index)`
- `buttonTriggered(button, buttonId, index, item)`
- `buttonEventTriggered(eventName, payload, index, item, buttonId)`

방법:

- `triggerIndex(index)`
- `buttonAt(index)`
- `collectButtons()`

호환성:

- `buttons` 기본 속성 별칭은 수동 하위 항목에 대해 유지됩니다.
- 수동 `IconButton` 선언은 도구 모음 슬롯으로 처리됩니다. 선언된 개수는 도구 모음 버튼 개수와 같습니다.
- 수동 `IconButton` 클릭은 도구 모음 활성화에 자동으로 연결되고 기본/경계 없는 활성 톤 전환을 수신합니다.

<a id="figma-layout-baseline"></a>

## Figma 레이아웃 기준선

기본 레이아웃은 Figma(`Layerd Visual Render System`, 노드 `180:1011`)의 `HierarchyToolbar`를 따릅니다.

- 도구 모음에는 데스크탑 및 모바일의 `200 x 26` 기준선이 있습니다.
- `minimumToolbarWidth`의 기본값은 데스크톱에서는 `200`이고 모바일에서는 `400`입니다.
- 기본 패딩은 데스크톱에서 수평 `8px` 및 수직 `2px`입니다(모바일에서는 `16px` 및 `4px`).
- 각 도구 모음 슬롯은 상속된 버튼 프레임(데스크톱의 경우 `22 x 22`, 모바일의 경우 `44 x 44`)을 따릅니다.
- 각 주식 아이콘은 데스크톱에서 `Theme.iconSm`: `18 x 18`를 사용하고 모바일에서는 `36 x 36`를 사용합니다.
- 항목은 간격 없이 선행 삽입과 인접해 있습니다. 4개의 참조 슬롯은 데스크탑 x 위치 `8`, `30`, `52` 및 `74`에서 시작됩니다.
- `distributeSpacing`의 기본값은 `false`입니다. 호스트에 의도적으로 분산 슬롯이 필요한 경우에만 `true`로 설정하세요.
- 기본 배경은 투명합니다(`backgroundOpacity = 0`).

사용자 지정 고정 간격의 경우 `distributeSpacing: false`를 유지하고 `spacing`를 명시적으로 설정합니다.

<a id="item-model-contract"></a>

## 아이템 모델 계약

각 항목은 객체 또는 문자열일 수 있습니다.

개체 필드:

- `id` / `buttonId` / `key`
- `iconName` / `icon` (svg 아이콘 이름)
- `iconSource` / `source` / `url`(직접 소스 옵션)
- `enabled`
- `visible`
- `selected` / `active`
- `eventName` / `event` / `action`
- `eventPayload` / `payload`
- `events`(문자열 또는 이벤트 객체의 배열)
- `onTriggered` / `onClicked` / `handler`

문자열 항목:

- `iconName`로 취급

<a id="callback-context"></a>

## 콜백 컨텍스트

콜백이 존재하면 툴바는 다음을 사용하여 콜백을 호출합니다.

- `index`
- `item`
- `button`
- `buttonId`
- `toolbar`
- `eventName`
- `payload`
- `emit(eventName, payload)`
- `activate(index)`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.HierarchyToolbar {
    id: toolbar
    buttonItems: [
        {
            id: "structure",
            iconName: "projectStructure",
            selected: true,
            eventName: "hierarchy.structure"
        },
        {
            id: "layers",
            iconName: "projectStructure",
            events: [
                "hierarchy.layers",
                { name: "analytics.hierarchy.layers", payload: ({ source: "toolbar" }) }
            ],
            onClicked: function(ctx) {
                ctx.emit("hierarchy.layers.clicked", ({ id: ctx.buttonId }))
            }
        }
    ]
}
```

<a id="failure-pattern"></a>

## 실패 패턴

`buttonItems` 객체 항목이 `iconName` 및 `iconSource`를 모두 생략하는 경우 슬롯은 `IconButton`의 대체 경로 아이콘 동작으로 렌더링됩니다. 예측 가능한 시각적 개체에는 항상 명시적인 아이콘 이름을 제공하세요.

<a id="empty-and-stable-selection"></a>

## 비어 있고 안정적인 선택

빈 툴바는 `activeButton: null`, `activeButtonId: -1`, 및 `activeIndex: -1` 에 정착합니다. 변경되지 않은 선택을 정규화하면 또 다른 ID 변경을 발생시키거나 또 다른 이벤트 루프 패스를 예약하지 않습니다. Qt QML 에서 숫자 `var` 를 재할당하는 경우 값이 같아도 통지를 보낼 수 있으므로, 빈 계층 구조 패널이 반응형이 되도록 해당 할당을 보호하십시오. `hierarchy_toolbar_empty_state_does_not_reschedule` 경우 `LVRSTests_import_api` 에서 통지 및 정착된 큐를 모두 확인합니다.

<a id="shared-motion"></a>

## 공유 모션

각 ToolbarButton는 도구 모음의 크기를 조정하거나 이동하지 않고도 로컬로 반응합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
