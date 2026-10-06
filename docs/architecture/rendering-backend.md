<a id="rendering-backend-policy"></a>

# 렌더링 백엔드 정책

이 문서는 LVRS가 플랫폼 전체에서 그래픽 백엔드 정책을 선택, 검증 및 보고하는 방법을 정의합니다.

<a id="bootstrap-entry"></a>

## 부트스트랩 항목

기본 부트스트랩 경로:

- `lvrs::preApplicationBootstrap(options)`
- `lvrs::postApplicationBootstrap(app, options)`

위치: `src/backend/runtime/appbootstrap.h`, `src/backend/runtime/appbootstrap.cpp`

`preApplicationBootstrap`는 그래픽 백엔드 부트스트랩 및 선택적 렌더링 품질 전역 기본값을 담당합니다.

<a id="platform-backend-matrix"></a>

## 플랫폼 백엔드 매트릭스

- macOS / iOS: Metal가 필요합니다.
- Windows: D3D11가 선호됩니다. 부트스트랩는 DirectX 런타임를 먼저 조사하고 D3D11를 초기화할 수 없는 경우 OpenGL로 대체됩니다.
- Android: Vulkan가 선호됩니다. 부트스트랩는 런타임 로더를 먼저 조사하고 시작 시 Vulkan를 사용할 수 없을 때 OpenGL로 폴백합니다.
- Linux: Qt 기본 백엔드 선택입니다.
- WASM / 기타 대상: 하드 재정의 없음; Qt 기본 백엔드 선택이 사용됩니다.

필수 백엔드를 사용할 수 없고 플랫폼 대체 경로가 없는 경우 부트스트랩는 오류 메시지와 함께 `ok == false`를 반환합니다.

<a id="build-time-enforcement"></a>

## 빌드 시간 시행

CMake 옵션: `LVRS_ENFORCE_VULKAN`(기본값 `ON`)

활성화되면 대상 플랫폼에 대해 고정 백엔드 Qt 기능을 확인할 수 없는 경우 구성이 조기에 실패합니다. 이렇게 하면 오류가 런타임에서 구성/구축 단계로 이동됩니다.

<a id="runtime-diagnostics"></a>

## 런타임 진단

부트스트랩 진단이 활성화되면 시작 로그에는 렌더링 기본값, 환경 시딩, 백엔드 프로브 후보, 선택한 로더 및 대체 경로 이유에 대한 단계별 압축 JSON 페이로드가 포함됩니다.

예:

- `LVRS bootstrap.pre.render-quality {"platform":"android", ...}`
- `LVRS bootstrap.graphics.probe {"requestedBackend":"vulkan","candidates":[...], ...}`
- `LVRS bootstrap.graphics.selected {"requestedBackend":"d3d11","selectedBackend":"d3d11","loader":"d3d11", ...}`
- `LVRS bootstrap.graphics.fallback {"requestedBackend":"vulkan","selectedBackend":"opengl","reason":"...", ...}`
- `LVRS graphics backend: opengl, loader = windows-fallback`

<a id="interaction-with-renderquality"></a>

## RenderQuality와의 상호 작용

`configureRenderQualityDefaults` 가 활성화되면 앱 생성 전에 `RenderQuality::configureGlobalDefaults()` 가 적용됩니다. 이것은 텍스트/ MSAA  기본값을 백엔드 정책과 정렬하게 유지하면서, Qt  이 존재하기 전에 런타임  (런타임) 가 요구하는 사전 애플리케이션/사전 윈도우 상태만 시드합니다. 부트스트랩 프로필도 글로벌 기본값이 적용되기 전에 파이프라인 캐시 활성화 및 애포 sizing 와 같은 시나리오그래프 환경 힌트를 심습니다. Android / iOS 는 `4x`   부트스트랩   MSAA 최소값으로 축소된 애포 sizing 을 유지하는 반면, WASM 는 가벼운 단일 프레임 부트스트랩 프로필을 사용하며 기본적으로 배치/파이프라인 캐시 힌트를 명시적으로 비활성화하고 부트스트랩 동안 데스크톱 깊이/스텐실 기본값을 강요하지 않습니다. 기본 쉘 경로는 런타임 -direct 입니다. `RenderQuality.applyWindow(...)` 는 라이브 창을 LVRS 품질 정책에 연결하는 반면, `RenderQuality.applyDeviceTierPreset(...)` 는 스톡 시작 경로 대신 명시적인 하위 소비 측 오버라이드로 남습니다. iOS 에서 LVRS 는 창이 아직 전 표시 상태일 때만 네이티브 표면 형식/그래픽 구성을 변형하며, MSAA 에 대한 플랫폼 품질 최소값과 라이브 Metal 표면이 존재하면 동적 슈퍼샘플링을 유지합니다.

<a id="failure-handling-guidance"></a>

## 실패 처리 지침

부트스트랩가 실패하는 경우:

1. 즉시 시동을 중지하고,
2. 부트스트랩 오류 메시지를 인쇄합니다.
3. 플랫폼 SDK/런타임(Metal/Vulkan) 및 Qt 기능 지원을 확인합니다.
4. 명시적 진단을 활성화하여 다시 실행합니다.

<a id="related-files"></a>

## 관련 파일

- `src/backend/runtime/vulkanbootstrap.h`
- `src/backend/runtime/vulkanbootstrap.cpp`
- `src/backend/runtime/renderquality.h`
- `src/backend/runtime/renderquality.cpp`

<a id="practical-target-matrix-validation"></a>

## 실용적인 목표 매트릭스 검증

배송하기 전에 대상 제품군당 하나의 실제 장치/시뮬레이터를 검증하십시오.

- macOS: Metal 백엔드 선택 및 안정적인 시작을 확인합니다.
- iOS: Metal 경로 및 텍스트 렌더링 품질을 확인합니다.
- Windows: D3D11 경로와 OpenGL 대체 경로 경로가 첫 번째 프레임에 도달할 수 있는지 확인합니다.
- Linux: Qt 기본 백엔드 선택으로 안정적인 시작을 확인합니다.
- Android: Vulkan 지원 장치 동작과 OpenGL 대체 경로 정책을 모두 확인합니다.
- WASM: 더 가벼운 부트스트랩 프로필로 브라우저 시작을 확인합니다.

<a id="deployment-troubleshooting"></a>

## 배포 문제 해결

부트스트랩 단계에서 시작이 실패하는 경우:

- `LVRS bootstrap.pre.*` / `LVRS bootstrap.graphics.*` 라인을 먼저 검사하고,
- Qt 빌드에 필수 렌더링 백엔드 기능이 포함되어 있는지 확인하세요.
- 대체 경로 가능 대상(Windows D3D11, Android Vulkan)에 대한 런타임 로더/드라이버 존재를 확인합니다.
- Vulkan 프로브 실패 시 Android 대체 경로 로그를 확인합니다.
- D3D11 프로브 실패 시 Windows 대체 경로 로그를 확인합니다.
- 환경 재정의가 호환되지 않는 그래픽 API를 강제하지 않는지 확인하세요.

<a id="operational-recommendation"></a>

## 운영 권장 사항

비프로덕션 빌드 및 CI 스모크 실행에서 백엔드 부트스트랩 진단을 활성화된 상태로 유지합니다. 이는 앱 수준 UI 로직이 포함되기 전에 환경 회귀를 포착합니다.
