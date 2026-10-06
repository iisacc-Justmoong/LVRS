# AppBootstrap

위치: `src/backend/runtime/appbootstrap.h` / `src/backend/runtime/appbootstrap.cpp` / `src/backend/runtime/appentry.h`

`AppBootstrap`는 그래픽 백엔드 정책, 스타일 설정 및 글꼴 대체 경로 설정을 위한 사전/사후 애플리케이션 초기화 루틴을 제공합니다.

## API

- `lvrs::preApplicationBootstrap(options) -> AppBootstrapState`
- `lvrs::postApplicationBootstrap(app, options) -> void`
- `lvrs::loadQmlRootObjects(engine, roots, options) -> QmlRootLoadResult`
- `lvrs::runQmlAppLifecycleStage(context, hooks, stage, logDiagnostics) -> QmlBootstrapQueueResult`
- `lvrs::scheduleQmlAppLifecycleStage(receiver, context, hooks, stage, logDiagnostics) -> bool`
- `lvrs::runBootstrappedQmlApp(argc, argv, launchSpec) -> int`

Quick 모듈 앱의 경우 Qt 에 대해 `src/backend/runtime/appentry.h` 와 `QmlAppLaunchSpec` 를 pre/post 부트스트랩 시퀀스 주위의 표준 래퍼로 선호합니다. 이는 필요한 부트스트랩 순서를 그대로 유지하며, `QQmlApplicationEngine::loadFromModule(...)` 이전에 `initialProperties` 를 통해 루트 QML 속성을 초기화할 수 있습니다. `runBootstrappedQmlApp()` 는 이제 루트 생성을 `loadQmlRootObjects()` 에 위임하므로, 하위 소비 측 앱은 레거시 `moduleUri/rootObject` 필드를 통해 하나의 루트를 로드하거나 `QmlAppLaunchSpec::roots` 를 통해 여러 루트를 로드할 수 있습니다.

<a id="qml-root-loading"></a>

## QML 루트 로딩

`QmlRootLoadSpec`는 하나의 루트 객체를 설명합니다.

- `moduleUri: QString`
- `rootObject: QString`(기본값 `Main`)
- `initialProperties: QVariantMap`
- `windowActivationPolicy: QmlWindowActivationPolicy`(기본값 `Inherit`)

`QmlAppLaunchSpec`는 호환성을 위해 단일 루트 필드를 유지하고 다음을 추가합니다.

- 멀티 루트 앱용 `roots: QList<QmlRootLoadSpec>`
- 앱 수준 대체 경로인 `windowActivationPolicy: QmlWindowActivationPolicy`

`roots` 가 비어있을 때, LVRS 는 `moduleUri` , `rootObject` , `initialProperties` , 및 `windowActivationPolicy` 에서 하나의 `QmlRootLoadSpec` 를 생성합니다. `roots` 가 비어있지 않을 때, 각 루트는 `moduleUri` 를 생략하여 앱 레벨 모듈 URI 를 상속할 수 있으며, 루트별 초기 속성은 앱 레벨 초기 속성과 병합되지 않습니다.

`QmlRootLoadResult` 보고서:

- `ok`
- `errors`
- `rootObjects`
- `windows`
- `errorMessage()`

루트 로딩은 엄격합니다. 새 엔진 루트 객체를 추가하지 않는 빈 모듈 URI/root 객체 또는 `loadFromModule(...)` 호출은 무시되는 대신 로드 실패로 보고됩니다.

<a id="window-activation-policy"></a>

## 창 활성화 정책

`QmlWindowActivationPolicy`는 로드된 루트 객체가 `QWindow`일 때 LVRS가 수행하는 작업을 제어합니다.

- `Inherit`: 앱/기본 정책 사용
- `None`: QML 가시성 및 활성화를 그대로 유지합니다.
- `Show`: `show()`에 전화
- `ShowAndRaise`: `show()` 및 `raise()`를 호출합니다.
- `ShowRaiseAndActivate`: `show()`, `raise()` 및 `requestActivate()`를 호출합니다.

