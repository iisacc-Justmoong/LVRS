# IconMenuButton

위치: `src/qml/components/control/buttons/IconMenuButton.qml`

`IconMenuButton`는 [DropdownButton](DropdownButton.md)의 `iconMode: true` 사전 설정이며 끝에 V 표시가 있습니다.

<a id="purpose"></a>

## 목적

- 아이콘 중심 도구 모음을 위한 컴팩트 메뉴 트리거를 제공합니다.
- 아이콘 대체 경로 및 표시기 렌더링을 결정적으로 유지합니다.

<a id="core-api"></a>

## 코어 API

기본 아이콘:

- `iconSource`(`url`의 별칭)
- `iconName`
- `iconGlyph`(텍스트 문자 대체 경로)
- `iconSize`

표시기:

- `indicatorSize`(읽기 전용, `Theme.iconSm`)
- 톤/활성화 인식 표시기 선택(`resolvedIndicatorName`)
- `Theme.iconPath(...)`에서 렌더링됨
- 표시기 소스는 슈퍼샘플링 인식 `sourceSize`를 사용합니다.

표시기 아이콘 매핑:

- 기본값: `generalchevronDown`
- 경계선 없음: `generalchevronDownBorderless`
- 기본/파괴: `generalchevronDownAccent`
- 비활성화됨: `generalchevronDownDisabled`

레이아웃:

- 기본 톤: `Primary` (`Kind=accent`)
- 기본 및 표시기 아이콘 프레임: `Theme.iconSm`(데스크톱의 경우 `18 x 18`, 모바일의 경우 `36 x 36`)
- 고정 프레임: `40 x 22` 데스크탑, `80 x 44` 모바일
- 왼쪽 삽입: `Theme.gap4`; 오른쪽, 위쪽, 아래쪽: `Theme.gap2`(`4 / 2 / 2 / 2` 데스크탑)
- `cornerRadius: Theme.radiusMd`(`8` 데스크톱, `16` 모바일)
- 측정된 아이콘-표시기 중첩: `spacing: -Theme.gap2`(`-2` 데스크톱, `-4` 모바일); 콘텐츠 레이아웃은 이 공용 속성을 사용하므로 호출자는 구성 요소를 교체하지 않고도 이를 재정의할 수 있습니다.

<a id="figma-visual-contract"></a>

## Figma 시각적 계약

- 출처: `700:337`, `Type=IconMenuButton`.
- 데스크탑 기본 아이콘 경계는 `x=4, y=2, 18 x 18`입니다. 갈매기형 경계는 `x=20, y=2, 18 x 18`입니다.
- 기존 `generalprojectStructure.svg` 및 톤별 `generalchevronDown*.svg` 자산은 내보낸 Figma 벡터와 일치하여 재사용됩니다.
- 현재 Figma 세트에는 악센트, 기본값 및 테두리 없는 변형이 있습니다. 상속된 파괴/비활성화 상태는 동일한 형상을 유지합니다.

주입된 방법:

- `AbstractButton`에서 상속됨: `method`, `methods`, `hasInjectedMethods`, `invokeMethods(...)`

<a id="icon-resolution-order"></a>

## 아이콘 해결 순서

1. `iconSource` (`url`)
2. `iconName`
3. 그룹화된 `icon.name`(상위 스타일 객체에서 제공하는 경우)
4. 대체 경로 아이콘(`projectStructure`)

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.IconMenuButton {
    iconName: "projectStructure"
    method: function(eventData) {
        menu.open()
    }
}
```

<a id="shared-motion"></a>

## 공유 모션

두 기호 모두 상위 버튼의 동작과 하나의 활성화 이벤트를 공유합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
