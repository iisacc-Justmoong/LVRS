# ApplicationWindow

Location: `qml/ApplicationWindow.qml`

`ApplicationWindow` is LVRS's root window component.
It hosts the adaptive layout/navigation content directly, manages size classes, and wires global runtime event signals.
No top header/title panel is injected by default.

## Responsibilities

- Cross-platform minimum-size policy.
- Safe-margin policy for mobile footprints.
- Render-quality layer supersampling bridge (`RenderQuality`).
- Global event bridge to app-level signals.
- Native window style integration.
- Plain window background via `windowColor` (`Theme.window` by default).

## Global Event Signals

- `globalPressedEvent(eventData)`
- `globalContextEvent(eventData)`

Internally, these are emitted through two always-on `EventListener` instances using triggers:
- `globalPressed`
- `globalContextRequested`

Each payload includes coordinates and resolved UI/input context.

## Runtime Boot Sequence

On completion, `ApplicationWindow`:

1. enforces font fallback policy,
2. applies render quality,
3. starts and attaches `RuntimeEvents`,
4. hooks backend user events via `Backend.hookUserEvents()`,
5. emits creation/debug logs.

## Key Aliases and Properties

Window/platform:
- `windowColor`, `forceNativeDarkTitleBar`
- `platform`, `isMobilePlatform`, `isDesktopPlatform`
- `widthClass`, `heightClass`, `isCompact`, `isExpanded`
- `desktopMinWidth`, `desktopMinHeight`, `mobileMinWidth`, `mobileMinHeight`
- `usePlatformSafeMargin`, `safeMargin`

Scaffold and navigation aliases:
- `navIndex`, `navigationEnabled`
- `navTitle`, `navTitleVisible`
- `navWidth`, `navDrawerWidth`
- `wideBreakpoint`
- `scaffoldLayoutMode`, `scaffoldLayoutPlatform`
- `scaffoldForceDesktopOnLargeMobile`, `scaffoldMobileDesktopMinWidth`
- `scaffoldPreferBottomNavigation`, `scaffoldBottomNavigationMaxItems`
- `scaffoldCompactSpacingEnabled`, `scaffoldCompactSpacingBreakpoint`
- `scaffoldNavRailMaxWidthRatio`, `scaffoldDrawerMarginSafety`
- `navDelegate`, `navHeader`, `navFooter`

Routing aliases:
- `pageRouter`
- `pageRoutes`, `pageInitialPath`
- `useInternalPageStack`
- `internalRouterRegisterAsGlobalNavigator`
- `internalPageStackEnabled` (readonly)
- `activePageRouter` (readonly)

Adaptive state:
- `adaptiveLayoutProfile`
- `adaptiveNavigationMode`
- `adaptiveMobileLayout`, `adaptiveDesktopLayout`
- `adaptiveRailNavigation`, `adaptiveDrawerNavigation`, `adaptiveBottomNavigation`

Event/runtime:
- `globalEventListenersEnabled`
- `lastGlobalPressedEventData`, `lastGlobalContextEventData`
- `autoAttachRuntimeEvents`, `autoHookBackendUserEvents`
- `matchesMedia(rule)`

Content slot:
- `default property alias content`

## Usage

```qml
import LVRS 1.0 as LV

LV.ApplicationWindow {
    visible: true
    width: 1280
    height: 800
    title: "LVRS"

    navigationEnabled: false

    onGlobalContextEvent: function(eventData) {
        console.log(eventData.ui.path)
    }
}
```
