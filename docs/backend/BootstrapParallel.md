# BootstrapParallel

위치: `src/backend/runtime/bootstrapparallel.h` / `src/backend/runtime/bootstrapparallel.cpp`

`BootstrapParallel`는 독립적인 시작 도메인을 병렬로 로드한 다음 수집된 결과를 선택한 QObject 스레드에 적용하기 위한 작은 런타임 실행기입니다.

비용이 많이 드는 작업이 독립적인 앱 부트스트랩 단계를 위한 것이지만 최종 변형은 기본/UI 스레드에서 발생해야 합니다.

<a id="purpose"></a>

## 목적

- 독립적인 로드 작업을 한계가 설정된 작업자 풀로 팬아웃합니다.
- 입력된 `QVariant` 페이로드 및 작업별 진단을 수집합니다.
- 결정론적 우선순위에 따라 성공적인 결과를 적용합니다.
- 일반적으로 `QGuiApplication` 또는 `QQmlApplicationEngine`인 수신기 스레드를 통해 적용 콜백을 라우팅합니다.
- 치명적인 부트스트랩 오류와 정상적인 오류를 구별합니다.

## API

- `lvrs::runBootstrapParallelTasks(tasks, options) -> BootstrapParallelRunResult`

핵심 구조체:

- `BootstrapParallelTaskContext`
- `BootstrapParallelTask`
- `BootstrapParallelTaskResult`
- `BootstrapParallelRunOptions`
- `BootstrapParallelRunResult`

<a id="task-contract"></a>

## 업무 계약

`BootstrapParallelTask` 필드:

- `name`
- `priority`
- `fatal`
- `metadata`
- `load(context, value, errorMessage)`
- `apply(result, errorMessage)`

`load`는 작업자 스레드에서 실행됩니다. QML 객체, UI 스레드가 소유한 QObject 트리 또는 ViewModels를 직접 건드리는 것을 피해야 합니다. `value`를 통해 도메인 데이터를 반환합니다.

`apply`는 모든 로드가 완료된 후 실행됩니다. `BootstrapParallelRunOptions::applyReceiver`가 설정되어 있고 호출자가 해당 수신자 스레드에 아직 없는 경우 LVRS는 `Qt::BlockingQueuedConnection`를 사용하여 콜백을 호출합니다.

기본적으로 실패한 로드의 경우 `apply`를 건너뜁니다.

<a id="options"></a>

## 옵션

`BootstrapParallelRunOptions` 필드:

- `applyReceiver`: 스레드가 적용 콜백을 소유한 QObject입니다. `nullptr`는 호출자 스레드를 의미합니다.
- `maxThreadCount`: 한계가 설정된 작업자 수입니다. `0`는 `QThread::idealThreadCount()`를 사용합니다.
- `skipApplyOnLoadFailure`: 기본 `true`.
- `logDiagnostics`: 기본 `true`, `LVRS bootstrap.parallel.*` 라인을 내보냅니다.

<a id="result"></a>

## 결과

`BootstrapParallelRunResult`에는 다음이 포함됩니다.

- `ok`
- `taskResults`
- `errors`
- `elapsedMs`
- `fatalFailure()`
- `errorMessage()`
- `diagnostics()`

각 작업 결과에는 다음이 포함됩니다.

- `ok`, `loadOk`, `applied`, `applyOk`
- `errorMessage`, `applyErrorMessage`
- `value`
- `metadata`
- `loadElapsedMs`, `applyElapsedMs`

<a id="usage"></a>

## 사용법

```cpp
lvrs::BootstrapParallelTask libraryTask;
libraryTask.name = QStringLiteral("library");
libraryTask.priority = 10;
libraryTask.metadata = {{QStringLiteral("domain"), QStringLiteral("library")}};
libraryTask.load = [](const lvrs::BootstrapParallelTaskContext &, QVariant *value, QString *errorMessage) {
    QVariantMap snapshot = loadLibrarySnapshot();
    if (snapshot.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Library snapshot is empty");
        return false;
    }
    if (value)
        *value = snapshot;
    return true;
};
libraryTask.apply = [libraryViewModel](const lvrs::BootstrapParallelTaskResult &result, QString *) {
    libraryViewModel->applySnapshot(result.value.toMap());
    return true;
};

lvrs::BootstrapParallelRunOptions options;
options.applyReceiver = qGuiApp;
options.maxThreadCount = 4;

const lvrs::BootstrapParallelRunResult result =
    lvrs::runBootstrapParallelTasks({libraryTask}, options);
if (result.fatalFailure())
    return false;
```

수명 주기 후크 예:

```cpp
lvrs::QmlBootstrapTask loadDomains;
loadDomains.name = QStringLiteral("load-domains");
loadDomains.stage = lvrs::QmlAppLifecycleStage::AfterFirstIdle;
loadDomains.run = [](const lvrs::QmlAppLifecycleContext &context, QString *errorMessage) {
    lvrs::BootstrapParallelRunOptions options;
    options.applyReceiver = context.application;

    const lvrs::BootstrapParallelRunResult result =
        lvrs::runBootstrapParallelTasks(buildDomainTasks(), options);
    if (!result.ok && errorMessage)
        *errorMessage = result.errorMessage();
    return result.ok;
};
```

<a id="responsibility-boundary"></a>

## 책임 경계

LVRS 소유:

- 작업 팬아웃,
- 한계가 설정된 작업자 실행,
- 결정론적 결과/순서 적용,
- 적용 스레드 호출,
- 타이밍 및 오류 진단.

앱이 소유하는 것:

- 도메인 선택,
- 파일/데이터베이스/네트워크 구문 분석,
- 페이로드 스키마,
- ViewModel 돌연변이 규칙,
- 실패한 도메인이 치명적인지 여부.
