<a id="debug"></a>

# 디버그

위치: `src/backend/runtime/debuglogger.h` / `src/backend/runtime/debuglogger.cpp`

`Debug`(`DebugLogger`)는 메모리 버퍼, 필터링 및 선택적 stdout 에코와 통합된 QML/C++용 공유 로깅 싱글턴입니다.

<a id="purpose"></a>

## 목적

- 구조화된 애플리케이션/런타임 로깅을 중앙 집중화합니다.
- UI 도구용 한계가 설정된 메모리 내 입력 버퍼를 제공합니다.
- `RuntimeEvents`에서 선택적 런타임 스트림 캡처를 제공합니다.

<a id="core-methods"></a>

## 핵심 방법

로그 방출:

- `log(component, event, data?)`
- `warn(component, event, data?)`
- `error(component, event, data?)`

런타임 부착:

- `attachRuntimeEvents()`
- `detachRuntimeEvents()`

쿼리 및 유지 관리:

- `entries(limit?)`
- `filteredEntries(limit?)`
- `summary()`
- `clearEntries()`
- `setFilters(levels, components, text)`

<a id="key-properties"></a>

## 주요 속성

활성화:

- `enabled`
- `runtimeCaptureEnabled`
- `runtimeEchoEnabled`
- `paused`

에코 정책:

- `runtimeEchoMinIntervalMs`
- `runtimeEchoExcludeTypes`
- `stdoutMinimumLevel`
- `stdoutNoiseReductionEnabled`
- `verboseOutput`
- `jsonOutput`

버퍼/필터 상태:

- `maxEntries`, `entryCount`, `droppedCount`, `sequence`
- `runtimeAttached`
- `levelFilter`, `componentFilter`, `textFilter`
- `lastEntry`

<a id="runtime-capture-flow"></a>

## 런타임 캡처 흐름

1. `attachRuntimeEvents()`는 런타임 싱글턴를 해결합니다.
2. `eventRecorded`를 구독하세요.
3. 런타임 이벤트를 디버그 항목으로 변환합니다.
4. 필터/출력 정책을 적용합니다.
5. 한계가 설정된 항목 버퍼를 추가합니다.

<a id="usage-example"></a>

## 사용예

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.Debug.enabled = true
    LV.Debug.attachRuntimeEvents()
    LV.Debug.setFilters(["WARN", "ERROR"], [], "")
}
```

<a id="related-schema"></a>

## 관련 스키마

자세한 출력 항목 스키마 및 콘솔 행 필드는 다음에 정의되어 있습니다.

- `docs/backend/DebugOutput.md`

<a id="advanced-filter-example"></a>

## 고급 필터 예

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.Debug.enabled = true
    LV.Debug.runtimeCaptureEnabled = true
    LV.Debug.runtimeEchoEnabled = false
    LV.Debug.setFilters(["WARN", "ERROR"], ["RuntimeEvents", "PageRouter"], "")
}
```

<a id="performance-notes"></a>

## 성능 노트

- 긴 세션을 위해 `maxEntries` 한계가 설정된를 유지하십시오.
- 하위 소비 측 파서가 요구하는 경우에만 stdout JSON 출력을 활성화합니다.
- 고주파 노이즈를 억제하려면 `runtimeEchoExcludeTypes`를 적극적으로 사용하십시오.

## FAQ

Q. 로거를 활성화하면 비즈니스 로직 동작이 변경됩니까?   A. 그러면 안됩니다. 로거는 관찰 전용이며 기능 결과를 변경해서는 안 됩니다.
