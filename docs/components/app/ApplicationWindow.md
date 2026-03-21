# ApplicationWindow

Location: `qml/ApplicationWindow.qml`

`ApplicationWindow` is the LVRS root shell that combines adaptive navigation layout, render/runtime wiring, and global event bridging.
`ApplicationWindow` now also carries the standard downstream bootstrap contract, so consumer app roots can mount directly on it without going through `LV.AppBootstrapWindow`.

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
- `backendRuntimeProfile`, `canonicalPlatform`
- `layoutClassWidth`, `layoutClassHeight`, `widthClass`, `heightClass`, `isCompact`, `isExpanded`
- `desktopMinWidth/Height`, `mobileMinWidth/Height`
- `useBackendMobileScale`, `mobileViewScale`, `effectiveMobileViewScale`
- `usePlatformSafeMargin`, `safeMargin`

### Window and platform overrides

- `windowColor`
- `forceNativeDarkTitleBar`
- `solidChrome`
- profile-driven mobile policy helpers:
  - `runtimeEventsAutoAttachRecommended`
  - `mobileSystemWindowDelegationRecommended`
  - `mobileSystemInsetsDelegationRecommended`
  - `mobileDisplayCoverageOverrideRecommended`
  - `mobileFullscreenVisibilityRecommended`
  - `mobileFullscreenGeometryHintRecommended`
- OS-delegation defaults:
  - `delegateMobileWindowingToSystem`
  - `delegateMobileInsetsToSystem`
- mobile coverage overrides:
  - `forceFullWindowAreaOnMobile` (defaults to `false` on mobile while `delegateMobileInsetsToSystem=true`)
  - `mobileDisplayCoverageOverrideEnabled` (profile-driven capability; masked off while `delegateMobileWindowingToSystem=true`)
  - `mobileFullscreenVisibilityOverride` (profile-driven capability; masked off while `delegateMobileWindowingToSystem=true`)
  - `mobileFullscreenGeometryHintOverride` (profile-driven capability; masked off while `delegateMobileWindowingToSystem=true`)
  - `mobileOversizedHeightEnabled` (default `false`; opt in only for explicit oversized-surface workarounds)
  - `mobileOversizedHeight`
  - `mobileLayoutHeightHint`
  - `mobileOversizedHeightActive`
  - `mobileLayoutViewportHeight`
  - `mobileTopMarginFill`, `mobileBottomMarginFill`

### Runtime and event bridge

- `globalEventListenersEnabled` (default `false`)
- `autoAttachRuntimeEvents` (default follows `runtimeEventsAutoAttachRecommended || globalEventListenersEnabled`)
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

Default quality-first profile in current implementation:

- `mobileViewScale: 1.0` (avoid unnecessary scaled composition blur in default path)
- `inactiveRenderDowngradeEnabled: false`
- `inactiveRenderMsaaSamples: 8`
- `autoApplyDeviceTierPreset: true`
- `forcedDeviceTierPreset: -1` (auto-detect tier)

Default app-root bootstrap profile in current implementation:

- `navigationEnabled: false`
- `useInternalPageStack: true`
- `internalRouterRegisterAsGlobalNavigator: true`
- `mobileOversizedHeightEnabled: false`
- `initialRoutePath: "/"`
- `pageInitialPath` follows `initialRoutePath` until a downstream app overrides it directly

### Adaptive scaffold and page-stack API

Aliases to internal scaffold include:

- navigation model: `navItems`, `navIndex`, `navigationEnabled`
- layout policy: `scaffoldLayoutMode`, `scaffoldLayoutPlatform`, `scaffoldForceDesktopOnLargeMobile`, `scaffoldMobileDesktopMinWidth`
- navigation mode policy: `scaffoldPreferBottomNavigation`, `scaffoldBottomNavigationMaxItems`, `scaffoldNavRailMaxWidthRatio`, `scaffoldDrawerMarginSafety`
- page stack: `initialRoutePath`, `pageRoutes`, `pageInitialPath`, `useInternalPageStack`, `activePageRouter`, `internalPageStackEnabled`

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
- runtime-profile driven adaptive keys:
  - `adaptiveWideBreakpoint`, `adaptiveNavWidth`, `adaptiveNavDrawerWidth`
  - `adaptiveMobileDesktopMinWidth`, `adaptiveBottomNavigationMaxItems`
  - `adaptiveCompactSpacingBreakpoint`, `adaptiveNavRailMaxWidthRatio`, `adaptiveDrawerMarginSafety`
  - `adaptiveDrawerEnterDuration`, `adaptiveDrawerExitDuration`, `adaptiveAnimatedTransitions`
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
- `applyMobileDisplayCoverageOverride()` (applies or releases framework-managed fullscreen/geometry hints)
- `requestWindowMove()`

## Behavior Notes

- Adaptive layout transitions are guarded to avoid invalid one-step transitions and resize oscillation.
- Adaptive scaffold metrics now come from `Platform.runtimeProfile()` on a per-OS basis rather than being inferred from a coarse mobile/desktop family split.
- `globalEventListenersEnabled` and `autoHookBackendUserEvents` are independent; enabling one does not force the other.
- Runtime attach and backend hook are feature-flagged; both can be fully disabled for constrained hosts.
- The standard bootstrap route contract now lives in `ApplicationWindow` itself, so downstream projects can seed `initialRoutePath` through `QmlAppLaunchSpec::initialProperties` without wrapping the root type.
- Mobile system delegation defaults are platform-aware: on iOS and Android the root now prefers OS-managed windowing and insets, so `ApplicationWindow` no longer force-enters fullscreen or rebinds the content root unless a project explicitly opts back into those overrides.
- Android still exposes the legacy fullscreen coverage path through `mobileDisplayCoverageOverrideEnabled`, `mobileFullscreenVisibilityOverride`, and `mobileFullscreenGeometryHintOverride`; disabling `delegateMobileWindowingToSystem` is the first step when a downstream app intentionally wants that path back.
- Mobile safe-area fill keeps default layout bounds tied to the visible viewport. Enable `mobileOversizedHeightEnabled` only when an app explicitly needs the older oversized-surface workaround.
- The oversized remainder is treated as non-layout top/bottom margin fill and painted with `windowColor`.

## Usage

```qml
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    visible: true
    width: 430
    height: 932

    title: "MyApp"
    pageRoutes: [
        { path: "/", component: homePage }
    ]

    Component {
        id: homePage
        Item {}
    }
}
```
