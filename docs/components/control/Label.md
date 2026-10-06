<a id="label"></a>

# 라벨

위치: `src/qml/components/control/display/Label.qml`

`Label`는 스타일 토큰을 타이포그래피 측정항목에 매핑하는 LVRS 텍스트 래퍼입니다.

<a id="purpose"></a>

## 목적

- `Theme`에서 일관된 인쇄 스타일 매핑을 제공합니다.
- 별칭을 통해 텍스트 관련 `Text` API를 노출합니다.
- 디버그 경고 후크를 통해 스타일 준수를 확인합니다.

<a id="style-constants"></a>

## 스타일 상수

- `title`
- `title2`
- `header`
- `header2`
- `body`
- `description`
- `caption`
- `disabled`

<a id="core-api"></a>

## 코어 API

- `style`
- `sizeToContentHeight`(기본값 `false`, 자연스럽게 래핑된 콘텐츠 측정 가능)
- `text`, `color`, `font`
- `elide`, `wrapMode`
- `horizontalAlignment`, `verticalAlignment`
- `lineHeight`, `lineHeightMode`, read-only `contentHeight`, read-only `lineCount`
- `maximumLineCount`, `fontSizeMode`
- `textFormat`, `linkColor`

계산된 스타일 출력:

- `resolvedStyleColor`
- `stylePixelSize`
- `styleWeight`
- `styleName`
- `styleLineHeight`
- `styleLetterSpacing`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.Label {
    text: "Status"
    style: body
}
```

<a id="how-it-works"></a>

## 동작 원리

- 스타일 열거형은 색상, 두께, 크기, 줄 높이 및 스타일 이름에 대한 `Theme` 토큰 그룹에 매핑됩니다.
- 내부 `Text` 항목은 렌더링 기준 원본입니다.
- 스타일 변경 시 비호환 텍스트 스타일은 `Debug.warn`를 통해 디버그 경고를 표시할 수 있습니다.

<a id="advanced-example-two-line-clamped-description"></a>

## 고급 예시: 2-라인 클램프된 설명

```qml
import LVRS 1.0 as LV

LV.Label {
    width: 280
    style: description
    wrapMode: Text.WordWrap
    maximumLineCount: 2
    text: "This text is clamped to two lines and then elided by text metrics."
}
```

<a id="practical-tip"></a>

## 실용적인 팁

유지 관리를 위해 하드 코딩된 글꼴 메트릭 대신 의미 체계 `style` 상수를 사용합니다.

## FAQ

Q. 텍스트 스타일을 사용자 정의 글꼴 재정의 또는 스타일 상수로 구성해야 합니까?   A. 먼저 스타일 상수를 선호하고, 불가피한 경우에만 최소한의 재정의를 적용하세요.

<a id="shared-motion"></a>

## 공유 모션

불투명도가 원활하게 전환됩니다. 텍스트 값, 접근성 콘텐츠 및 측정값이 동시에 업데이트됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