기본 앱 수준 정책은 기존 QML 기반 가시성을 유지하기 위한 `None`입니다. 이전에 각 루트 로드 후 `show/raise/activate`를 반복했던 앱은 이제 앱 사양 또는 개별 루트 사양에 `windowActivationPolicy = QmlWindowActivationPolicy::ShowRaiseAndActivate`를 설정할 수 있습니다.

<a id="lifecycle-hooks-and-queue"></a>

## 수명주기 후크 및 대기열

`QmlAppLaunchSpec::lifecycle`는 3개의 부트스트랩 스테이지를 제공합니다.

- `AfterRootLoaded` (`after-root-loaded`)
- `AfterWindowActivated` (`after-window-activated`)
- `AfterFirstIdle` (`after-first-idle`)

각 단계는 직접 후크와 명명된 대기열 작업을 지원합니다.

- `afterRootLoaded`
- `afterWindowActivated`
- `afterFirstIdle`
- `tasks: QList<QmlBootstrapTask>`

`QmlBootstrapTask` 필드:

- `name: QString`
- `stage: QmlAppLifecycleStage`
- `priority: int`(낮은 값이 먼저 실행되고 동일한 우선순위를 위해 삽입 순서가 유지됨)
- `fatal: bool`
- `run(context, errorMessage) -> bool`

`runBootstrappedQmlApp()` 는 루트 생성 후 `AfterRootLoaded` 와 `AfterWindowActivated` 를 동기적으로 실행합니다. 동기 단계 중 어느 하나에서 치명적인 실패가 발생하면 `-1` 로 시작이 중단됩니다. `AfterFirstIdle` 는 `scheduleQmlAppLifecycleStage()` 를 통해 0- 지연 이벤트 루프 턴으로 예약되어, 일반적인 지연된 부트스트랩 `QTimer::singleShot(0, ...)` 패턴과 일치합니다. 예약된 대기 단계에서 치명적인 실패가 발생하면 `-1` 로 애플리케이션이 종료됩니다.

수명주기 컨텍스트는 다음을 노출합니다.

- `application`
- `engine`
- `rootLoadResult`
- `stage`

LVRS는 단계 타이밍, 작업 순서 지정 및 진단만 소유합니다. 하위 소비 측 앱은 여전히 ​​각 단계에 속하는 도메인 서비스 또는 데이터 로더를 결정합니다.

## `AppBootstrapOptions`

- `applicationName: QString`
- `quickStyleName: QString`
- `configureRenderQualityDefaults: bool`(기본값 `true`)
- `bootstrapGraphicsBackend: bool`(기본값 `true`)
- `logBootstrapDiagnostics: bool`(기본값 `true`)
- `logGraphicsBackend: bool`(기본값 `true`)
- `installBundledFonts: bool`(기본값 `true`)
- `installPretendardFallbacks: bool`(기본값 `true`)
- `enforcePretendardFallback: bool`(기본값 `true`)

## `AppBootstrapState`

- `ok: bool`
- `errorMessage: QString`
- `graphicsBackend: GraphicsBackendBootstrapResult`

<a id="required-call-order"></a>

## 필수 호출 순서

1. `QGuiApplication`를 생성하기 전에 `preApplicationBootstrap`를 호출하세요.
2. `state.ok == false`인 경우 시작을 중단합니다.
3. `QGuiApplication`를 구축하세요.
4. 앱 생성 후 즉시 `postApplicationBootstrap`를 호출하세요.

<a id="what-preapplicationbootstrap-does"></a>

## `preApplicationBootstrap`의 기능

- 옵션 `RenderQuality::configureGlobalDefaults()`.
- 옵션 `QQuickStyle::setStyle(quickStyleName)`.
- 컴팩트 JSON 페이로드를 사용한 단계별 부트스트랩 진단 로깅.
- 옵션 그래픽 백엔드 부트스트랩 및 진단 로깅.
- `RenderQuality` 전역 기본값이 적용되기 전에 플랫폼 부트스트랩 프로필을 통해 시드 장면 그래프 환경 힌트(예: 파이프라인 캐시 및 아틀라스 크기 조정)가 적용됩니다.
- 부트스트랩 책임은 백엔드 선택 및 Qt 사전 창 기본값으로 제한됩니다. 실시간 품질 정책은 런타임 `RenderQuality.applyWindow(...)`의 소유로 유지됩니다.

