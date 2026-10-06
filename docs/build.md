<a id="build-and-runtime-setup"></a>

# 빌드 및 런타임 설정

<a id="quick-install"></a>

## 빠른 설치

```bash
./install.sh
```

Windows PowerShell에서:

```powershell
.\install.ps1
```

`install.sh` 는 이제 Rust CLI `lvrs install` 를 얇게 감싸는 래퍼입니다. 로컬 Qt 버전 발견은 `/Volumes/Storage/Qt` 를 사용하며, `6.8.3` 하위의 macOS, iOS, Android, 및 WASM 키트를 포함합니다. `QT_VERSION_ROOT` 와 명시적 플랫폼 Qt 힌트는 여전히 지원됩니다. macOS 에 `lvrs install` 는 첫 번째 CMake 구성 전에 호스트 키트를 해결하고 `Qt6_DIR` 와 `LVRS_BOOTSTRAP_QT_PREFIX_MACOS` 를 모두 제공합니다. 명시적인 Qt 힌트와 `CMAKE_PREFIX_PATH` 내의 유효한 Qt 항목은 기본 키트보다 우선하며, 오래된 검색 경로 항목은 이동된 기본 설치물을 숨기지 않습니다. 이전 설치된 프레임워크는 구성이 성공한 후만 제거되므로, 구성 실패 시 이를 유지합니다. CLI 소스를 업데이트하거나 Qt 를 이동한 후, 현재 CLI 를 빌드하고 설치된 `lvrs` 실행 파일을 새로고침하기 위해 `./install.sh` 를 체크아웃에서 실행합니다. 소스 변경 사항만 커밋하는 것은 이미 설치된 CLI 바이너리를 대체하지 않습니다.
- `cargo`를 사용할 수 있는 경우 `cargo run --manifest-path src/rust-cli/Cargo.toml --target-dir build/rust-cli --bin lvrs -- install ...`를 실행합니다.
- `cargo`를 사용할 수 없지만 `PATH`에 `lvrs`가 있는 경우 `lvrs install ...`를 실행합니다.
- 둘 다 사용할 수 없는 경우 CLI를 먼저 빌드하라는 지침에 따라 설치 종료를 종료합니다.
- `install.ps1` 는 Windows 래퍼입니다. Qt 6 MinGW 접두사를 자동 감지하고, Ninja 를 사용하며, 부트스트랩 플랫폼을 `windows` 로 기본값으로 설정한 후 동일한 `lvrs install` 흐름을 호출하여 Windows 프레임워크 패키지를 `<prefix>/platforms/windows` 하위에 설치합니다. Rust CLI 가 사용 불가능한 경우, Windows 프레임워크 패키지에 대한 동등한 직접 CMake `bootstrap_lvrs_all` 흐름으로 회귀합니다.
- 직접 `lvrs install` 를 사용하면 체크아웃 바깥에서 실행될 때 `<prefix>/src/LVRS/INSTALL_SOURCE_INFO.txt` 에서 저장소 루트를 복구할 수 있습니다. 저장된 절대 경로가 상위 디렉토리 이름 변경 후 오래되었다면, CLI 는 후미 경로 섹먼트를 매칭하여 루트를 재위치하거나, 그렇지 않으면 설치된 소스 스냅샷을 원위치로 폴백합니다.
- `LVRS_ROOT` 또는 `LVRS_PROJECT_ROOT`가 `~/.local/SDK/LVRS`와 같은 설치된 접두사를 가리키는 경우 직접 CLI 명령은 해당 값을 접두사로 처리하고 `<prefix>/src/LVRS`를 통해 소스를 확인합니다. 해당 스냅샷이 누락된 경우 체크아웃 내에서 실행된 명령은 현재 저장소 루트로 대체됩니다.

