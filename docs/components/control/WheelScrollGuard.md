# WheelScrollGuard

위치: `src/qml/components/control/util/WheelScrollGuard.qml`

`WheelScrollGuard`는 휠 델타를 의도된 내부 플릭 가능 항목으로 라우팅하고 선택적으로 이벤트를 소비합니다.

<a id="purpose"></a>

## 목적

- 내부 스크롤 표면과 외부 스크롤 표면 사이에 중첩된 스크롤 블리드를 방지합니다.
- 휠 델타 입력을 한계가 설정된 `contentY` 업데이트로 변환합니다.

## API

- `targetFlickable`(`contentY`, `contentHeight`, `height`를 노출해야 함)
- `consumeInside`(기본값 `true`)
- `fallbackStep`(기본값 `Theme.gap20`)
- `wheelRouted(wheelEvent, delta, previousContentY, nextContentY)` 신호

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.WheelScrollGuard {
    anchors.fill: parent
    targetFlickable: innerFlick
    consumeInside: true
}
```

<a id="how-it-works"></a>

## 동작 원리

- 내부 `EventListener` 트리거 `wheel`를 사용합니다.
- 포인터 포인트가 대상 플릭 가능한 범위 내에 있는 경우에만 이벤트를 라우팅합니다.
- 델타 소스 우선순위: `pixelDelta.y` -> `angleDelta.y`는 `fallbackStep`로 변환됩니다.
- 한계가 설정된 `contentY` 업데이트(`0..maxContentY`)를 적용하고 `wheelRouted`를 내보냅니다.

<a id="advanced-example-passive-routing-mode"></a>

## 고급 예: 패시브 라우팅 모드

```qml
import LVRS 1.0 as LV

LV.WheelScrollGuard {
    targetFlickable: editorFlick
    consumeInside: false
}
```

이 모드는 휠 델타를 라우팅하지만 이벤트 수락을 강제하지는 않습니다.

## FAQ

Q. 휠 스크롤이 여전히 외부 컨테이너에 영향을 미치는 이유는 무엇입니까?   A. `consumeInside`를 확인하세요. false인 경우 이벤트 전파가 의도적으로 허용됩니다.

Q. 휠 이벤트가 발생해도 왜 움직이지 않나요?   A. 대상이 유효한 `contentHeight`, `height`를 노출하고 현재 스크롤 가능한 오버플로가 있는지 확인합니다.

<a id="validation-checklist"></a>

## 검증 체크리스트

- 포인터 내부 감지는 중첩된 변환 아래에 올바르게 매핑됩니다.
- 델타 변환은 마우스와 터치패드 전체에서 일관되게 작동합니다.
- 한계가 설정된 스크롤은 콘텐츠 제한을 초과하는 것을 방지합니다.

<a id="shared-motion"></a>

## 공유 모션

휠 라우팅은 동기식입니다. 애니메이션이 이벤트를 지연시키거나 수신자를 변경하지 않습니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