<a id="platform-bootstrap-policy"></a>

### 플랫폼 부트스트랩 정책

- macOS / iOS: Metal 백엔드 수정; `4x/3`(macOS) 또는 `4x/2`(iOS) MSAA/비행 중인 프레임 부트스트랩 프로필.
- Windows: 런타임 프로빙 및 OpenGL 대체 경로가 포함된 D3D11 첫 번째 부트스트랩; `4x/2` 부트스트랩 렌더 프로필.
- Android: Vulkan-첫 번째 부트스트랩 및 OpenGL 대체 경로; `4x/2` 부트스트랩 렌더 프로필 및 텍스처 아틀라스 가장자리 감소.
- Linux: Qt 기본 백엔드 선택; `4x/2` 부트스트랩 렌더 프로필.
- WASM: Qt 기본 백엔드 선택; 부분 업데이트, 배치 렌더러 및 파이프라인 캐시 힌트가 명시적으로 강제 해제되고 부트스트랩에서 데스크톱 수준 깊이/스텐실 기본값을 강제하지 않고 더 가벼운 `2x/1` 부트스트랩 렌더링 프로필.

<a id="diagnostics-output"></a>

### 진단 출력

`logBootstrapDiagnostics == true`, 부트스트랩는 stdout에 간결한 구조의 라인을 작성합니다. 주요 이벤트는 다음과 같습니다.

- `LVRS bootstrap.pre.options`
- `LVRS bootstrap.pre.render-quality`
- `LVRS bootstrap.pre.quick-style`
- `LVRS bootstrap.graphics.probe`
- `LVRS bootstrap.graphics.selected`
- `LVRS bootstrap.graphics.fallback`
- `LVRS bootstrap.pre.complete`
- `LVRS bootstrap.post.application`
- `LVRS bootstrap.post.font-policy`
- `LVRS bootstrap.entry.import-paths`
- `LVRS bootstrap.entry.load-request`
- `LVRS bootstrap.entry.root-load-request`
- `LVRS bootstrap.entry.root-loaded`
- `LVRS bootstrap.entry.root-load-failed`
- `LVRS bootstrap.lifecycle.stage-start`
- `LVRS bootstrap.lifecycle.queue-start`
- `LVRS bootstrap.lifecycle.task-start`
- `LVRS bootstrap.lifecycle.task-complete`
- `LVRS bootstrap.lifecycle.task-failed`
- `LVRS bootstrap.lifecycle.stage-complete`

페이로드에는 플랫폼 태그, 요청된 부트스트랩 옵션, 효과적인 렌더 프로필 기본값, 장면 그래프 환경 값, 백엔드 프로브 후보, 대체 경로 이유 및 글꼴 정책 결정이 포함됩니다.

<a id="what-postapplicationbootstrap-does"></a>

## `postApplicationBootstrap`의 기능

- 애플리케이션 이름을 적용합니다(제공된 경우).
- 공유 `FontPolicy` 부트스트랩 경로에서 번들 글꼴을 로드합니다.
- Pretendard 대체를 설치합니다.
- 선택적으로 Pretendard 대체 경로를 적용하고 적용이 실패할 경우 경고합니다.

<a id="typical-c-usage"></a>

## 일반적인 C++ 사용법

```cpp
lvrs::AppBootstrapOptions options;
options.applicationName = QStringLiteral("MyApp");
options.quickStyleName = QStringLiteral("Basic");

const lvrs::AppBootstrapState state = lvrs::preApplicationBootstrap(options);
if (!state.ok)
    return -1;

QGuiApplication app(argc, argv);
lvrs::postApplicationBootstrap(app, options);
```

<a id="option-tuning-examples"></a>

## 옵션 튜닝 예

최소 시작(제한된 테스트 하니스에 대해 백엔드 부트스트랩 비활성화):

```cpp
lvrs::AppBootstrapOptions options;
options.bootstrapGraphicsBackend = false;
options.logGraphicsBackend = false;
```

글꼴 중심 시작(입력 체계 정책만 유지):