설치 흐름은 `bootstrap_lvrs_all`를 빌드합니다. 기본적으로 프레임워크 부트스트랩 플랫폼 세트는 현재 호스트를 따릅니다.
- Linux: `linux`
- macOS: `macos;ios;android;wasm`
- Windows: `windows;android;wasm`
일치하는 Qt 키트가 없는 플랫폼은 부트스트랩 타겟 생성 중에 건너뜁니다. `./install.sh --platforms linux,android,wasm` (쉼표/분리부호 목록) 를 사용하여 플랫폼 집합을 제한하거나 덮어씁니다. Linux 호스트에서는 설치자가 정리/빌드 전에 의존성 사전 검사 를 실행합니다. 호스트 C++ 툴체인, 필수 Qt 6.5 + 모듈, 및 Qt 호스트 도구를 검증한 후, 사용 가능한 경우 공통 Qt 레이아웃에서 `Qt6_DIR` / `LVRS_BOOTSTRAP_QT_PREFIX_LINUX` 를 자동으로 해결합니다. 그 Linux 의존성이 누락되어 있고 배포판 패키지 관리자가 인식되면, CLI 는 정확한 설치 명령을 출력하고 `./install.sh --install-linux-deps` 로 실행할 수 있습니다. `lvrs doctor --fix` 는 설치 시작 없이 동일한 호스트 측 의존성 확인/수정 흐름을 실행합니다. `lvrs doctor --bootstrap [--with-wasm|--platforms ...]` 는 또한 `src/main.cpp` 부트스트랩 엔트리 마커를 추가로 검증하고 누락된 크로스 플랫폼 Qt / Android / WASM 자동 감지 힌트를 보고하며, 요청된 부트스트랩 타겟 세트가 준비되지 않았을 때0 가 아닌 상태로 종료합니다. 설치된 패키지는 `<prefix>/platforms/<platform>` ( `macos` , `linux` , `windows` , `ios` , `android` , `wasm` ) 에 기록된 후, 호스트 플랫폼 경로가 CMake 사용자 패키지 레지스트리에 등록됩니다. 체크아웃 루트는 `Workspace/SDK/LVRS` 입니다; 기본 설치 루트는 `~/.local/SDK/LVRS` 입니다. CMake 은 동일한 기본값을 사용하며, 명시적인 `CMAKE_INSTALL_PREFIX` , `--prefix` , 및 `LVRS_INSTALL_PREFIX` 오버라이드가 계속 지원됩니다. 체크아웃을 이동한 후 `./install.sh` 를 재실행하면 `build/` 를 재생성하고 설치된 스냅샷에 새로운 절대 소스 경로를 기록합니다. 설치기는 또한 실행 중인 CLI 를 `<prefix>/bin/lvrs` ( `lvrs.exe` 에서 Windows ) 로 복사하며, `env.sh` 는 해당 디렉토리를 `PATH` 에 추가합니다. 셸 래퍼는 `CARGO_TARGET_DIR` 가 명시적으로 설정되지 않는 한 Cargo 출력을 `build/rust-cli/` 아래로 유지하므로, CMake 깨끗한 재설치는 실행 중인 CLI 를 제거하지 않습니다. 설치기는 항상 깨끗한 재설정을 수행합니다: 이전 CMake 빌드 상태를 제거한 후 구성을 수행하여 `build/rust-cli/` 를 보존하므로 실행 중인 CLI 가 설치에 사용 가능하게 유지된 후, 구성이 성공적으로 완료되면 설치된 LVRS 아티팩트만 제거합니다. 소스 스냅샷은 숨겨진 `.build.lvrs-stale-*` 정리 잔여물과 `src/rust-cli/build` 및 레거시 `src/rust-cli/target` 디렉토리를 모두 제외합니다. `install.sh` 는 기본적으로 호스트 빌드에서 예제/테스트를 구성합니다; `--without-examples --without-tests` 를 전달하여 이를 비활성화할 수 있습니다. 호스트 예제가 활성화되면 설치기는 `lvrs_host_examples_all` 타겟을 먼저 빌드합니다. 각 빌드 트리 예제는 `build/example/<ExampleName>/bin` 하에 실행 가능 파일을 생성하며, Linux 는 추가로 `bin/lvrs-runtime/` 를 LVRS 공유 라이브러리 및 QML 모듈과 함께 빌드합니다. 검출된 `example/*/bin/LVRSExample*` 경로는 런처 스크립트입니다: 저장소 런처는 `build/example/.../bin` 로 되돌아가고, 설치된 소스 스냅샷은 해당 런처 옆에 새로 갱신된 데스크톱 런타임인 `*.real` 파일로 받습니다. `--without-examples` 가 사용되면 해당 스냅샷 런타임 페이로드가 제거됩니다.

<a id="rust-cli-entry-points"></a>

## Rust CLI 진입점

직접 CLI 호출(래퍼 없음):

```bash
cargo run --manifest-path src/rust-cli/Cargo.toml --target-dir build/rust-cli --bin lvrs -- install
```

기본 진입점 부트스트랩 프로필:

```bash
cargo run --manifest-path src/rust-cli/Cargo.toml --target-dir build/rust-cli --bin lvrs -- bootstrap
```

`lvrs bootstrap`는 `--platforms`가 제공되지 않는 한 호스트 일치 대상 세트로 기본 설정됩니다.
- Linux 호스트: `linux`
- macOS 호스트: `macos;ios;android`
- Windows 호스트: `windows;android`
`--with-wasm`는 해당 호스트 기본 세트에 `wasm`를 추가합니다. 실행하기 전에 `src/main.cpp`에 예상되는 LVRS 부트스트랩 항목 마커(`runBootstrappedQmlApp`, `rootObject = QStringLiteral("Main")`)가 포함되어 있는지 확인합니다.

<a id="configure"></a>

## 구성

```bash
cmake -S . -B build
```

