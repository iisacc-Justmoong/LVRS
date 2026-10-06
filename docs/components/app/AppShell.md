# AppShell

위치: `src/qml/AppShell.qml`

`AppShell`는 추가 API를 추가하지 않고 `ApplicationWindow`를 직접 상속하는 호환성 래퍼입니다.

<a id="purpose"></a>

## 목적

- 레거시 가져오기/사용 경로를 유지합니다.
- 새 코드가 `LV.ApplicationWindow`를 직접 채택하는 동안 마이그레이션이 안전한 별칭을 제공합니다.

<a id="api-contract"></a>

## API 계약

- 추가 속성, 메서드 또는 신호는 정의되지 않습니다.
- 모든 동작은 `ApplicationWindow`에서 상속됩니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.AppShell {
    visible: true
    width: 1100
    height: 720
    title: "LVRS"
    navItems: ["Overview", "Runs"]
}
```

<a id="recommendation"></a>

## 추천

표준 소비자 앱 루트에는 `LV.ApplicationWindow`를 사용하고, 호환성 표시 루트 래퍼로만 `LV.AppBootstrapWindow`를 유지하고, 호환성을 위해서만 `AppShell`를 유지하세요.

<a id="migration-note"></a>

## 마이그레이션 노트

`AppShell`에서 `ApplicationWindow`로 마이그레이션할 때 `AppShell`가 직접 래퍼이므로 속성 이름은 동일하게 유지됩니다. 권장 마이그레이션 경로는 유형 사용만 바꾸는 것입니다.

<a id="compatibility-scope"></a>

## 호환성 범위

`AppShell`는 의도적으로 얇습니다. 호환성만 보장됩니다. 새로운 기능 스위치는 `ApplicationWindow`에 처음 문서화되어 있습니다.

## FAQ

Q. `AppShell`는 다른 탐색 수명주기를 제공합니까?   A. 아니요. 탐색 동작은 `ApplicationWindow`와 정확히 동일합니다.

Q. 새 모듈은 안정성을 위해 `AppShell`만 가져와야 합니까?   A. 아니요. 표준 앱 루트 부트스트랩 경로에 `ApplicationWindow`를 사용하거나 레거시 유형 호환성이 필요한 경우에만 `AppBootstrapWindow`를 사용하세요.

<a id="deprecation-strategy"></a>

## 지원 중단 전략

프로젝트 정책이 `AppShell`를 더 이상 사용하지 않는 경우 다음을 사용하여 호환성 창을 유지하세요.

1. codemod 지원 유형 이름 바꾸기,
2. 릴리스 노트 매핑 테이블,
3. 새로운 `AppShell` 사용에 대한 임시 린트 규칙 경고입니다.

<a id="shared-motion"></a>

## 공유 모션

이 호환성 래퍼는 창 및 탐색 피드백을 ApplicationWindow에 위임합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
