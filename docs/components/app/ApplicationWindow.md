# ApplicationWindow

Location: `qml/ApplicationWindow.qml`

`ApplicationWindow` is the LVRS root shell that combines adaptive navigation layout, render/runtime wiring, and global event bridging.
`ApplicationWindow` now also carries the standard downstream bootstrap contract, so consumer app roots can mount directly on it without going through `LV.AppBootstrapWindow`.

## Purpose

- Own platform/size-class and adaptive scaffold state.
- Bridge backend-driven render policy (`RenderQuality`) into root layer behavior.
- Optionally auto-start runtime listeners and backend user-event mirror.
- Provide a page-stack host (`PageRouter`) and adaptive navigation delegates.
- Apply an app's primary accent to the shared LVRS theme.

## App Primary Color

Set `primaryColor` on the app root to brand primary buttons, selection highlights,
sliders, checked controls, links, navigation, and Alert actions:

```qml
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: app
    visible: true
    primaryColor: "#A571E6"

    LV.LabelButton { text: "Continue" }
}
```

The initial fallback is LVRS blue, `Theme.defaultPrimary` (`#0A84FF`). The input
also accepts a QML color binding or a `primaryColor` entry in
`QQmlApplicationEngine::setInitialProperties()`. Changing `app.primaryColor`
at runtime updates existing controls and controls created later. Assign
`LV.Theme.defaultPrimary` to restore blue.

Theme colors are shared by windows in the same QML engine. Set the color on the
main app root; auxiliary windows that omit `primaryColor` inherit the current
theme without resetting it. Multiple explicit inputs update that same theme;
they do not establish independent window palettes. Separate QML engines retain
separate themes. `AppBootstrapWindow` and `AppShell` inherit the input too.

Explicit control color overrides remain in effect. Status colors (`danger`,
`warning`, `success`), neutral surfaces, and named icon palette colors keep their
existing meanings. The window's Qt Quick Controls `palette.highlight` and
`palette.link` also follow the shared primary color.

## Startup Sequence

On completion, main flow is:

1. `FontPolicy.enforceApplicationFallback()`
2. optional `RenderQuality.applyDeviceTierPreset(...)` when `autoApplyDeviceTierPreset == true`
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
- `mobileSystemSafeLeftInset/TopInset/RightInset/BottomInset`
- `mobileSystemSafeAreaResolved`, `mobileSystemSafeAreaBounds`
- `layoutSafeLeftInset/TopInset/RightInset/BottomInset`
- `renderSurfaceBounds`, `layoutSafeAreaBounds`

### Window and platform overrides

- `primaryColor` (app accent; initially `Theme.defaultPrimary`, `#0A84FF`)
- `windowColor`
- `windowBackgroundOpacity` (default 0.50; background fill only)
- `backgroundBlurEnabled` (default on when the native backend supports it)
- read-only `backgroundBlurSupported`, `backgroundBlurActive`
- `forceNativeDarkTitleBar`
- `solidChrome`
- system chrome interaction:
  - `windowChromeInteractionsEnabled`
  - `windowDragHandleEnabled`, `windowDragHandleHeight`
  - `windowDragHandleTopMargin/LeftMargin/RightMargin`
  - `windowDragExclusionItems`
  - `windowResizeHandlesEnabled`, `windowResizeEdges`
  - `windowResizeBorderThickness`, `windowResizeCornerSize`
  - `windowChromeInteractionZ`
  - read-only `windowChromeInteractionLayer`, `windowDragHandleItem`
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
  - `forceFullWindowAreaOnMobile` (defaults to `true` on mobile so the render surface stays full-bleed)
  - `mobileDisplayCoverageOverrideEnabled` (profile-driven capability; masked off while `delegateMobileWindowingToSystem=true`)
  - `mobileFullscreenVisibilityOverride` (profile-driven capability; masked off while `delegateMobileWindowingToSystem=true`; on iOS this resolves to maximized edge-to-edge coverage instead of hiding the status bar)
  - `mobileFullscreenGeometryHintOverride` (profile-driven capability; masked off while `delegateMobileWindowingToSystem=true`)
  - `mobileOversizedHeightEnabled` (default `false`; opt in only for explicit oversized-surface workarounds)
  - `mobileOversizedHeight`
  - `mobileLayoutHeightHint`
  - `mobileOversizedHeightActive`
  - `mobileLayoutViewportHeight`
  - `mobileTopMarginFill`, `mobileBottomMarginFill`

