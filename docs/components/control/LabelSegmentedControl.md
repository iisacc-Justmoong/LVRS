# LabelSegmentedControl

위치: `src/qml/components/control/buttons/LabelSegmentedControl.qml`

`LabelSegmentedControl`는 `LabelButton` 하위 항목을 위한 Figma 라벨 분할 컨테이너입니다.

<a id="purpose"></a>

## 목적

- 레이블 버튼에 대해 `IconSegmentedControl`와 동일한 분할된 셸 동작을 제공합니다.
- 분할된 톤 정책과 크기 조정을 일관되게 유지합니다.

<a id="core-api"></a>

## 코어 API

컨테이너 스타일:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`(기본값 `Theme.radiusLg`, `12`), `resolvedCornerRadius`
- `horizontalPadding`, `verticalPadding`, `spacing`
- `borderWidth`, `borderColor`
- `backgroundColor`

행동:

- `forceBorderlessTone`(기본값 `true`): 자식의 `tone`을 바꾸지 않고 무테 외관을 적용
- `segmentCount`(읽기 전용)
- `default property alias buttons: segmentRow.data`
- 주입 방법 API: `method`, `methods`, `hasInjectedMethods`, `invokeMethods(...)`

<a id="behavior-contract"></a>

## 행동 계약

- 톤이 가능한 하위 항목은 세그먼트로 처리됩니다. Figma에는 2–15개 변형이 있으며 코드의 자식 수에 별도 상한은 없습니다.
- 표준 버튼은 원래 `tone`을 유지하면서 무테 외관을 적용받습니다. `forceBorderlessTone`을 끄거나 버튼을 옮기면 원래 외관이 복원됩니다.
- 하위 버튼 크기는 `LabelButton`의 소유로 유지됩니다. 세그먼트 패딩이나 간격이 변경되면 행이 다시 배치되므로 업데이트된 데스크톱/모바일 버튼 측정항목이 컨테이너에 전파됩니다.
- 컨테이너 자체는 직접 오케스트레이션을 위해 삽입된 메서드를 수신할 수 있습니다. 하위 버튼 클릭은 각 하위 버튼의 소유로 유지됩니다.

선택 상태는 일반 버튼과 같은 `checkable`·`checked` 계약을 사용합니다. 선택된 버튼은
타입을 바꾸지 않고 앱의 `Theme.accentMuted`(악센트의 Selected 색상)를 배경으로 그립니다.
앱의 `Theme.primaryColor`가 바뀌면 선택 배경도 갱신됩니다. 자동 단일 선택이 필요하면
자식 버튼의 `autoExclusive`를 사용하거나 앱의 선택 모델에 `checked`를 바인딩합니다.

<a id="figma-visual-contract"></a>

## Figma 시각적 계약

- 출처: [Figma `206:3827`(`LabelSegementedControl`)](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=206-3827), 변형 갱신 2026-10-30.
- 각 세그먼트는 경계 없는 `44:599` `LabelButton`: `56 x 22`, 수평 패딩 `8`, 몸체 `13px Medium / 13px`입니다.
- 컨테이너: 가로 패딩 `4`, 세로 패딩 `3.5`, 간격 `2`, 테두리 `2`, 반경 `12`(`Theme.radiusLg`). 하위 버튼은 반경 `8`(`Theme.radiusMd`)를 유지합니다.
- 표면: `Theme.panelBackground08`; 국경: `Theme.panelBackground12`.

|개수|데스크탑 크기|
| ---: | ---: |
| 2 | `122 x 29` |
| 3 | `180 x 29` |
| 4 | `238 x 29` |
| 5 | `296 x 29` |
| 6 | `354 x 29` |
| 7 | `412 x 29` |
| 8–15 | `470–876 x 29` |

데스크톱과 모바일은 `count * 56 + (count - 1) * 2 + 8` 너비를 사용합니다. 2섹먼트 컨트롤은 `122 x 29` 이며, 2 `56 x 22` 버튼, `2` 간격, `4` 수평 패딩, 및 `3.5` 수직 패딩을 가집니다. 바디는 `13px` 를 유지합니다.

<a id="verification"></a>

## 검증

`LVRSTests_import_api::segmented_control_figma_contract_loads` 는 데스크톱 및 모바일 테마 설정 하에 있는 2–15개 버튼을 확인하며, 컨테이너/버튼 반지름, 경계, 톤, 패딩, 간격, 및 색상을 포함합니다. 네이티브 Qt 플랫폼으로 실행할 때 렌더링된 참조 이미지를 저장하려면 `LVRS_SEGMENT_CAPTURE_DIR` 를 설정합니다. 모바일 행은 호스트의 공유 테마 계약을 확인하며, 이는 기기 실행이 아닙니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.LabelSegmentedControl {
    method: function(eventData) { syncSelectionModel() }

    LV.LabelButton { text: "A"; checkable: true; autoExclusive: true; checked: true }
    LV.LabelButton { text: "B"; checkable: true; autoExclusive: true }
}
```

<a id="shared-motion"></a>

## 공유 모션

하위 버튼은 개별적으로 반응합니다. 행은 명시적인 간격 변경에도 애니메이션을 적용합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