번들 CLion MinGW 툴체인가 포함된 Windows에서 일치하는 Qt MinGW 패키지 접두사에서 CMake를 가리킵니다.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64
```

Windows의 로컬 디버그 빌드의 경우 Windows DLL 내보내기 표면이 명시적으로 유지 관리될 때까지 정적 LVRS 연결을 선호합니다.

```bash
cmake -S . -B build -DLVRS_BUILD_SHARED_LIBS=OFF
```

빌드하는 동안 Qt 런타임를 유지하고 MinGW 툴체인를 `PATH`에 일치시킵니다.

```powershell
$env:PATH = "C:/Qt/6.8.3/mingw_64/bin;C:/Qt/Tools/mingw1310_64/bin;$env:PATH"
```

이 저장소의 CLion 디버그 프로필은 동일한 접두사와 `PATH`를 사용하고 `build/`에 빌드 아티팩트를 씁니다.

<a id="build"></a>

## 빌드

```bash
cmake --build build -j
```

저장소-로컬 개발의 경우 루트 도우미를 선호합니다.

```bash
./build.sh
```

`build.sh`는 `LVRS_BUILD_EXAMPLES=ON` 및 `LVRS_BUILD_TESTS=ON`를 사용하여 `build/`를 구성하고 호스트 출력을 빌드하며 체크인된 `example/*/bin/LVRSExample*` 실행 프로그램이 더 이상 빌드 트리 런타임과 일치하지 않는 경우 중단됩니다.

<a id="run"></a>

## 실행

```bash
./build.sh
./example/VisualCatalog/bin/LVRSExampleVisualCatalog
```

모든 호스트 예제를 빌드하고 모든 `build/example/*/bin` 디렉터리를 채웁니다.

```bash
./build.sh --without-tests
```

시각 카탈로그 예시 타겟 ( `LVRSExampleVisualCatalog` )은 `build/example/VisualCatalog/bin/LVRSExampleVisualCatalog` 로 실행 가능 파일을 방출합니다. 검출된 런처인 `example/VisualCatalog/bin/LVRSExampleVisualCatalog` 는 저장소 내 개발 중 런타임 스냅샷 후 `LVRSCore` 프레임워크 라이브러리 타겟 자체는 실행 가능한 앱을 방출하지 않습니다.

<a id="test"></a>

## 테스트

```bash
ctest --test-dir build --output-on-failure
```

<a id="build-options"></a>

## 빌드 옵션

- `LVRS_BUILD_EXAMPLES` (`OFF`): 실행 가능한 예제를 빌드합니다.
- `LVRS_BUILD_TESTS` (`OFF`): 테스트를 빌드하고 등록합니다.
- `LVRS_INSTALL_QML_MODULE`(`ON`): `<prefix>/lib/qt6/qml/LVRS` 아래에 QML 모듈 아티팩트(`qmldir`, qmltypes, 플러그인, QML 파일)를 설치합니다.
- `LVRS_ENFORCE_VULKAN`(`ON`): 기능 게이트 백엔드가 필요한 플랫폼에 대한 고정 그래픽 백엔드 Qt 기능 요구 사항이 누락된 경우 CMake 구성에 실패합니다.
- `LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS` ( `ON` ): 플랫폼별 릴리스/relwithdebinfo/minsizerel 컴파일 및 링크 최적화 플래그를 적용합니다. 섹션 컴파일 플래그는 C, C++, Objective-C 및 Objective- C++ 에만 적용됩니다. 링커 옵션은 CMake 의 언어 인식 `LINKER:` 번역을 사용하여, Swift 타겟이 동일한 헬퍼를 사용할 수 있습니다. macOS 에서 `LVRSTests_cmake_swift_release_optimizations` 는 Release Swift 소비자를 C++ 정적 라이브러리에 연결하여 빌드 및 실행합니다.
- `LVRS_ENABLE_IPO`(`ON`): 툴체인 지원이 가능할 때 릴리스와 유사한 구성에 대해 절차간 최적화(LTO)를 활성화합니다.
- `LVRS_SANITIZER`(`none`): 소독제 계측(`none`, `address`, `thread`, `undefined`).
- `LVRS_FORCE_X86_QT_TOOLS`(`OFF`): 필요한 경우 Rosetta를 통해 Qt 호스트 도구를 실행합니다.
- `LVRS_ENABLE_FRAMEWORK_BOOTSTRAP_TARGETS` (`ON`): `bootstrap_lvrs_*` 프레임워크 다중 플랫폼 대상을 생성합니다.
- `LVRS_BOOTSTRAP_INSTALL_ROOT` (`<build>/lvrs-install`): `bootstrap_lvrs_*`에서 사용하는 설치 루트입니다.
- `LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET`: 중첩된 부트스트랩 구성으로 전달되는 선택적 Apple 배포 대상입니다. macOS 부트스트랩는 그렇지 않으면 상위 `CMAKE_OSX_DEPLOYMENT_TARGET`를 상속합니다. iOS는 해당 호스트 값을 상속하지 않습니다.
- `LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET_<PLATFORM>`: `LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET_MACOS` 또는 `LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET_IOS`와 같은 플랫폼별 재정의.

<a id="downstream-cmake-integration"></a>

## 하위 소비 측 CMake 통합

먼저 접두사에 LVRS를 설치합니다.

```bash
cmake -S . -B build \
  -DLVRS_BUILD_EXAMPLES=OFF \
  -DLVRS_BUILD_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX=/path/to/lvrs-prefix
cmake --build build -j
cmake --install build
```

그런 다음 Qt Quick 프로젝트에서 다음을 수행합니다.

```cmake
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2)
find_package(LVRS CONFIG REQUIRED)

