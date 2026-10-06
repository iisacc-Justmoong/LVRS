# PermissionSequencer

위치: `src/backend/runtime/permissionsequencer.h` / `src/backend/runtime/permissionsequencer.cpp`

`PermissionSequencer`는 앱 정의 권한 요청 단계를 순차적으로 실행하고 요청 기록을 저장합니다. 의도적으로 일반적입니다. LVRS는 전체 디스크 액세스, 사진 라이브러리 액세스, 문서 폴더 또는 플랫폼 브리지 API에 대해 알지 못합니다.

<a id="purpose"></a>

## 목적

- 결정적인 우선순위에 따라 권한 요청 단계를 실행합니다.
- 기본적으로 필수 오류가 발생하면 중지됩니다.
- 진단을 위해 메모리 내 요청 기록을 유지합니다.
- 승인/거부/건너뛰기/사용할 수 없음/실패 수를 집계합니다.
- 개발자 도구 및 로그에 대한 구조화된 결과 맵을 제공합니다.

## API

- `PermissionRequestSequencer::run(steps, options) -> PermissionRequestRunResult`
- `PermissionRequestSequencer::history()`
- `PermissionRequestSequencer::clearHistory()`

핵심 구조체:

- `PermissionRequestStepContext`
- `PermissionRequestStep`
- `PermissionRequestStepResult`
- `PermissionRequestRunOptions`
- `PermissionRequestRunResult`

상태 열거:

- `Granted`
- `Denied`
- `Skipped`
- `Unavailable`
- `Failed`

<a id="step-contract"></a>

## 단계 계약

`PermissionRequestStep` 필드:

- `name`
- `priority`
- `required`
- `metadata`
- `request(context, details, errorMessage)`

콜백은 플랫폼/도메인 동작을 소유합니다. 네이티브 API를 호출하고, 앱 UI를 표시하고, 앱 상태를 검사하고, 관련이 없는 경우 권한을 건너뛸 수 있습니다. LVRS는 반환된 상태와 세부정보만 기록합니다.

필수 단계가 `Denied`, `Unavailable` 또는 `Failed`를 반환하면 실행이 실패합니다. 해당 상태의 선택적 단계가 기록되지만 실행이 실패하지는 않습니다.

<a id="options"></a>

## 옵션

`PermissionRequestRunOptions` 필드:

- `stopOnRequiredFailure`: 기본 `true`.
- `appendHistory`: 기본 `true`.
- `logDiagnostics`: 기본 `true`, `LVRS bootstrap.permission.*` 라인을 내보냅니다.
- `metadata`: 모든 단계 컨텍스트에 복사되었습니다.

<a id="result"></a>

## 결과

`PermissionRequestRunResult`에는 다음이 포함됩니다.

- `ok`
- `runId`
- `stoppedEarly`
- `stepResults`
- `errors`
- `elapsedMs`
- `completedCount`
- `grantedCount`
- `deniedCount`
- `skippedCount`
- `unavailableCount`
- `failedCount`
- `requiredFailure()`
- `errorMessage()`
- `diagnostics()`

각 단계 결과에는 다음이 포함됩니다.

- `name`, `index`, `priority`, `runId`
- `required`, `ok`, `status`
- `granted`, `terminalFailure`
- `errorMessage`
- `metadata`
- `details`
- `elapsedMs`

<a id="usage"></a>

## 사용법

```cpp
lvrs::PermissionRequestSequencer sequencer;

lvrs::PermissionRequestStep fullDisk;
fullDisk.name = QStringLiteral("full-disk-access");
fullDisk.priority = 10;
fullDisk.required = true;
fullDisk.request = [](const lvrs::PermissionRequestStepContext &, QVariantMap *details, QString *errorMessage) {
    const auto status = requestFullDiskAccessThroughAppBridge();
    if (details)
        details->insert(QStringLiteral("bridge"), QStringLiteral("macos"));
    if (status == AppPermissionStatus::Granted)
        return lvrs::PermissionRequestStatus::Granted;
    if (errorMessage)
        *errorMessage = QStringLiteral("Full disk access was not granted");
    return lvrs::PermissionRequestStatus::Denied;
};

lvrs::PermissionRequestStep photos;
photos.name = QStringLiteral("photo-library");
photos.priority = 20;
photos.required = false;
photos.request = [](const lvrs::PermissionRequestStepContext &, QVariantMap *, QString *) {
    return lvrs::PermissionRequestStatus::Skipped;
};

const lvrs::PermissionRequestRunResult result = sequencer.run({fullDisk, photos});
if (!result.ok)
    qWarning().noquote() << result.errorMessage();
```

포그라운드 서비스 예:

```cpp
lvrs::ForegroundServiceTask permissionBootstrap;
permissionBootstrap.name = QStringLiteral("permission-bootstrap");
permissionBootstrap.start = [&sequencer](const lvrs::ForegroundServiceStartContext &, QString *errorMessage) {
    const lvrs::PermissionRequestRunResult result =
        sequencer.run(buildPermissionStepsForCurrentPlatform());
    if (!result.ok && errorMessage)
        *errorMessage = result.errorMessage();
    return result.ok;
};
```

<a id="responsibility-boundary"></a>

## 책임 경계

LVRS 소유:

- 단계 주문,
- 순차적 실행,
- 역사 저장,
- 상태/카운트 집계,
- 일반 진단.

앱이 소유하는 것:

- 권한 이름,
- 네이티브 권한 호출,
- 프롬프트 UI,
- 플랫폼 교량 기능,
- 앱별 권한이 필요한지 여부.
