# IconSegmentedControl

위치: `src/qml/components/control/buttons/IconSegmentedControl.qml`

`IconSegmentedControl`는 `IconButton` 하위 항목을 위한 Figma 아이콘 세그먼트 컨테이너입니다.

<a id="purpose"></a>

## 목적

- 공유 분할 쉘(배경, 테두리, 패딩, 간격)을 제공합니다.
- 필요한 경우 하위 버튼 톤을 표준화합니다.

<a id="core-api"></a>

## 코어 API

컨테이너 스타일:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`(기본값 `Theme.radiusLg`, `12`), `resolvedCornerRadius`
- `horizontalPadding`, `verticalPadding`, `spacing`
- `borderWidth`, `borderColor`
- `backgroundColor`

행동:

- `orientation`: `Qt.Horizontal`(기본값) 또는 `Qt.Vertical`; 실행 중 변경 가능
- `forceBorderlessTone`(기본값 `true`): 자식의 `tone`을 바꾸지 않고 무테 외관을 적용
- `segmentCount`(읽기 전용)
- `default property alias buttons: segmentRow.data`
- 주입 방법 API: `method`, `methods`, `hasInjectedMethods`, `invokeMethods(...)`

<a id="behavior-contract"></a>

## 행동 계약

- `tone`을 노출하는 하위 항목은 세그먼트 버튼으로 처리됩니다. Figma에는 2–15개 변형이 있으며 코드의 자식 수에 별도 상한은 없습니다.
- `forceBorderlessTone == true`일 때 표준 버튼은 원래 `tone`을 유지하면서 무테 외관을 사용합니다.
- 하위 변경 사항은 `Qt.callLater`(`scheduleSyncSegmentStyles`)로 동기화되며, 부모에서 제거되면 임시 외관은 해제됩니다.
- 하위 버튼 크기는 `IconButton`의 소유로 유지됩니다. 세그먼트 토큰 변경으로 인해 행 재배치가 강제되므로 컨테이너는 업데이트된 데스크톱/모바일 버튼 측정항목을 따릅니다.
- 컨테이너는 `invokeMethods(...)`를 통해 주입된 메서드를 실행할 수 있습니다. 하위 버튼에는 여전히 고유한 클릭 방법이 있습니다.

선택 상태는 일반 버튼과 같은 `checkable`·`checked` 계약을 사용합니다. 선택된 버튼은
타입을 바꾸지 않고 앱의 `Theme.accentMuted`(악센트의 Selected 색상)를 배경으로 그립니다.
앱의 `Theme.primaryColor`가 바뀌면 선택 배경도 갱신됩니다. 자동 단일 선택이 필요하면
자식 버튼의 `autoExclusive`를 사용하거나 앱의 선택 모델에 `checked`를 바인딩합니다.

<a id="figma-visual-contract"></a>

## Figma 시각적 계약

- 출처: [Figma `206:3912`(`IconSegementedControl`)](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=206-3912), 변형 갱신 2026-10-30.
- 각 세그먼트는 `18 x 18` 아이콘과 `2` 삽입이 포함된 경계선 없는 `44:599` `IconButton`: `22 x 22`입니다.
- 컨테이너: 가로/세로 패딩 `4`, 간격 `2`, 테두리 `2`, 반경 `12`(`Theme.radiusLg`). 하위 버튼은 반경 `8`(`Theme.radiusMd`)를 유지합니다.
- 표면: `Theme.panelBackground08`; 국경: `Theme.panelBackground12`.

|개수|데스크탑 크기|
| ---: | ---: |
| 2 | `54 x 30` |
| 3 | `78 x 30` |
| 4 | `102 x 30` |
| 5 | `126 x 30` |
| 6 | `150 x 30` |
| 7 | `174 x 30` |
| 8–15 | `198–366 x 30` |

데스크톱과 모바일은 `count * 22 + (count - 1) * 2 + 8` 너비를 사용합니다. 2세그먼트 컨트롤은 `54 x 30`이며, 세로형은 `30 x 54`입니다. 세로형은 가로형의 너비와 높이가 바뀌며 15개일 때 `30 x 366`입니다.

<a id="verification"></a>

## 검증

`LVRSTests_import_api::segmented_control_figma_contract_loads` 는 데스크톱 및 모바일 테마 설정 하에 있는 2–15개와 세로 방향을 확인하며, 컨테이너/버튼 반지름, 경계, 톤, 패딩, 간격, 및 색상을 포함합니다. 네이티브 Qt 플랫폼으로 실행할 때 렌더링된 참조 이미지를 저장하려면 `LVRS_SEGMENT_CAPTURE_DIR` 를 설정합니다. 모바일 행은 호스트의 공유 테마 계약을 확인하며, 이는 기기 실행이 아닙니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.IconSegmentedControl {
    orientation: Qt.Vertical
    methods: [function(eventData) { syncToolbarState() }]

    LV.IconButton { iconName: "projectStructure"; checkable: true; autoExclusive: true }
    LV.IconButton { iconName: "projectStructure"; checkable: true; autoExclusive: true }
}
```

<a id="shared-motion"></a>

## 공유 모션

선택한 아이콘 버튼은 레이블 세그먼트와 동일한 리바운드 및 키보드 포커스 피드백을 받습니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