lvrs_add_qml_app(
    TARGET MyApp
    URI MyApp
    QML_FILES
        Main.qml
)
```

`CMAKE_PREFIX_PATH` 를 `/path/to/lvrs-prefix` 설치 루트로 설정할 때 하위 소비 측 프로젝트를 구성합니다. `lvrs_configure_qml_app()` 는 설치된 패키지 소비를 위해 `QT_QML_IMPORT_PATH` 를 설정하고, 미설정 시 기본 실행 가능 파일 출력 디렉토리 ( `<build>/bin` )를 적용하며, 정적 QML 플러그인 아티팩트 LVRS 를 정적 패키지 빌드에서 자동 링크/가입하고, Linux 단계에서 `lvrs-runtime/` 를 실행 가능 파일과 LVRS 공유 라이브러리에 QML 모듈과 함께 배치합니다. `runBootstrappedQmlApp()` 는 Linux 런타임 QML 임포트 위치 (예: `lvrs-runtime/qml`, 설치된 `../lib/qt6/qml`, 스냅샷 플랫폼 레이아웃) 를 자동으로 탐지하여 QML 루트를 로드하기 전에 적용합니다. `QmlAppLaunchSpec::initialProperties` 는 레거시 단일 루트 `loadFromModule(...)` 경로 바로 전에 `QQmlApplicationEngine::setInitialProperties(...)` 를 통해 적용되며, `QmlAppLaunchSpec::roots` 는 각 루트 초기 속성으로 여러 `QmlRootLoadSpec` 항목을 선언할 수 있습니다. 윈도우 루트는 앱 수준의 `QmlWindowActivationPolicy` 를 상속하거나 `show()`, `raise()`, `requestActivate()` 에 대한 자체 정책을 사용할 수 있습니다. `QmlAppLaunchSpec::lifecycle` 는 `after-root-loaded`, `after-window-activated`, 0-delay `after-first-idle` 후크/큐 작업을 제공하여 저우선도 도메인 시작이 수동으로 작성된 `QTimer::singleShot(0, ...)` 블록에서 벗어나게 합니다. `ForegroundServiceGate` 는 가시 루트 윈도우가 존재할 때까지 일회성 포그라운드 서비스 시작을 보호할 수 있습니다. `PermissionRequestSequencer` 는 앱 정의된 권한 요청 단계를 순차적으로 실행하고 메모리 내 요청 이력을 유지할 수 있습니다. `BootstrapParallel` 는 한계가 설정된 워커 풀에서 독립적인 시작 도메인 로드를 실행하고 선택된 QObject 스레드에서 수집된 결과를 적용할 수 있습니다. `QmlContextBinder` 는 루트 로딩 전에 C++ 서비스를 컨텍스트 속성으로 노출하고 C++ `ViewModel` 하위 클래스를 `LV.ViewModels` 싱글턴 에 등록할 수 있습니다. `QmlTypeRegistrar` 는 하위 소비 측 앱 QML 타입을 매니페스트에서 등록하고 중복 또는 실패한 등록에 대한 구조화된 진단을 반환할 수 있습니다. Qt 런타임 배포 자체는 타겟 환경에 특화되어 있습니다. `LV.ApplicationWindow` 는 모바일/데스크톱 재순서를 위한 적응형 레이아웃 정책 API 를 제공합니다:
- `scaffoldLayoutMode` (`auto`, `mobile`, `desktop`)
- `scaffoldLayoutPlatform` 재정의(기본 정식 플랫폼 토큰, 별칭은 `Platform.normalizeTarget()`를 통해 정규화됨)
- `scaffoldForceDesktopOnLargeMobile` + `scaffoldMobileDesktopMinWidth`
- `scaffoldPreferBottomNavigation` + `scaffoldBottomNavigationMaxItems`
- `scaffoldCompactSpacingEnabled` + `scaffoldCompactSpacingBreakpoint`
- `scaffoldNavRailMaxWidthRatio` + `scaffoldDrawerMarginSafety`
- 런타임 상태 플래그: `adaptiveMobileLayout`, `adaptiveDesktopLayout`, `adaptiveRailNavigation`, `adaptiveDrawerNavigation`, `adaptiveBottomNavigation`
- `matchesMedia()` 토큰: `mobile-layout`, `desktop-layout`, `rail-nav`, `drawer-nav`, `bottom-nav`
상태는 페이지 스택 라우팅 ( `LV.PageRouter` ) 을 사용하고, 배치 는 `RowLayout` / `ColumnLayout` 플렉스 레이아웃을 `LV.ApplicationWindow` 내부에서 사용합니다. `LV.ApplicationWindow` 페이지 스택 API : `initialRoutePath`, `pageRoutes`, `pageInitialPath`, `useInternalPageStack`, `activePageRouter`, `pageStackNavigated`, `pageStackNavigationFailed` 입니다. 기본 `auto` 모드는 정통 모바일 타겟과 그 정규화된 별칭 ( `android`, `android-arm64`, `ios`, `ios-simulator`, ...) 에 대해 모바일 우선이며, 명시적으로 구성되지 않는 한 넓은 화면 모바일 윈도우를 데스크톱 레일 레이아웃으로 강제로 설정하는 것을 방지합니다. `desktop-compact` 프로필 또한 항목 수가 설정된 제한에 맞을 때 하단 내비게이션을 선택합니다. 모바일/데스크톱 단일 프로젝트 앱에 권장되는 앱 루트 부트스트랩 프로필:

```cpp
lvrs::QmlAppLaunchSpec launchSpec;
launchSpec.bootstrap.applicationName = QStringLiteral("MyApp");
launchSpec.bootstrap.quickStyleName = QStringLiteral("Basic");
launchSpec.moduleUri = QStringLiteral("MyApp");
launchSpec.rootObject = QStringLiteral("Main");
launchSpec.initialProperties = QVariantMap{
    {QStringLiteral("initialRoutePath"), QStringLiteral("/")},
    {QStringLiteral("bootstrapTitle"), QStringLiteral("MyApp")}
};
```

```qml
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    visible: true
    pageRoutes: [
        { path: "/", component: homePage }
    ]

    Component {
        id: homePage
        Item {}
    }
}
```
`LV.ApplicationWindow` 는 가져온 하위 소비 측 프로필의 재사용 가능한 부트스트랩 루트입니다. 이제 `initialRoutePath` 에서 직접 플랫폼 프로필 기반 런타임 첨부, 전역 네비게이터 등록, 내부 페이지 스택 초기화를 소유합니다. `LV.ApplicationWindow` 와 `LV.Window` 는 `autoApplyDeviceTierPreset` 를 `false` 로 기본값으로 설정하고 `forcedDeviceTierPreset` 를 `-1` 에 유지하므로, 하위 소비 측 앱이 명시적으로 장치 계층 기본 설정 애플리케이션을 선택하지 않는 한 스톡 앱은 런타임 직접 `RenderQuality` 경로에 그대로 유지됩니다. iOS 에서 `LV.ApplicationWindow` 는 이제 프레임워크 관리형 전체 창 커버리지 경로로 기본값이 되어 렌더링 표면이 상태 바, 노치, 홈 인디케이터 영역으로 확장됩니다; Android 는 기본적으로 OS 관리형 창/내측 여백 정책을 유지합니다. 두 경로 모두 내부 뷰포트 에 고정 `16` 논리 픽셀 레이아웃 내측 여백만 적용합니다. 기존 코드베이스가 여전히 오래된 타입 이름과 `visible: true` 를 원할 때 `LV.AppBootstrapWindow` 는 호환성 래퍼로 계속 사용 가능합니다. 또한 크로스 플랫폼 런타임 타겟을 자동으로 생성합니다:
- `run_<target>_macos`
- `run_<target>_linux`
- `run_<target>_windows`
- `run_<target>_ios`
- `run_<target>_android`
- `run_<target>_wasm`
호스트 데스크탑 대상은 즉시 실행되는 반면, 호스트가 아닌 대상은 `CMAKE_SYSTEM_NAME` 재구성 힌트를 인쇄합니다. 또한 크로스 플랫폼 부트스트랩 대상을 생성합니다.
- `bootstrap_<target>_macos`
- `bootstrap_<target>_linux`
- `bootstrap_<target>_windows`
- `bootstrap_<target>_ios`
- `bootstrap_<target>_android`
- `bootstrap_<target>_wasm`
- `bootstrap_<target>_all`
또한 실행/내보내기 편의 대상을 생성합니다.
- `launch_<target>_ios`
- `launch_<target>_android`
- `launch_<target>_wasm`
- `export_<target>_xcodeproj`
- `export_<target>_android_studio`
- `export_<target>_wasm_site`
`bootstrap_<target>_all` 는 하나의 빌드 호출로 모든 플랫폼 부트스트랩 작업을 트리거합니다. 데스크톱 부트스트랩 타겟은 실행 가능 아티팩트를 생성합니다. Linux 앱 타겟은 `lvrs_add_qml_app()` 단계 `lvrs-runtime/` 옆에 실행 가능 파일을 빌드하여 LVRS 공유 라이브러리가 상대 `RPATH` 를 통해 해결됩니다. iOS 부트스트랩 는 기본적으로 Xcode 프로젝트를 생성하며 `xcrun simctl` 를 통해 시뮬레이터 앱을 설치합니다. Android 부트스트랩 는 기본적으로 Android 스튜디오 (Gradle) 프로젝트를 생성하고 `adb` 를 통해 APK 를 설치합니다. WASM 부트스트랩 는 브라우저 아티팩트를 생성하고 wasm 부트스트랩 빌드 트리에 `LVRSWasmArtifact.cmake` 엔트리 메타데이터를 작성합니다. `launch_<target>_wasm` 는 로컬 정적 HTTP 서버를 통해 wasm 빌드 트리를 제공하며 브라우저를 자동으로 열 수 있습니다. `export_<target>_wasm_site` 는 재귀적으로 wasm 웹 자산을 수집하며 (중첩된 레이아웃 안전) 감지된 엔트리로 `index.html` 리디렉션을 생성합니다. 발견 가능한 Qt 키가 없는 모든 플랫폼은 구성 시간 상태 메시지로 건너뛰며, 관련 `bootstrap_`, `launch_`, `export_` 타겟은 생성되지 않습니다. 툴체인 /prefix 오버라이드:
- `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>`
- `LVRS_BOOTSTRAP_QT_HOST_PREFIX`(크로스 플랫폼 구성 및 Android 배포 도구 조회를 위한 호스트 Qt 접두사)
- `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>`
- `LVRS_BOOTSTRAP_GENERATOR_<PLATFORM>`
- `LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET` / `LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET_<PLATFORM>`(Apple 중첩 구성 배포 대상, 플랫폼별 값이 우선)
- `LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT`(iOS 부트스트랩의 경우 기본 `ON`)
- `LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT`(Android 부트스트랩의 경우 기본 `ON`)
- `LVRS_ANDROID_STUDIO_PROJECT_DIR`(기본값: `<platform-build>/android-studio`)
- `LVRS_BOOTSTRAP_ANDROIDDEPLOYQT`(`androiddeployqt`에 대한 명시적 경로 재정의)
- `LVRS_BOOTSTRAP_ANDROID_SDK_ROOT` / `LVRS_BOOTSTRAP_ANDROID_NDK` (Android SDK/NDK 명시적 재정의)
- `LVRS_BOOTSTRAP_WASM_HOST` / `LVRS_BOOTSTRAP_WASM_PORT` / `LVRS_BOOTSTRAP_WASM_OPEN_BROWSER` (WASM 실행 서버/브라우저 동작)
- `LVRS_IOS_SIMULATOR_NAME`(기본값: `iPhone 17 Pro`)
- `LVRS_ANDROID_EMULATOR_SERIAL`(기본값: `emulator-5554`)
- `LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS`(`LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS`를 부트스트랩 재구성으로 전파)
- `LVRS_BOOTSTRAP_LVRS_ENABLE_IPO`(`LVRS_ENABLE_IPO`를 부트스트랩 재구성으로 전파)

애플리케이션 및 프레임워크 부트스트랩 동작은 최적화 제어 및 해결된 Apple 배포 타겟을 하위 플랫폼 구성에 전파합니다. macOS 자식은 비어 있지 않은 `CMAKE_OSX_DEPLOYMENT_TARGET` 부모를 상속하며, iOS 는 일반 또는 iOS 특정 부트스트랩 오버라이드가 필요하여 호스트 macOS 버전이 실수로 적용되지 않습니다. 부트스트랩 캐시 인자는 `OFF` 와 같은 명시적인 false 값을 보존하며, 실제로 빈 값만 생략됩니다. 이는 호출자가 모든 하위 플랫폼 구성에서 플랫폼 최적화 또는 IPO 를 비활성화할 수 있음을 보장합니다. `LVRS_DIR` 및 패키지 레지스트리 정책 (`CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY`, `CMAKE_FIND_USE_PACKAGE_REGISTRY`) 은 호스트 구성 캐시에서 플랫폼별 부트스트랩 재구성으로 자동으로 전파됩니다. 프레임워크 및 애플리케이션 부트스트랩 동작은 해결된 호스트 Qt 접두어를 `QT_HOST_PATH` 로 하위 구성에 전달하여, Android 및 WASM 는 Qt 이동 후 호스트 도구를 찾을 수 있습니다. `LVRS_BOOTSTRAP_QT_HOST_PREFIX` 가 `QT_HOST_PATH` 보다 우선순위를 가지며, 그렇지 않으면 호스트 접두사가 `Qt6_DIR` 에서 파생됩니다. 설치된 CMake 패키지는 부트스트랩 동작과 함께 `LVRSBootstrapCacheArgs.cmake` 를 포함하여 하위 소비 측 프로젝트가 이 타겟을 LVRS 체크아웃 없이 사용할 수 있습니다. 호스트 빌드가 설치된 다중 플랫폼 접두사에서 LVRS 를 소비할 때, 부트스트랩 는 `LVRS_DIR` 를 요청된 플랫폼 패키지 디렉토리로 자동으로 재작성합니다 (예: `<prefix>/platforms/ios/lib/cmake/LVRS` ) 따라서 iOS / Android / WASM 툴체인은 루트 접두사 패키지 발견에 의존하지 않습니다. LVRS 패키지 구성은 스크립트를 위해 플랫폼/ 툴체인 힌트 변수를 내보냅니다:
- `LVRS_LAYOUT_VERSION`
- `LVRS_ACTIVE_PLATFORM`
- `LVRS_ACTIVE_PREFIX`
- `LVRS_INSTALL_ROOT`
- `LVRS_QT_HOST_PREFIX_HINT`
- `LVRS_QT_IOS_PREFIX_HINT`
- `LVRS_QT_ANDROID_PREFIX_HINT`
- `LVRS_QT_WASM_PREFIX_HINT`
- `LVRS_ANDROID_SDK_HINT`
- `LVRS_ANDROID_NDK_HINT`
- `LVRS_EMSDK_HINT`
LVRS 부트스트랩 외부에서 수동으로 크로스 플랫폼을 구성하려면 툴체인가 설치 루트에서 `find_package()` 조회를 루트하는 경우 `LVRS_DIR`를 `<prefix>/platforms/<platform>/lib/cmake/LVRS`로 직접 설정하세요. 일회성 부트스트랩 명령 예:
```bash
cmake --build build --target bootstrap_MyApp_all
```
이 동작을 비활성화하려면 `lvrs_configure_qml_app(<target> NO_PLATFORM_RUNTIME_TARGETS)`를 사용하십시오. `lvrs_add_qml_app()`는 `SOURCES`가 생략되면 실행 준비된 진입점을 자동으로 생성할 수 있습니다. `lvrs_configure_project_defaults()`를 사용하여 Apple 번들/plist/자격, Android 패키지 소스 디렉터리/패키지 ID 및 iOS 플러그인 제외 기본값을 중앙 집중화합니다.

프레임워크 전용 부트스트랩 대상은 프로젝트 루트에서 생성됩니다.
- `bootstrap_lvrs_macos`
- `bootstrap_lvrs_linux`
- `bootstrap_lvrs_windows`
- `bootstrap_lvrs_ios`
- `bootstrap_lvrs_android`
- `bootstrap_lvrs_wasm`
- `bootstrap_lvrs_all`
`bootstrap_lvrs_all` 는 선택된 프레임워크 부트스트랩 플랫폼 세트를 `<build>/lvrs-bootstrap/framework/<platform>` 하에 빌드하고, `LVRSCore` 를 빌드하며, `${LVRS_BOOTSTRAP_INSTALL_ROOT}/<platform>` 에 설치합니다. 중첩된 플랫폼별 빌드는 `--parallel 1` 로 실행되어 상속된 `MAKEFLAGS`, `MFLAGS`, `CMAKE_BUILD_PARALLEL_LEVEL` 를 지우므로 바깥쪽 Make 작업 서버가 내부 프레임워크 빌드로 누출되지 않습니다. 기본 프레임워크 부트스트랩 플랫폼 세트는 `macos;linux;windows;ios;android;wasm` 모든 런타임 플랫폼이며, `LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS` 가 제공되지 않는 한입니다. 발견 가능한 Qt 키트가 없는 모든 플랫폼은 구성 시간 상태 메시지로 건너뜁니다. iOS 프레임워크 부트스트랩 는 또한 선택된 전체 Xcode 와 iPhoneOS SDK 를 필요로 하며, 명령줄 도구만 사용하는 호스트는 `bootstrap_lvrs_ios` 를 건너뛰고 `bootstrap_lvrs_all` 로 실패하지 않습니다. `LVRS_BOOTSTRAP_INSTALL_PREFIX_<PLATFORM>` 로 플랫폼별 설치 경로를 덮어씁니다. 크로스 호스트 플랫폼의 경우 `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>` 와 `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>` 를 통해 일치하는 Qt 키트/툴체인을 제공합니다. 애플 호환성을 위해 macOS 에 `CMAKE_OSX_DEPLOYMENT_TARGET` 로 부모를 구성하거나 `LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET_<PLATFORM>` 를 명시적으로 설정합니다; 선택된 값은 중첩된 플랫폼 캐시에 영구 저장되며 결과 Mach-O 로드 명령에 반영됩니다.

<a id="rendering-backend-enforcement"></a>

## 렌더링 백엔드 적용

구성 시 `LVRS_ENFORCE_VULKAN=ON`가 다음과 같은 경우:

- macOS/iOS는 Qt Metal 지원(`QT_FEATURE_metal >= 0`)을 제공해야 합니다.
- Android는 Qt Vulkan 지원(`QT_FEATURE_vulkan >= 0`)을 제공해야 합니다.
- `Vulkan::Vulkan`는 Vulkan 고정 대상에 대해 검색 가능한 경우 사용되지만 구성 시 부재는 경고로 처리되고 대신 런타임 로더 검색이 사용됩니다.

런타임에서:

- macOS/iOS는 Metal로 고정됩니다.
- Windows는 D3D11를 선호하고, 부트스트랩 중에 DirectX 런타임를 검색하고, DirectX를 초기화할 수 없으면 OpenGL로 대체됩니다.
- Android는 Vulkan를 선호하고, 부트스트랩 중에 런타임 로더 가용성을 조사하고, Vulkan를 초기화할 수 없는 경우 OpenGL로 대체됩니다.
- Linux는 Qt 기본 백엔드 선택을 사용합니다.
- WASM/기타 플랫폼은 Qt 기본 백엔드 선택을 대체 경로로 사용합니다.
- 필요한 고정 백엔드를 초기화할 수 없고 사용 가능한 플랫폼 대체 경로가 없으면 시작이 빠르게 실패합니다.
- 부트스트랩 stdout은 이제 렌더링 프로필, 장면 그래프 환경, 프로브 후보, 대체 경로 이유, 가져오기 경로 및 글꼴 정책 세부 정보가 포함된 구조화된 `LVRS bootstrap.*` 라인을 내보냅니다. 따라서 하위 소비 측 앱은 `LV.Debug`를 활성화하지 않고도 첫 번째 프레임 진단을 캡처할 수 있습니다.

부트스트랩 렌더링 기본값은 애플리케이션 생성 전에 보수적으로 선택됩니다. 모바일 타겟은 데스크톱 타겟보다 더 가벼운 MSAA / 비행 프레임 프로파일을 사용하므로 런타임 -direct `RenderQuality` 경로가 연결되기 전에 첫 번째 창이 더 낮은 메모리 압력으로 시작합니다. Android 와 iOS 는 모두 `4x/2` 를 유지합니다; iOS 에서 라이브 Metal 표면은 생성 후 안정적으로 유지되므로 이후 런타임 정책 변경이 반앨리어싱된 스왑체인 리소스를 종료하지 않습니다.

<a id="notes"></a>

## 메모

- Qt 6.5+와 `Quick` 및 `QuickControls2`가 필요합니다.
- 백엔드 Qt 기능 검사가 `CMakeLists.txt`에서 발생하도록 수정되었습니다.
- 백엔드 선택 로직은 `src/backend/runtime/vulkanbootstrap.cpp`에 있습니다.
- 하위 소비 측 앱 부트스트랩 템플릿은 `src/main.cpp`에서 제공됩니다(기본적으로 빌드되지 않음).
- 권장되는 재사용 가능한 부트스트랩 API는 `src/backend/runtime/appbootstrap.h`입니다.

<a id="ci-build-pipeline-example"></a>

## CI 빌드 파이프라인 예시

최소 CI 파이프라인은 일반적으로 다음 시퀀스를 실행합니다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

테스트 대상이 없는 경우 `ctest`를 유지하되 테스트 통과 의미 없음을 허용하거나 프로젝트 옵션 뒤에 게이트를 지정하세요.

<a id="p4-quality-automation-commands"></a>

## P4 품질 자동화 명령

P4 품질 자동화 스크립트는 `tests/ci/` 아래에 제공됩니다.

```bash
# PR 통과 기준(성능 + 시각적 검증)
./tests/ci/run_p4_quality.sh