### Runtime and event bridge

- `globalEventListenersEnabled` (default `false`)
- `autoAttachRuntimeEvents` (default follows `globalEventListenersEnabled`; stock runtime profiles keep `runtimeEventsAutoAttachRecommended=false`)
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

Default runtime-direct quality profile in current implementation:

- `mobileViewScale: 1.0` (avoid unnecessary scaled composition blur in default path)
- `inactiveRenderDowngradeEnabled: false`
- `inactiveRenderMsaaSamples: 8`
- `autoApplyDeviceTierPreset: false`
- `forcedDeviceTierPreset: -1` (reserved for opt-in device-tier application)

Default mobile sizing contract in current implementation:

- `Theme` uses the same authored metric and typography values on desktop, iOS, and Android. Both token scale factors are `1.0`; Body size and line height remain `13px`.
- `usePlatformSafeMargin` defaults to `true` on iOS and Android, and `safeMargin` defaults to a fixed `16` logical pixels on each side.
- The render surface remains full-bleed on mobile; `ApplicationWindow` forces its automatic `contentItem` safe-area paddings back to `0` in the default mobile path.
- The scaffold content is also full-bleed by default. `safeMargin` no longer constrains the internal content host automatically.
- `layoutSafeAreaBounds` is now a helper rectangle only: it describes the fixed layout inset derived from `safeMargin`, but LVRS does not automatically place app content inside that box.
- `mobileSystemSafeLeftInset/TopInset/RightInset/BottomInset` and `mobileSystemSafeAreaBounds` expose the real platform safe-area margins so downstream apps can decide which regions to reserve.
- `mobileViewScale` defaults to `1.0`; the default path adds no Theme or composition multiplier.

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
- navigation icon sizing: `navigationIconSize` (defaults to `Theme.iconSm`)
- layout policy: `scaffoldLayoutMode`, `scaffoldLayoutPlatform`, `scaffoldForceDesktopOnLargeMobile`, `scaffoldMobileDesktopMinWidth`
- navigation mode policy: `scaffoldPreferBottomNavigation`, `scaffoldBottomNavigationMaxItems`, `scaffoldNavRailMaxWidthRatio`, `scaffoldDrawerMarginSafety`
- page stack: `initialRoutePath`, `pageRoutes`, `pageInitialPath`, `useInternalPageStack`, `activePageRouter`, `internalPageStackEnabled`
- interactive transition bridge: `pageTransitionController`

Stock adaptive-navigation delegates render `icon`, `iconName`, or `symbol` text glyphs in a square `navigationIconSize` frame. Its default follows `Theme.iconSm` (`18 x 18` on desktop, `36 x 36` on mobile), so rail, drawer, and bottom-navigation icons follow the same compact icon contract while remaining explicitly overridable.

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
- `mobileCoverageTargetVisibilityForPlatform(platformName)` (maps iOS to `Window.Maximized` and other mobile coverage paths to `Window.FullScreen`)
- `applyMobileDisplayCoverageOverride()` (applies or releases framework-managed edge-to-edge visibility, fullscreen-geometry, and expanded-client-area hints)
- `requestWindowMove()`
- `requestWindowResize(edges)`

## Behavior Notes

