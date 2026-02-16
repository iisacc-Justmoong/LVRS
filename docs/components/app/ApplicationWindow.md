# ApplicationWindow

Location: `qml/ApplicationWindow.qml`

`ApplicationWindow` is LVRS's root shell component. It wraps `AppScaffold`, manages size classes, and wires global runtime event signals.

## Responsibilities

- Cross-platform minimum-size policy.
- Safe-margin policy for mobile footprints.
- Render-quality layer supersampling bridge (`RenderQuality`).
- Global event bridge to app-level signals.
- Native window style integration.

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

Navigation and scaffold aliases:
- `navIndex`, `navigationEnabled`, `navTitle`, `navWidth`, `headerActions`, `pageRouter`
- `scaffoldLayoutMode`, `scaffoldPreferBottomNavigation`, `scaffoldBottomNavigationMaxItems`
- `scaffoldCompactSpacingEnabled`, `scaffoldCompactSpacingBreakpoint`
- `scaffoldNavRailMaxWidthRatio`, `scaffoldDrawerMarginSafety`

Sizing and platform:
- `platform`, `isMobilePlatform`, `isDesktopPlatform`
- `widthClass`, `heightClass`, `isCompact`, `isExpanded`

## Usage

```qml
import LVRS 1.0 as LV

LV.ApplicationWindow {
    visible: true
    width: 1480
    height: 980
    title: "LVRS Visual Catalog"

    onGlobalContextEvent: function(eventData) {
        console.log(eventData.ui.path)
    }
}
```
