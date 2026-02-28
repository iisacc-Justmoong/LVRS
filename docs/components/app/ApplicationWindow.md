# ApplicationWindow

Location: `qml/ApplicationWindow.qml`

`ApplicationWindow` is the LVRS root shell that combines adaptive navigation layout, render/runtime wiring, and global event bridging.

## Purpose

- Own platform/size-class and adaptive scaffold state.
- Bridge backend-driven render policy (`RenderQuality`) into root layer behavior.
- Optionally auto-start runtime listeners and backend user-event mirror.
- Provide a page-stack host (`PageRouter`) and adaptive navigation delegates.

## Startup Sequence

On completion, main flow is:

1. `FontPolicy.enforceApplicationFallback()`
2. optional `RenderQuality.applyDeviceTierPreset(...)`
3. `RenderQuality.applyWindow(windowRoot)`
4. `SvgManager.ensureMinimumScale(effectiveSupersampleScale)`
5. optional runtime attach (`autoAttachRuntimeEvents`)
6. optional backend hook (`autoHookBackendUserEvents`)
7. native window style + mobile display coverage override refresh

## Core Property Groups

### Platform and sizing

- `platform`, `isMobilePlatform`, `isDesktopPlatform`
- `layoutClassWidth`, `layoutClassHeight`, `widthClass`, `heightClass`, `isCompact`, `isExpanded`
- `desktopMinWidth/Height`, `mobileMinWidth/Height`
- `useBackendMobileScale`, `mobileViewScale`, `effectiveMobileViewScale`
- `usePlatformSafeMargin`, `safeMargin`

### Window and platform overrides

- `windowColor`
- `forceNativeDarkTitleBar`
- `solidChrome`
- mobile coverage overrides:
  - `forceFullWindowAreaOnMobile`
  - `mobileDisplayCoverageOverrideEnabled`
  - `mobileFullscreenVisibilityOverride`
  - `mobileFullscreenGeometryHintOverride`
  - `mobileOversizedHeightEnabled`
  - `mobileOversizedHeight`
  - `mobileLayoutHeightHint`
  - `mobileOversizedHeightActive`
  - `mobileLayoutViewportHeight`
  - `mobileTopMarginFill`, `mobileBottomMarginFill`

### Runtime and event bridge

- `globalEventListenersEnabled` (default `false`)
- `autoAttachRuntimeEvents` (default follows `globalEventListenersEnabled`)
- `autoHookBackendUserEvents` (default `false`)
- `lastGlobalPressedEventData`, `lastGlobalContextEventData`
- signals: `globalPressedEvent(...)`, `globalContextEvent(...)`

### Render-quality bridge

- `inactiveRenderDowngradeEnabled`
- `inactiveRenderMsaaSamples`
- `autoApplyDeviceTierPreset`
- `forcedDeviceTierPreset`
- `effectiveSupersampleScale`, `sceneSupersamplingActive`
- internal supersample host uses backend-resolved texture sizing and mipmap policy

### Adaptive scaffold and page-stack API

Aliases to internal scaffold include:

- navigation model: `navItems`, `navIndex`, `navigationEnabled`
- layout policy: `scaffoldLayoutMode`, `scaffoldLayoutPlatform`, `scaffoldForceDesktopOnLargeMobile`, `scaffoldMobileDesktopMinWidth`
- navigation mode policy: `scaffoldPreferBottomNavigation`, `scaffoldBottomNavigationMaxItems`, `scaffoldNavRailMaxWidthRatio`, `scaffoldDrawerMarginSafety`
- page stack: `pageRoutes`, `pageInitialPath`, `useInternalPageStack`, `activePageRouter`, `internalPageStackEnabled`

Adaptive state outputs:

- `adaptiveLayoutProfile`
- `adaptiveNavigationMode`
- `adaptiveMobileLayout`, `adaptiveDesktopLayout`
- `adaptiveRailNavigation`, `adaptiveDrawerNavigation`, `adaptiveBottomNavigation`

Signals:

- `navActivated(index, item)`
- `adaptiveLayoutStateChanged(profile, navigationMode)`
- `pageStackNavigated(path, params)`
- `pageStackNavigationFailed(path)`

### Backend adaptive policy

- `useBackendAdaptivePolicy`
- `backendAdaptivePolicyOverrides`
- `backendRuntimeProfile`
- `backendAdaptivePolicyDefaults`
- `backendAdaptivePolicy`
- resolved numeric policy outputs:
  - `backendWideBreakpoint`, `backendNavWidth`, `backendNavDrawerWidth`
  - `backendMobileDesktopMinWidth`, `backendBottomNavigationMaxItems`
  - `backendCompactSpacingBreakpoint`, `backendNavRailMaxWidthRatio`, `backendDrawerMarginSafety`
  - `backendDrawerEnterDuration`, `backendDrawerExitDuration`
  - `backendAnimatedTransitions`

## Key Methods

- `matchesMedia(rule)`
- `ensureRuntimeEventsAttached()`
- `applyNativeWindowStyle()`
- `applyMobileDisplayCoverageOverride()`
- `requestWindowMove()`

## Behavior Notes

- Adaptive layout transitions are guarded to avoid invalid one-step transitions and resize oscillation.
- `globalEventListenersEnabled` and `autoHookBackendUserEvents` are independent; enabling one does not force the other.
- Runtime attach and backend hook are feature-flagged; both can be fully disabled for constrained hosts.
- Mobile safe-area fill strategy can force an intentionally oversized window height while keeping layout computation bounded to `mobileLayoutViewportHeight`.
- The oversized remainder is treated as non-layout top/bottom margin fill and painted with `windowColor`.

## Usage

```qml
import LVRS 1.0 as LV

LV.ApplicationWindow {
    visible: true
    width: 1280
    height: 800

    globalEventListenersEnabled: true
    useInternalPageStack: true
    pageRoutes: [
        { path: "/", component: homePage },
        { path: "/reports", component: reportsPage }
    ]
}
```