- Adaptive layout transitions are guarded to avoid invalid one-step transitions and resize oscillation.
- Solid desktop chrome uses the same system move and eight-region native-first resize layer as `LV.Window`, including the min/max-constrained macOS fallback when Qt Cocoa rejects system resize. Set `windowDragHandleEnabled` or `windowResizeHandlesEnabled` to `false` when application-owned hit regions call the request methods directly; use `windowDragExclusionItems` to keep title-bar controls interactive.
- `windowMoveAttempted(started)` and `windowResizeAttempted(edges, started)` report whether the platform accepted each pointer-initiated request.
- Adaptive scaffold metrics now come from `Platform.runtimeProfile()` on a per-OS basis rather than being inferred from a coarse mobile/desktop family split.
- `globalEventListenersEnabled` and `autoHookBackendUserEvents` are independent; enabling backend user-event mirroring does not force global listeners.
- Runtime attach and backend hook are feature-flagged; both can be fully disabled for constrained hosts.
- `scaffoldLayoutPlatform` is normalized through `Platform.normalizeTarget()` before adaptive mobile/desktop policy is resolved, so aliases such as `osx`, `ios-simulator`, and `android-arm64` are safe.
- The standard bootstrap route contract now lives in `ApplicationWindow` itself, so downstream projects can seed `initialRoutePath` through `QmlAppLaunchSpec::initialProperties` without wrapping the root type.
- Stock shells now default to the runtime-direct `RenderQuality` path; automatic device-tier preset application is disabled unless a downstream app explicitly turns `autoApplyDeviceTierPreset` back on.
- Mobile system delegation defaults are platform-aware: Android still prefers OS-managed windowing/insets, while iOS now defaults to the framework-managed full-window coverage path so the render surface can extend into the status-bar, notch, and home-indicator regions.
- When that framework-managed coverage path is active, `ApplicationWindow` now pushes the underlying `QWindow` through `MaximizeUsingFullscreenGeometryHint` and, on Qt 6.9+, `ExpandedClientAreaHint`/`NoTitleBarBackgroundHint` as well.
- iOS coverage now uses `Window.Maximized` together with `MaximizeUsingFullscreenGeometryHint`, which keeps the system status indicators visible while letting the render surface reach into the status-bar, notch, and home-indicator regions.
- Android still exposes the legacy fullscreen coverage path through `mobileDisplayCoverageOverrideEnabled`, `mobileFullscreenVisibilityOverride`, and `mobileFullscreenGeometryHintOverride`; disabling `delegateMobileWindowingToSystem` is the first step when a downstream app intentionally wants that path back.
- `WindowSafeAreaObserver` is the low-level API for downstream apps that want direct access to the platform safe-area margins without routing through the root convenience properties.
- Mobile safe-area fill keeps default layout bounds available as metadata only. Enable `mobileOversizedHeightEnabled` only when an app explicitly needs the older oversized-surface workaround.
- The oversized remainder is treated as non-layout top/bottom margin fill and painted with `windowColor`.
- Default mobile sizing uses the same Theme tokens as desktop. Downstream apps can override individual component metrics or explicitly opt into `mobileViewScale`.
- `pageTransitionController` always follows the router that `activePageRouter` currently resolves to, so shell-level gesture drivers do not need to repeat router lookup.

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

## Default background material

The standard `background` is WindowMaterial with a uniform near-black fill (#0B0B0B) at 50% opacity and no gradients. On macOS an active native frosted backdrop also blurs other windows and the desktop behind this window. Window/foreground opacity remains 1; `windowBackgroundOpacity` controls only the background fill. `windowColor` defaults to Theme.materialWindowFill (#0B0B0B), independently of primaryColor, and remains explicitly overridable. Buttons and selection keep their app accent. The 64px material blur remains available for captured backdrops. The background remains full-bleed, with outer border/corner/elevation delegated to native window chrome. Consumers can override `background` normally. See [Materials](../surfaces/Materials.md).

## Shared motion

The drawer enters with a bounded rebound; navigation controls use the shared press response. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.

`materialBackdropSource` exposes the app content/supersampling host without the sibling popup overlay. ContextMenu captures this item together with the window background, so content behind a menu is blurred without capturing the menu itself. This preserves capture safety during motion and resizing.
