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

## Usage Example

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    const profile = LV.Platform.runtimeProfile("ios")
    console.log("ios backend:", profile.graphicsBackend)
}
```

## Operational Notes

- Alias tokens (for example `osx`, `win32`) should be normalized before comparison.
- `runtimeProfile(target)` is the preferred API for structured target decisions.

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