```cpp
lvrs::AppBootstrapOptions options;
options.configureRenderQualityDefaults = false;
options.bootstrapGraphicsBackend = false;
options.installBundledFonts = true;
options.installPretendardFallbacks = true;
```

멀티루트 앱 항목:

```cpp
lvrs::QmlAppLaunchSpec launchSpec;
launchSpec.bootstrap.applicationName = QStringLiteral("MyApp");
launchSpec.bootstrap.quickStyleName = QStringLiteral("Basic");
launchSpec.moduleUri = QStringLiteral("MyApp");
launchSpec.windowActivationPolicy = lvrs::QmlWindowActivationPolicy::ShowRaiseAndActivate;

lvrs::QmlRootLoadSpec shellRoot;
shellRoot.rootObject = QStringLiteral("Main");
shellRoot.initialProperties.insert(QStringLiteral("initialRoutePath"), QStringLiteral("/"));

lvrs::QmlRootLoadSpec inspectorRoot;
inspectorRoot.rootObject = QStringLiteral("InspectorWindow");

launchSpec.roots = {shellRoot, inspectorRoot};
return lvrs::runBootstrappedQmlApp(argc, argv, launchSpec);
```

지연된 부트스트랩 대기열:

```cpp
lvrs::QmlBootstrapTask loadPresets;
loadPresets.name = QStringLiteral("load-presets");
loadPresets.stage = lvrs::QmlAppLifecycleStage::AfterFirstIdle;
loadPresets.priority = 20;
loadPresets.run = [](const lvrs::QmlAppLifecycleContext &, QString *errorMessage) {
    const bool ok = startPresetLoad();
    if (!ok && errorMessage)
        *errorMessage = QStringLiteral("Preset load failed");
    return ok;
};

launchSpec.lifecycle.afterWindowActivated = [](const lvrs::QmlAppLifecycleContext &context) {
    qInfo() << "Activated windows:" << context.rootLoadResult.windows.size();
};
launchSpec.lifecycle.tasks = {loadPresets};
```

여러 시작 도메인을 독립적으로 로드한 다음 기본 스레드에 적용할 수 있는 경우 병렬 도메인 로드는 수명 주기 작업 내에 중첩될 수 있습니다. 실행자 계약에는 `docs/backend/BootstrapParallel.md`를 사용하십시오. 앱에서 도메인 구문 분석 및 ViewModel 돌연변이 정책을 유지합니다.

하나 이상의 루트 창이 표시된 후 서비스가 한 번만 시작되어야 하는 경우 `AfterWindowActivated` 작업에서 `ForegroundServiceGate`를 사용하여 포그라운드 서비스 시작을 보호할 수 있습니다. `docs/backend/ForegroundServices.md`를 참조하세요. 서비스 ID, 권한 및 스케줄러 의미 체계는 앱 정책으로 유지됩니다.

권한 부트스트랩는 포그라운드 서비스 또는 수명 주기 작업 내에서 `PermissionRequestSequencer`를 사용하여 요청 기록을 보존하면서 앱 정의 권한 확인을 순서대로 실행할 수 있습니다. `docs/backend/PermissionSequencer.md`를 참조하세요. 네이티브 권한 API 및 프롬프트 정책은 앱/플랫폼 코드로 유지됩니다.

<a id="troubleshooting"></a>

## 문제 해결

- `state.ok == false`: `state.errorMessage`를 기본 근본 원인 문자열로 사용합니다.
- 부트스트랩 stdout 라인에는 이미 렌더링 기본값, 백엔드 프로빙 및 대체 경로 이유에 대한 구조화된 페이로드가 포함되어 있습니다. 가능한 경우 CI 및 충돌 보고서에 캡처하십시오.
- 누락된 스타일 변경: 앱을 생성하기 전에 `quickStyleName`가 설정되어 있는지 확인하세요.
- 예기치 않은 글꼴 대체 경로: 번들 글꼴 리소스 및 대체 경로 적용 결과를 확인합니다.

## FAQ

Q. `postApplicationBootstrap`를 건너뛸 수 있나요?   A. 기술적으로는 건너뛸 수 있지만 권장 시작 기준(글꼴/대체/앱 이름)은 불완전합니다.
