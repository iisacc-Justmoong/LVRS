<a id="routing-policy"></a>

# 라우팅 정책

목표: 명시적인 경로 의미 체계와 가역적 상태를 통해 예측 가능한 스택 탐색을 보장합니다.

<a id="rule-1-route-path-grammar"></a>

## 규칙 1: 경로 경로 문법

지원되는 경로 모양:

- 정적: `/reports`
- 매개변수: `/runs/[id]`
- 나머지: `/logs/[...path]`

경로는 선행 슬래시로 정규화되고 후행 슬래시는 없습니다(루트 제외).

<a id="rule-2-pagerouterpath-is-the-source-of-stack-truth"></a>

## 규칙 2: `PageRouter.path`는 스택 진실의 소스입니다.

탐색 작업(`go`, `replace`, `setRoot`, `pop`, `popToRoot`)은 스택 상태와 현재 경로 상태를 일관되게 업데이트해야 합니다.

<a id="rule-3-back-semantics-are-stack-only"></a>

## 규칙 3: 백 의미론은 스택 전용입니다.

실행 취소 탐색은 다음과 같이 표시됩니다.

- `pop()`
- `popToRoot()`

모든 사용자 정의 "back" 동작은 명시적 스택 돌연변이로 모델링되어야 합니다.

<a id="rule-4-component-navigation-is-out-of-band-unless-encoded"></a>

## 규칙 4: 인코딩되지 않은 경우 구성 요소 탐색은 대역 외입니다.

`goTo(component)` 및 관련 구성요소-대상 작업은 허용되지만 `path` 항목으로 인코딩되지 않는 한 경로 ID를 전달하지 않습니다.

<a id="rule-5-route-level-model-metadata-should-be-colocated"></a>

## 규칙 5: 경로 수준 모델 메타데이터는 같은 위치에 배치되어야 합니다.

페이지에 모델 바인딩/소유권이 필요한 경우 페이지별 임시 바인딩보다 경로 메타데이터(`viewModelKey`, `viewId`, `writable`)를 선호합니다.

<a id="rule-6-not-found-fallback-must-be-explicit"></a>

## 규칙 6: 찾을 수 없음 대체 경로는 명시적이어야 합니다.

경로 확인이 실패하면 `notFoundComponent` 또는 `notFoundSource`만 대체 경로 탐색을 생성할 수 있습니다. 그렇지 않으면 `navigationFailed(path)`가 실행되어야 합니다.

<a id="rule-7-global-navigator-ownership-is-explicit"></a>

## 규칙 7: 글로벌 네비게이터 소유권이 명시적입니다.

의도한 활성 라우터만 글로벌 탐색기로 등록해야 합니다. 중첩된 라우터는 등록 동작을 의도적으로 정의해야 합니다.

<a id="route-change-review-checklist"></a>

## 경로 변경 검토 체크리스트

경로를 수정하는 경우:

1. 경로 정규화 동작이 변경되지 않았는지 확인합니다.
2. 동적/휴식 매개변수가 여전히 올바르게 해결되는지 확인하세요.
3. 찾을 수 없는 동작이 명시적으로 남아 있는지 확인하고,
4. 교체/설정/팝 흐름에서 `path` 스택 동기화를 확인합니다.
5. 경로 수준 MVVM 메타데이터가 여전히 의도한 대로 바인딩되는지 확인합니다.

<a id="recommended-route-definition-pattern"></a>

## 권장 경로 정의 패턴

다음을 포함하여 경로 객체 근처에 경로 문서를 함께 배치하는 것을 선호합니다.

- 경로 의도,
- 예상 매개변수,
- 모델 키 보기,
- 쓰기 가능한 소유권 요구 사항.

이는 기능이 성장하는 동안 라우팅 회귀를 크게 줄입니다.

<a id="enforcement-note"></a>

## 시행 참고 사항

경로 전환은 `path` 상태에서만 재현 가능해야 합니다. 내비게이션을 복원하기 위해 추가적인 숨겨진 상태가 필요한 경우 라우팅 설계가 과소 지정됩니다.

<a id="audit-checklist"></a>

## 감사 체크리스트

- 모든 경로에는 명확한 소유권과 목적이 있으며,
- 동적/휴식 세그먼트 구문 분석은 통합 테스트로 다룹니다.
- 스택 돌연변이(`push/replace/set/pop`)는 되돌릴 수 있고 추적 가능한 상태로 유지됩니다.