# Sanitizer 조합
./tests/ci/run_p4_sanitizers.sh address
./tests/ci/run_p4_sanitizers.sh undefined

# 장시간 지속 실행 묶음
LVRS_SOAK_ITERATIONS=5000 ./tests/ci/run_p4_soak.sh
```

자세한 계약 및 임계값은 `docs/quality-automation-p4.md`에 문서화되어 있습니다.

<a id="packaging-checklist"></a>

## 포장 체크리스트

LVRS 기반 바이너리를 배송하는 경우:

1. 대상 OS에 대한 Qt 런타임 배포를 확인합니다.
2. 패키지된 아티팩트에서 아이콘/글꼴 리소스를 사용할 수 있는지 확인하세요.
3. 대상 머신의 그래픽 백엔드 요구 사항(Metal/Vulkan)을 확인하세요.
4. 운영자를 위해 환경 재정의 확인(`LVRS_APP_*`)이 문서화되어 있습니다.

<a id="runtime-smoke-test-script"></a>

## 런타임 연기 테스트 스크립트

릴리스 후보의 경우 다음을 확인하는 스모크 테스트를 실행하세요.

- 백엔드 부트스트랩 오류 없이 앱 프로세스가 시작됩니다.
- 루트 QML가 성공적으로 로드되었습니다.
- 경로 탐색은 적어도 하나의 정적 경로와 하나의 동적 경로에 대해 작동합니다.
- input/런타임 이벤트는 창과 상호 작용할 때 캡처됩니다.

<a id="installed-package-regression"></a>

## 설치된 패키지 회귀

macOS에서 `LIBRARY_PATH`는 설치된 라이브러리 디렉터리를 CMake에 암시적으로 만들 수 있습니다. 그런 다음 LVRS는 생략된 자체 런타임 검색 경로만 가져온 대상에 추가하므로 `@rpath/libLVRS.dylib`는 계속 로드됩니다. 다른 패키지 경로는 CMake의 일반적인 RPATH 처리에 따라 유지됩니다.

Source CTest 는 설치된 SDK 의 로더 오버라이드를 상속하는 대신 build-tree LVRS 라이브러리와 QML 임포트를 선택합니다. `embedded_qml_matches_source` 는 모든 내장 모듈 QML 파일과 현재 체크아웃을 비교하므로 오래된 라이브러리가 명시적으로 보고됩니다. macOS 에서 직접 테스트 실행 가능 파일을 호출할 때 `DYLD_LIBRARY_PATH="$PWD/build"` 도 설정해야 합니다.

설치된 소비자는 패키지 계약을 확인하며, 22px InputField 높이, 4.5px 텍스트 배치, 12px 섹션드 컨트롤 반지름, 154px 폼 높이, 안정된 빈 HierarchyToolbar 알림, 그리고 모든 8 `LV.Card` 타입과 그 내부 리소스 임포트를 확인합니다. 내장 소스 비교는 카드 계열이 사용하는 모듈 루트 리소스 별칭도 해결합니다. 호스트에서 작성된 macOS / iOS / Android 메트릭을 덮어씁니다; 이는 기기 실행을 구성하지 않습니다. 추가적으로 모든 내장 QML 리소스 해시를 체크아웃과 비교하려면 `LVRS_CONSUMER_SOURCE_DIR` 를 공급합니다.

```sh
LIBRARY_PATH="$HOME/.local/SDK/LVRS/platforms/macos/lib" cmake \
  -S tests/installed-consumer -B build/installed-consumer \
  -DLVRS_DIR="$HOME/.local/SDK/LVRS" \
  -DLVRS_CONSUMER_SOURCE_DIR="$PWD" \
  -DLVRS_CONSUMER_REQUIRE_IMPLICIT_LIBRARY_PATH=ON
