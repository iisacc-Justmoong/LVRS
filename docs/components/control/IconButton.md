# IconButton

위치: `src/qml/components/control/buttons/IconButton.qml`

`IconButton`는 [PushButton](PushButton.md)의 `iconMode: true` 사전 설정이며 선택적 글리프/텍스트가 포함되어 있습니다.

<a id="purpose"></a>

## 목적

- 프로젝트 구조 대체 경로 아이콘과 함께 컴팩트 아이콘 작업 버튼을 제공합니다.
- URL, 아이콘 이름 토큰 또는 문자 모양 텍스트로 아이콘 소스를 지원합니다.

## API

아이콘 입력:

- `iconSource`(내부 `url`에 대한 별칭)
- `iconName`
- `iconGlyph`
- `iconSize`

계산된/아이콘 해상도:

- `resolvedIconName`
- `resolvedIconSource`

레이아웃:

- 기본 톤: `Primary` (`Kind=accent`)
- 기본 아이콘 프레임: `Theme.iconSm`(데스크톱에서는 `18 x 18`, 모바일에서는 `36 x 36`)
- 고정 프레임: 데스크톱의 경우 `22 x 22`, 모바일의 경우 `44 x 44`
- 소형 삽입: `Theme.gap2`(`2` 데스크탑, `4` 모바일)
- `cornerRadius: Theme.radiusMd`(`8` 데스크톱, `16` 모바일)
- `spacing`: 악센트용 `0`, 기타 Figma 톤용 `Theme.gap7`
- `iconGlyph`는 SVG 아이콘과 동일한 사각형 프레임을 사용합니다.

<a id="figma-visual-contract"></a>

## Figma 시각적 계약

- 출처: `44:599`, `Type=IconButton`.
- 아이콘은 데스크탑 `22 x 22` 프레임 내부의 `x=2, y=2, 18 x 18`를 차지합니다.
- 대체 경로 `generalprojectStructure.svg`는 Figma의 `#CED0D6` 폴더 및 `#548AF7` 수정자 형상과 일치하며 대체 자산 없이 재사용됩니다.
- 모든 5 톤은 동일한 기하학을 유지하며, 프로젝트 구조 아이콘은 자체 색상을 유지합니다.

주입된 방법:

- `AbstractButton`에서 상속됨: `method`, `methods`, `hasInjectedMethods`, `invokeMethods(...)`

<a id="resolution-order"></a>

## 해결 순서

1. 명시적 `iconSource`
2. 명시적 `iconName`
3. 그룹화된 아이콘 API의 `icon.name`
4. 기본 대체 경로 아이콘(`projectStructure`)

렌더링된 소스는 `RenderQuality.resolveTextureSource(...)`를 통해 확인됩니다. 정사각형 슈퍼샘플링 `sourceSize`는 논리적 아이콘 크기와 장치 픽셀 비율을 추적합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.IconButton {
    iconName: "add"
    method: function(eventData) {
        addItem()
    }
}
```

<a id="advanced-example-glyph-fallback-icon"></a>

## 고급 예: Glyph 대체 경로 아이콘

```qml
import LVRS 1.0 as LV

LV.IconButton {
    iconGlyph: "+"
    iconSize: 14
    text: "Add"
}
```

위의 명시적인 `14`는 지원되는 인스턴스별 재정의를 보여줍니다. 재고 기본값은 `Theme.iconSm`(데스크톱의 경우 `18`, 모바일의 경우 `36`)로 유지됩니다.

<a id="troubleshooting"></a>

## 문제 해결

아이콘이 렌더링되지 않는 경우:

1. 아이콘 이름이 아이콘 세트에 있는지 확인하십시오.
2. `Theme.iconPath(...)`에서 리소스 경로를 확인하고,
3. SVG 관리자 개정 업데이트가 차단되지 않았는지 확인하십시오.

<a id="recipe-toolbar-icon-action--tooltip"></a>

## 레시피: 툴바 아이콘 액션 + 툴팁

```qml
import QtQuick
import LVRS 1.0 as LV

LV.IconButton {
    id: refreshButton
    iconName: "refresh"
    tone: LV.AbstractButton.Borderless
    ToolTip.visible: hovered
    ToolTip.text: "Refresh"
}
```

<a id="production-notes"></a>

## 생산 노트

- 아이콘 이름을 의미상 안정적으로 유지하고 불투명한 일회성 이름을 피하세요.
- 인접한 컨트롤과의 일관성을 위해 디자인 토큰 아이콘 크기 값을 선호합니다.

<a id="shared-motion"></a>

## 공유 모션

아이콘은 작성된 프레임이 고정된 상태로 유지되는 동안 텍스트 버튼과 동일한 누르기 리듬을 사용합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
