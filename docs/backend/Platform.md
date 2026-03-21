# Platform

Location: `backend/platform/platforminfo.h` / `backend/platform/platforminfo.cpp`

`Platform` (`PlatformInfo`) exposes canonical runtime target metadata and target-policy helper APIs.

## Purpose

- Report current OS/arch/backend information.
- Normalize user target tokens.
- Provide target capability and backend readiness checks.

## Properties

Identity:

- `os`
- `canonicalOs`
- `arch`
- `graphicsBackend`

Family flags:

- `mobile`, `desktop`
- `android`, `ios`, `macos`, `windows`, `linux`, `wasm`

Backend capability flags:

- `metalSupported`
- `vulkanSupported`

Target catalogs:

- `runtimeTargets`
- `desktopTargets`
- `mobileTargets`
- `runtimeProfiles`

## Methods

Normalization and matching:

- `normalizeTarget(target)`
- `isKnownTarget(target)`
- `targetMatchesCurrent(target)`

Family checks:

- `targetIsMobile(target)`
- `targetIsDesktop(target)`

Policy checks:

- `supportsTargetGeneration(target)`
- `backendFeatureReadyFor(target)`
- `graphicsBackendFor(target?)`
- `runtimeProfile(target?)`

`runtimeProfile(target?)` returns a structured policy map. Common fields include:

- identity/capability: `target`, `known`, `host`, `current`, `desktop`, `mobile`, `android`, `ios`, `backend`
- bootstrap/runtime policy: `runtimeEventsAutoAttachRecommended`
- render bootstrap policy: `bootstrapMsaaSamples`, `bootstrapFramesInFlight`, `bootstrapPartialUpdateRecommended`, `bootstrapBatchRenderingRecommended`, `bootstrapPipelineCacheRecommended`, `bootstrapTextureAtlasEdge`
- mobile delegation policy: `mobileSystemWindowDelegationRecommended`, `mobileSystemInsetsDelegationRecommended`
- mobile view policy: `mobileDisplayCoverageOverrideRecommended`, `mobileFullscreenVisibilityRecommended`, `mobileFullscreenGeometryHintRecommended`
- adaptive view policy: `adaptiveWideBreakpoint`, `adaptiveNavWidth`, `adaptiveNavDrawerWidth`, `adaptiveMobileDesktopMinWidth`, `adaptiveBottomNavigationMaxItems`, `adaptiveCompactSpacingBreakpoint`, `adaptiveNavRailMaxWidthRatio`, `adaptiveDrawerMarginSafety`, `adaptiveDrawerEnterDuration`, `adaptiveDrawerExitDuration`, `adaptiveAnimatedTransitions`
- build metadata: `generationSupported`, `backendFeatureReady`, `cmakeSystemName`, `executableSuffix`, `sharedLibrarySuffix`, `directRunSupported`

## Usage Example

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    const profile = LV.Platform.runtimeProfile("ios")
    console.log("ios backend:", profile.backend)
}
```

## Operational Notes

- Alias tokens (for example `osx`, `win32`) should be normalized before comparison.
- `runtimeProfile(target)` is the preferred API for structured target decisions.
- The runtime-profile map is intended to drive view/runtime defaults without re-encoding platform policy in individual QML files.
- `backend` maps to the bootstrap-preferred renderer for the target family: Apple targets use `metal`, Android uses `vulkan`, Windows prefers `d3d11` with OpenGL fallback, and Linux/WASM keep Qt default backend selection.
- `mobileSystemWindowDelegationRecommended` and `mobileSystemInsetsDelegationRecommended` are the preferred switches when a view wants mobile-safe defaults that let the OS own fullscreen transitions, cutout areas, and other critical insets.
- The `adaptive*` keys are the canonical source for OS-specific scaffold metrics. `ApplicationWindow` consumes them directly instead of deriving layout policy from a coarse `mobile/desktop` split.

## Extended Example: Target Capability Gate

```qml
import LVRS 1.0 as LV

function canBuildFor(target) {
    if (!LV.Platform.isKnownTarget(target))
        return false
    return LV.Platform.supportsTargetGeneration(target)
        && LV.Platform.backendFeatureReadyFor(target)
}
```

## Practical Notes

- Use `normalizeTarget()` before persisting target identifiers.
- Prefer `runtimeProfile(target)` when multiple policy checks are needed in one call.

## FAQ

Q. Should app logic branch on raw `os` string?  
A. Prefer canonical helpers (`targetIsMobile`, `runtimeProfile`) to reduce alias/normalization bugs.
