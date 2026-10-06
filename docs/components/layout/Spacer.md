<a id="spacer"></a>

# 스페이서

위치: `src/qml/components/layout/Spacer.qml`

`Spacer`는 `HStack`, `VStack` 및 `ZStack` 컨텍스트를 위한 축 인식 유연한 필러입니다.

<a id="purpose"></a>

## 목적

- 활성 스택 축을 따라 확장합니다.
- 요청 시 최소 축 길이를 예약하세요.
- 오버레이(`ZStack`) 컨텍스트에서 전체 상위를 채웁니다.

## API

- `minLength`
- `stackAxis`(`"horizontal"` 또는 `"vertical"`, 일반적으로 자동 할당됨)

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.HStack {
    LV.Label { text: "Left" }
    LV.Spacer {}
    LV.Label { text: "Right" }
}
```

<a id="how-it-works"></a>

## 동작 원리

- 상위 스택 플래그(`__isHStack`, `__isVStack`) 또는 명시적 `stackAxis`에서 축을 확인합니다.
- 수평 모드에서는 너비를 채우고 `minimumWidth = minLength`를 적용합니다.
- 수직 모드에서는 높이를 채우고 `minimumHeight = minLength`를 적용합니다.
- `ZStack`에서는 `anchors.fill = parent`를 사용합니다.

<a id="practical-notes"></a>

## 실용적인 참고 사항

- 스택 컨텍스트에서 `Spacer`는 활성 축에만 영향을 미치고 교차 축은 변경되지 않은 상태로 유지합니다.
- `ZStack`에서 `Spacer`는 설계상 전체 채우기 오버레이 도우미로 작동합니다.
- 스택 동작을 모방하는 사용자 정의 컨테이너를 구성하는 경우에만 명시적 `stackAxis`를 선호합니다.

## FAQ

Q. `Spacer`가 일부 사용자 정의 컨테이너에서 확장되지 않는 이유는 무엇입니까?   A. 축 추론은 스택/레이아웃 컨텍스트에 따라 다릅니다. 사용자 정의 래퍼의 경우 `stackAxis`를 명시적으로 설정합니다.

Q. `Spacer`를 레이아웃 외부에서 사용할 수 있나요?   A. 예, 하지만 상위 요소가 호환 가능한 레이아웃 의미 체계를 노출하지 않는 한 확장 동작은 정의되지 않습니다.

<a id="validation-checklist"></a>

## 검증 체크리스트

- 런타임 상위 유형에 대한 축 추론(`horizontal`/`vertical`)을 확인합니다.
- `minLength`가 레이아웃 최소 크기에 반영되었는지 확인하고,
- 오버레이 컨텍스트에서 예기치 않은 앵커 충돌이 없는지 확인합니다.

<a id="shared-motion"></a>

## 공유 모션

최소 길이는 스택 축을 따라 탄력적으로 고정됩니다. 클릭할 수 있는 표면이 없습니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
