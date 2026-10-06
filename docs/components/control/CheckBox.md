# CheckBox

위치: `src/qml/components/control/check/CheckBox.qml`

`CheckBox`는 결정적 상태 시각적 기능을 갖춘 사용자 정의 페인트 확인란(`AbstractButton` 기반)입니다.

<a id="purpose"></a>

## 목적

- 플랫폼 스타일 변화와 별개로 체크박스 시각적 요소를 유지하세요.
- 명시적인 선택/선택 취소 + 활성화/비활성화 팔레트 및 테두리 정책을 노출합니다.

<a id="core-api"></a>

## 코어 API

상태:

- `checked`(상속됨)
- `enabled`(상속됨)
- `text`(상속됨)

형태 및 지표:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `boxSize`(데스크톱 `17 x 17`, 모바일 `34 x 34`)
- `framePadding`(`0.5`)는 데스크톱 및 모바일의 `18 x 18` 상호 작용 프레임 내부에 표시기를 유지합니다.
- `boxRadius`(`3.5`), `checkMarkStrokeWidth` 및 테두리 너비는 데스크톱 및 모바일에서 Figma 17px프레임 비율을 유지합니다.

팔레트 및 테두리:

- `checkedColor`, `uncheckedColor`
- `disabledCheckedColor`, `disabledUncheckedColor`
- `checkColor`, `checkMarkColorDisabled`
- `boxBorderWidth*`, `boxBorderColor*`
- `innerShadowSoftColor`, `innerShadowStrongColor`
- `useFigmaCheckedAssets`
- `checkedAssetSourceEnabled`, `checkedAssetSourceDisabled`

해결된 값:

- `resolvedCheckedFillColor`, `resolvedUncheckedFillColor`
- `resolvedCheckedAssetSource`, `usingFigmaCheckedAsset`
- `resolvedBoxRadius`
- `resolvedBoxBorderWidth`, `resolvedBoxBorderColor`
- `showInnerShadow`

<a id="behavior-contract"></a>

## 행동 계약

- `checkable: true`, `tone: Borderless`, 투명 배경 레이어.
- Figma 구성 요소 세트(`44:724`)에는 4개의 `57 x 18` 데스크톱 변형이 포함되어 있습니다. 선택/선택 취소 및 활성화/비활성화 교차.
- 표시기는 `(0.5, 0.5)`에서 시작하고, 레이블은 `(23.5, 2.5)`에서 시작하며, 표시기-레이블 간격은 `6px`입니다. 본문 텍스트는 `13px/13px` Medium에 고정되어 있습니다.
- 모바일은 데스크탑과 동일한 `17 x 17` 표시기, `0.5px` 프레임 패딩, `6px` 간격, `3.5px` 반경 및 `57 x 18` 레이블 경계를 사용합니다.
- 선택된 상태는 기본적으로 내보낸 정확한 Figma SVG 자산을 사용합니다. 활성화된 것은 80% 흰색 표시가 있는 `#0A84FF`입니다. 비활성화된 것은 30% 흰색 표시와 내보낸 내부 그림자 처리가 있는 `Theme.panelBackground12`입니다.
- 원본 파란 체크 색상에는 `useFigmaCheckedAssets` 가 기본값으로 true 로 설정되고, 사용자 정의 `checkedColor` 에는 false 로 설정되며, 앱의 사용자 정의 `ApplicationWindow.primaryColor` 를 포함합니다. 그리해진 체크 표시는 스톡 파란 이미지로 덮는 대신 구성된 채우기를 노출합니다. 사용자 정의 스냅샷 자산을 선택하려면 명시적으로 true 로 설정하거나, 사용자 정의 체크 표시 팔레트 속성을 선택하려면 false 로 설정합니다. Canvas 렌더러도 `shapeCylinder` 를 자동으로 선택합니다. 그의 업샘플링 백킹 스토어 ( `RenderQuality` + HiDPI ) 는 사용 가능하게 유지되며 상태/색상/스트로크 변경 및 렌더러 전환 시 다시 그립니다.
- `showInnerShadow`는 체크+활성화 시에만 비활성화됩니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.CheckBox {
    text: "Remember"
    checked: true
}
```

<a id="shared-motion"></a>

## 공유 모션

확인된 자산 또는 그려진 마크가 제자리에 고정됩니다. 채우기가 혼합되고 버튼이 리바운드됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
