# ForegroundServices

위치: `src/backend/runtime/foregroundservices.h` / `src/backend/runtime/foregroundservices.cpp`

`ForegroundServices` 는 가시적인 작업 공간 창이 존재한 후 앱 서비스를 시작하기 위한 원샷 게이트를 제공합니다. 스케줄러, 모니터 또는 부트스트랩 진입점과 같은 시작 작업에 의도되어 있으며, QML 루트가 다시 로드되거나 생명주기 후크가 한 번 이상 실행되는 동안 반복적으로 시작되어서는 안 됩니다.

<a id="purpose"></a>

## 목적

- 로드된 QML 루트에 표시되는 `QWindow`가 하나 이상 포함되어 있는지 감지합니다.
- 포그라운드 서비스 시작 콜백을 한 번 실행합니다.
- `priority`를 통해 결정론적 작업 순서를 유지합니다.
- 집계 서비스 시작 진단.
- 복구 가능한 오류와 치명적인 시작 오류를 구별합니다.

## API

- `visibleWorkspaceWindows(rootLoadResult)`
- `hasVisibleWorkspace(rootLoadResult)`
- `ForegroundServiceGate::startOnceWhenWorkspaceVisible(context, tasks, options)`
- `ForegroundServiceGate::reset()`

핵심 구조체:

- `ForegroundServiceStartContext`
- `ForegroundServiceTask`
- `ForegroundServiceTaskResult`
- `ForegroundServiceStartOptions`
- `ForegroundServiceStartResult`

<a id="task-contract"></a>

## 업무 계약

`ForegroundServiceTask` 필드:

- `name`
- `priority`
- `fatal`
- `metadata`
- `start(context, errorMessage)`

작업은 우선순위 오름차순으로 실행됩니다. 게이트는 콜백을 실행하기 전에 자체적으로 시작되었음을 표시하므로 한 서비스가 실패를 보고하더라도 반복 호출은 포그라운드 서비스를 복제하지 않습니다. 호출자에게 명시적인 재시도 정책이 있는 경우에만 `reset()`를 사용하십시오.

`ForegroundServiceStartContext`는 다음을 노출합니다.

- `application`
- `engine`
- `rootLoadResult`
- `visibleWindows`
- `metadata`

<a id="options"></a>

## 옵션

`ForegroundServiceStartOptions` 필드:

- `requireVisibleWorkspace`: 기본 `true`; 표시되는 창이 없으면 시작이 거부되고 게이트는 재시도 가능한 상태로 유지됩니다.
- `logDiagnostics`: 기본 `true`; `LVRS bootstrap.foreground.*` 라인을 방출합니다.
- `metadata`: 시작 컨텍스트에 복사되었습니다.

<a id="result"></a>

## 결과

`ForegroundServiceStartResult`에는 다음이 포함됩니다.

- `ok`
- `started`
- `alreadyStarted`
- `visibleWorkspace`
- `visibleWindowCount`
- `taskResults`
- `errors`
- `elapsedMs`
- `fatalFailure()`
- `errorMessage()`
- `diagnostics()`

<a id="usage"></a>

## 사용법

```cpp
auto foregroundGate = std::make_shared<lvrs::ForegroundServiceGate>();

lvrs::ForegroundServiceTask schedulerStart;
schedulerStart.name = QStringLiteral("async-scheduler");
schedulerStart.priority = 10;
schedulerStart.fatal = true;
schedulerStart.start = [scheduler](const lvrs::ForegroundServiceStartContext &, QString *errorMessage) {
    if (scheduler->start())
        return true;
    if (errorMessage)
        *errorMessage = QStringLiteral("Scheduler failed to start");
    return false;
};

lvrs::QmlBootstrapTask foregroundServices;
foregroundServices.name = QStringLiteral("foreground-services");
foregroundServices.stage = lvrs::QmlAppLifecycleStage::AfterWindowActivated;
foregroundServices.fatal = true;
foregroundServices.run = [foregroundGate, schedulerStart](const lvrs::QmlAppLifecycleContext &context,
                                                          QString *errorMessage) {
    const lvrs::ForegroundServiceStartResult result =
        foregroundGate->startOnceWhenWorkspaceVisible(context, {schedulerStart});
    if (!result.ok && errorMessage)
        *errorMessage = result.errorMessage();
    return result.ok;
};
```

<a id="responsibility-boundary"></a>

## 책임 경계

LVRS 소유:

- 보이는 창 감지,
- 원샷 게이트 상태,
- 결정론적 서비스 시작 주문,
- 고장/치명적 진단,
- 부트스트랩 로그 라인.

앱이 소유하는 것:

- 어떤 서비스가 존재하는지,
- 서비스가 치명적이라고 간주되는 경우,
- 스케줄러 내부,
- 권한 프롬프트,
- 플랫폼/도메인별 시작 정책.