cmake --build build/installed-consumer
ctest --test-dir build/installed-consumer --output-on-failure
```

컴파일러 암시적 디렉토리는 첫 번째 구성 중에 캡처되므로 `LIBRARY_PATH`를 변경할 때 새로운 소비자 빌드 디렉토리를 사용하십시오.

체크아웃 내부에서 `lvrs install` 를 실행할 때, 상속된 `LVRS_ROOT` 설치 접두사는 이전 소스 스냅샷을 선택해서는 안 됩니다. 명시적인 소스 디렉터리 오버라이드는 여전히 존중됩니다. 인쇄된 `Project root`, 설치기 종료 상태, 그리고 소스 CTest 결과와 독립적으로 설치된 소비자를 확인합니다.

<a id="recovering-an-interrupted-install"></a>

## 중단된 설치 복구

`cmake --build build` 는 완료된 구성 단계와 `build/CMakeCache.txt` 를 요구합니다. 툴체인 환경 소스는 변수만 내보내며 캐시를 생성하지 않습니다. 구성이 중단되었으면 `./install.sh` 를 다시 실행하거나 (또는 쉘의 `lvrs install` 래퍼를) 다시 실행합니다. 설치기는 빌드 전에 고정된 `build/` 디렉토리를 구성합니다. 시스템 위치에 `Workspace/SDK` 외부의 제 3 자 툴 체인을 설치합니다. 유닉스 래퍼는 시스템 셸 환경에서 `JAVA_HOME`, `ANDROID_HOME`, `ANDROID_NDK_ROOT`, 및 `EMSDK` 를 상속받으며; 형제 툴체인 디렉토리를 절대 자동 로드하지 않습니다. `LVRS_TOOLCHAIN_ENV_FILE` 를 명시적인 환경 파일 오버라이드에만 설정합니다. 명시적인 누락된 파일은 오류입니다.

이 macOS 워크스테이션에서 Java 는 Homebrew (`openjdk@21`) 에 의해 설치되며, Android SDK / NDK 패키지는 `/opt/homebrew/share/android-commandlinetools` 하에 `sdkmanager` 에 의해 관리되고, 버전화된 Emscripten SDK 는 `/opt/homebrew/share/emsdk` 하에 설치됩니다. Qt 6.8.3 는 NDK `26.1.10909125` 와 Emscripten `3.1.56` 를 사용합니다. 지속적인 셸 구성은 이 경로들을 내보냅니다. 컴파일러를 이동한 후, 재빌드하기 전에 고정된 `build/` 트리 하에 영향을 받는 크로스 플랫폼 빌드 캐시를 다시 생성합니다.

회귀 검사: `sh tests/test_install_wrapper.sh` 및 `cargo test --manifest-path src/rust-cli/Cargo.toml --target-dir build/rust-cli`. 여기에는 시스템 환경 상속, 무시된 형제 툴체인, 명시적 재정의, 인수 경계, CMake 정리 중 Cargo 실행 파일 보존, 누락된 빌드 디렉터리 재생성 등이 포함됩니다.
