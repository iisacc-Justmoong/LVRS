<a id="platform"></a>

# 플랫폼

위치: `src/backend/platform/platforminfo.h` / `src/backend/platform/platforminfo.cpp`

`Platform`(`PlatformInfo`)는 표준 런타임 대상 메타데이터 및 대상 정책 도우미 API를 노출합니다.

<a id="purpose"></a>

## 목적

- 현재 OS/arch/백엔드 정보를 보고합니다.
- 사용자 대상 토큰을 정규화합니다.
- 대상 기능 및 백엔드 준비 확인을 제공합니다.

<a id="properties"></a>

## 속성

신원:

- `os`
- `canonicalOs`
- `arch`
- `graphicsBackend`

가족 깃발:

- `mobile`, `desktop`
- `android`, `ios`, `macos`, `windows`, `linux`, `wasm`

백엔드 기능 플래그:

- `metalSupported`
- `vulkanSupported`

대상 카탈로그:

- `runtimeTargets`
- `desktopTargets`
- `mobileTargets`
- `runtimeProfiles`

<a id="methods"></a>

## 방법

정규화 및 일치:

- `normalizeTarget(target)`
- `isKnownTarget(target)`
- `targetMatchesCurrent(target)`

가족 수표:

- `targetIsMobile(target)`
- `targetIsDesktop(target)`

정책 확인:

- `supportsTargetGeneration(target)`
- `backendFeatureReadyFor(target)`
- `graphicsBackendFor(target?)`
- `runtimeProfile(target?)`

`runtimeProfile(target?)`는 구조화된 정책 맵을 반환합니다. 일반적인 필드는 다음과 같습니다.

- 신원/능력: `target`, `known`, `host`, `current`, `desktop`, `mobile`, `android`, `ios`, `src/backend`
- 부트스트랩/런타임 정책: `runtimeEventsAutoAttachRecommended`
- 부트스트랩 렌더링 정책: `bootstrapMsaaSamples`, `bootstrapFramesInFlight`, `bootstrapPartialUpdateRecommended`, `bootstrapBatchRenderingRecommended`, `bootstrapPipelineCacheRecommended`, `bootstrapTextureAtlasEdge`
- 모바일 위임 정책: `mobileSystemWindowDelegationRecommended`, `mobileSystemInsetsDelegationRecommended`
- 모바일 보기 정책: `mobileDisplayCoverageOverrideRecommended`, `mobileFullscreenVisibilityRecommended`, `mobileFullscreenGeometryHintRecommended`
- 적응형 보기 정책: `adaptiveWideBreakpoint`, `adaptiveNavWidth`, `adaptiveNavDrawerWidth`, `adaptiveMobileDesktopMinWidth`, `adaptiveBottomNavigationMaxItems`, `adaptiveCompactSpacingBreakpoint`, `adaptiveNavRailMaxWidthRatio`, `adaptiveDrawerMarginSafety`, `adaptiveDrawerEnterDuration`, `adaptiveDrawerExitDuration`, `adaptiveAnimatedTransitions`
- 빌드 메타데이터: `generationSupported`, `backendFeatureReady`, `cmakeSystemName`, `executableSuffix`, `sharedLibrarySuffix`, `directRunSupported`

<a id="usage-example"></a>

## 사용예

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    const profile = LV.Platform.runtimeProfile("ios")
    console.log("ios backend:", profile.backend)
}
```

<a id="operational-notes"></a>

## 운영 참고 사항

- 별칭 토큰(예: `osx`, `win32`)은 비교하기 전에 정규화되어야 합니다.
- `runtimeProfile(target)`는 구조화된 목표 결정에 선호되는 API입니다.
- 런타임 프로필 맵은 개별 QML 파일에서 플랫폼 정책을 다시 인코딩하지 않고 view/런타임 기본값을 구동하기 위한 것입니다.
- 스톡 프로필은 `runtimeEventsAutoAttachRecommended=false`를 유지합니다. 하위 소비 측 보기는 데스크톱 대상이 자동으로 시작한다고 가정하는 대신 의도적으로 `RuntimeEvents`를 선택해야 합니다.
- `src/backend`는 대상 제품군의 부트스트랩 기본 렌더러에 매핑됩니다. Apple 대상은 `metal`를 사용하고, Android는 `vulkan`를 사용하고, Windows는 `d3d11`를 선호합니다. OpenGL 대체 경로 및 Linux/WASM는 Qt 기본 백엔드 선택을 유지합니다.
- Android 와  iOS 는 모두  `4x`   부트스트랩   MSAA 바닥을 유지하며, 모바일 애포스 사이즈는  `1024` 에 유지됩니다.  iOS 에서는 첫 번째 네이티브 윈도우가 생성되기 전에 샘플 카운트 바닥이 고정되고, 이후  런타임 정책 변경은 라이브  Metal 서면을 다시 구축하려고 시도하지 않습니다.
- `mobileSystemWindowDelegationRecommended` 및 `mobileSystemInsetsDelegationRecommended`는 무조건적인 모바일 기본값이 아닌 플랫폼 프로필 스위치입니다. 현재 프로필 세트에서 Android는 OS 관리 창/삽입을 선호하는 반면, iOS는 프레임워크 관리 전체 창 범위를 선호합니다.
- `adaptive*` 키는 OS 관련 스캐폴드 측정항목의 표준 소스입니다. `ApplicationWindow`는 대략적인 `mobile/desktop` 분할에서 레이아웃 정책을 파생하는 대신 이를 직접 사용합니다.

<a id="extended-example-target-capability-gate"></a>

## 확장된 예: 목표 역량 게이트

```qml
import LVRS 1.0 as LV

function canBuildFor(target) {
    if (!LV.Platform.isKnownTarget(target))
        return false
    return LV.Platform.supportsTargetGeneration(target)
        && LV.Platform.backendFeatureReadyFor(target)
}
```

<a id="practical-notes"></a>

## 실용적인 참고 사항

- 대상 식별자를 유지하기 전에 `normalizeTarget()`를 사용하세요.
- 한 번의 호출로 여러 정책 확인이 필요한 경우 `runtimeProfile(target)`를 선호합니다.

## FAQ

Q. 원시 `os` 문자열에서 앱 논리가 분기되어야 합니까?   A. 별칭/정규화 버그를 줄이려면 정식 도우미(`targetIsMobile`, `runtimeProfile`)를 선호하세요.
