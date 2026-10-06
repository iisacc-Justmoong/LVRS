<a id="link"></a>

# 링크

위치: `src/qml/components/navigation/Link.qml`

`Link`는 `AbstractButton`를 기반으로 구축된 탐색 트리거 구성 요소입니다.

<a id="purpose"></a>

## 목적

- 뷰별 라우터 상용구 없이 선언적 경로/구성 요소 탐색을 제공합니다.
- 선택적 대체 의미 체계를 통해 경로 탐색과 구성 요소 탐색을 모두 지원합니다.

<a id="core-api"></a>

## 코어 API

라우팅:

- `router`(선택적 명시적 라우터)
- `href`
- `to`(`href`의 별칭)
- `params`
- `replace`
- `targetComponent`

시각적:

- `linkColor`, `hoverColor`, `pressedColor`, `disabledColor`
- `underline`

내용:

- 기본 `content` 슬롯
- 슬롯이 비어 있는 경우 텍스트 대체 경로 라벨

<a id="behavior-contract"></a>

## 행동 계약

라우터 해결 순서:

1. `router`
2. `Navigator.router`
3. 해결되지 않은 경우 작동하지 않음

탐색 동작:

- `targetComponent`가 설정된 경우:
  - `replace == true` -> `replaceWith(targetComponent, params)`
  - 그렇지 않으면 -> `goTo(targetComponent, params)`
- 그렇지 않으면 `href`가 존재하는 경우:
  - `replace == true` -> `replace(href, params)`
  - 그렇지 않으면 -> `go(href, params)`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.Link {
    href: "/reports"
    text: "Reports"
}
```

<a id="shared-motion"></a>

## 공유 모션

활성화 시 링크가 리바운드되고 해당 라우터가 대상 전환을 처리합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
