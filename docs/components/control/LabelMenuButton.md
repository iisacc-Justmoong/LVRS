# LabelMenuButton

위치: `src/qml/components/control/buttons/LabelMenuButton.qml`

`LabelMenuButton`는 [DropdownButton](DropdownButton.md)의 라벨 사전 설정입니다.

<a id="purpose"></a>

## 목적

- 컴팩트한 텍스트 우선 메뉴 트리거를 제공합니다.
- 신호음 및 활성화된 상태와 일관되게 표시기 동작을 유지합니다.

<a id="core-api"></a>

## 코어 API

텍스트 및 어조:

- `text`(상속됨)
- `tone`(상속됨, 기본값: `Primary`, `Kind=accent`와 일치)
- `AbstractButton`에서 상속된 주입 메서드 API: `method`, `methods`, `invokeMethods(...)`

표시기:

- `indicatorSize`(읽기 전용, 데스크톱 및 모바일의 경우 `Theme.iconSm`: `18 x 18`)
- 톤/활성화 상태의 `resolvedIndicatorName`
- `Theme.iconPath(...)`를 통해 렌더링됨
- 슈퍼샘플링 인식 아이콘 소스 크기(`indicatorSourceSize`)

표시기 아이콘 매핑:

- 기본값: `generalchevronDown`
- 경계선 없음: `generalchevronDownBorderless`
- 기본/파괴: `generalchevronDownAccent`
- 비활성화됨: `generalchevronDownDisabled`

레이아웃:

- 고정 `figmaButtonHeight`: `22` 데스크톱, `44` 모바일
- 왼쪽 삽입: `horizontalPadding: Theme.gap8`(`8` 데스크톱, `16` 모바일)
- 독립적인 오른쪽 삽입: `rightPadding: Theme.gap2`(`2` 데스크톱, `4` 모바일)
- `cornerRadius: Theme.radiusMd`(`8` 데스크톱, `16` 모바일)
- `verticalPadding: Theme.gap2`(`2` 데스크톱, `4` 모바일)
- 라벨-쉐브론 간격: `spacing: Theme.gapNone`(`0`)

<a id="figma-visual-contract"></a>

## Figma 시각적 계약

- 출처: `700:337`, `Type=LabelMenuButton`.
- 텍스트 `Open`를 사용하면 데스크탑 프레임은 `60 x 22`, 텍스트 `x=8, y=4.5, 32 x 13`, 쉐브론 `x=40, y=2, 18 x 18`입니다.
- 데스크톱과 모바일에서는 본문 타이포그래피가 `13px Medium / 13px`로 고정되어 있습니다.
- 해당 모바일 토큰 구성은 `88 x 44`입니다.
- 기존 톤별 `generalchevronDown*.svg` 자산은 Figma 벡터와 일치하며 재사용됩니다.
- 명시적 너비가 암시적 프레임보다 좁은 경우 갈매기형 표시는 오른쪽 삽입 내부에 남아 있는 동안 레이블은 사라집니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.LabelMenuButton {
    text: "Options"
    method: function(eventData) {
        menu.open()
    }
}
```

<a id="shared-motion"></a>

## 공유 모션

트리거는 클릭되거나 삽입된 메서드 콜백을 지연시키지 않고 리바운드됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
