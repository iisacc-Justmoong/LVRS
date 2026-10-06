<a id="debug-policy"></a>

# 디버그 정책

목표: 런타임 동작을 변경하거나 UX를 저하시키지 않고 중앙 집중식 진단을 제공합니다.

<a id="rule-1-route-application-logs-through-debug"></a>

## 규칙 1: `Debug`를 통해 애플리케이션 로그 라우팅

선호하는 API:

- `LV.Debug.log(component, event, data?)`
- `LV.Debug.warn(component, event, data?)`
- `LV.Debug.error(component, event, data?)`

<a id="rule-2-logging-is-opt-in"></a>

## 규칙 2: 로깅은 선택 사항입니다.

`Debug.enabled`의 기본값은 `false`입니다. 생산 흐름은 명시적으로 구성되지 않는 한 침묵을 유지해야 합니다.

<a id="rule-3-runtime-echo-is-rate-controlled"></a>

## 규칙 3: 런타임 에코는 속도에 따라 제어됩니다.

런타임 에코가 활성화되면 다음을 준수하십시오.

- 최소 에코 간격(`runtimeEchoMinIntervalMs`)
- 제외 유형 목록(`runtimeEchoExcludeTypes`)
- 표준 출력 임계값(`stdoutMinimumLevel`)

이는 빈도가 높은 사건으로 인한 홍수를 방지합니다.

<a id="rule-4-logging-must-never-block-ui-paths"></a>

## 규칙 4: 로깅은 UI 경로를 차단해서는 안 됩니다.

로깅은 대화형 이벤트 처리기에서 동기 차단 작업을 도입해서는 안 됩니다.

<a id="rule-5-filtering-belongs-in-logger-not-call-sites"></a>

## 규칙 5: 필터링은 호출 사이트가 아닌 로거에 속합니다.

UI 코드에서 조건부 로깅 로직을 분산시키는 대신 로거 필터(`levelFilter`, `componentFilter`, `textFilter`)를 사용하세요.

<a id="rule-6-schema-stability-is-mandatory"></a>

## 규칙 6: 스키마 안정성은 필수입니다.

출력 필드 이름이나 항목 스키마가 변경되면 동일한 변경 세트에서 `docs/backend/DebugOutput.md`를 업데이트하세요.

<a id="production-debug-strategy"></a>

## 생산 디버그 전략

권장 환경 분할:

- local/dev: `Debug.enabled=true`, 선택적 런타임 캡처 켜기
- 스테이징: 더 엄격한 필터와 속도 제한으로 캡처
- 프로덕션: 기본값은 비활성화되어 있으며 인시던트 기간에만 활성화됩니다.

<a id="incident-triage-workflow"></a>

## 사고 분류 워크플로

런타임 사건을 조사할 때:

1. 엄격한 필터로 로거를 활성화합니다(`WARN/ERROR` 먼저).
2. 런타임 이벤트를 첨부하고,
3. 한계가 설정된 항목 및 요약 스냅샷을 캡처합니다.
4. 타임스탬프 및 시퀀스 ID와 함께 데이터 내보내기,
5. 캡처 창이 닫힌 후 로거를 비활성화합니다.

<a id="enforcement-note"></a>

## 시행 참고 사항

정확성을 위해 로그가 필요한 모든 기능은 정책을 위반하는 것입니다. 로깅은 기능적 종속성이 아닌 진단적 종속성입니다.

<a id="audit-checklist"></a>

## 감사 체크리스트

- 모든 런타임/UI 진단 로그는 `Debug`를 통해 내보내집니다.
- 기능 흐름은 로그 부작용에 따라 달라지지 않습니다.
- 속도 제한 및 필터는 대용량 환경에서 구성됩니다.
