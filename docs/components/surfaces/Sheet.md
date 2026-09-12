# Sheet

`LV.Sheet` is a content host with a mobile bottom sheet and a centered desktop modal. It implements the [LVRS Sheet design](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=921-14595). The supplied view owns its data, inputs, and actions; Sheet owns placement, header, scrolling, and dismissal.

## Device corner radius

`cornerRadius` is a **real number in Qt Quick logical pixels**, including fractional values. Pass the measured display corner radius from the application's device-metrics provider. A larger measurement produces a rounder sheet regardless of the OS name. Zero explicitly means square corners. Invalid, infinite, and negative values resolve to zero; oversized values are limited to half the current surface's shorter dimension.

```qml
import QtQuick
import LVRS as LV

LV.Sheet {
    id: exportSheet
    title: "Export image"
    // deviceMetrics is supplied by the application, not an LVRS singleton.
    // If its measurement is already logical pixels, bind that value directly.
    cornerRadius: mobilePresentation
        ? deviceMetrics.displayCornerRadiusPixels / deviceMetrics.devicePixelRatio
        : 16
    contentComponent: exportView
}
```

The pixel ratio must describe the display on which the measurement was made. Do not multiply or divide a logical measurement again. Qt renders logical dimensions at the window's device pixel ratio; see [Qt high DPI coordinates](https://doc.qt.io/qt-6/highdpi.html). For example, illustrative inputs of 84 physical pixels at DPR 3 and 165 physical pixels at DPR 3 produce radii of 28 and 55. These are comparison fixtures, **not specifications for Android or iPhone models**.

The default radius (mobile 24, desktop 16) is a Figma fallback, not detected hardware. Sheet does not query private iOS APIs, infer radius from a safe-area inset, or maintain a device-name table. The caller is responsible for providing the actual device measurement and updating it when the host display changes. All four surface corners use the supplied radius, including the mobile bottom corners that meet the display edge. The shape is Qt's circular rounded rectangle; a radius alone does not describe a hardware superellipse.

## Content

Pass a reusable `Component` through `contentComponent`. The instantiated view is exposed as `loadedContent`; it stays alive across close/reopen and is replaced when the component changes. Set its `implicitHeight` for Fit sizing. Its width follows the content viewport.

```qml
Component {
    id: exportView
    Column {
        spacing: LV.Theme.gap12
        LV.Label { text: "Project overview.png" }
        LV.LabelButton { text: "Export image"; onClicked: exportSheet.close() }
    }
}
LV.LabelButton { text: "Export"; onClicked: exportSheet.open() }
```

Alternatively, put an Item or layout directly inside Sheet's default `content` slot. Bind its width to `parent.width`. Only one content path is displayed at a time: `contentComponent` takes precedence over inline children.

```qml
LV.Sheet {
    id: shareSheet
    title: "Share project"
    cornerRadius: 55 // illustration; bind the device measurement in an app
    Column {
        width: parent.width
        spacing: LV.Theme.gap12
        LV.Label { text: "Invite people to this project." }
        LV.LabelButton { text: "Copy link"; onClicked: shareSheet.close() }
    }
}
```

The built-in content Flickable scrolls oversized content while the header stays fixed. For a view that owns a ListView plus fixed bottom actions, set `scrollContent: false`; the loaded view fills the available content area and owns its internal scrolling. Avoid putting a second interactive Flickable inside the default scrolling viewport.

## API

| Property | Default / meaning |
| --- | --- |
| `presentation` | `Sheet.Automatic`; resolves from `Theme.mobileTarget`. `Sheet.Mobile` and `Sheet.Desktop` support explicit previews. |
| `detent` | `Sheet.Fit`; `Sheet.Medium` uses 57% and `Sheet.Large` 90% of the mobile viewport height. |
| `cornerRadius` | Mobile 24 / desktop 16 logical pixels; caller supplies measured device radius. |
| `resolvedCornerRadius` | Sanitized and geometry-bounded radius actually rendered. |
| `title`, `description` | Header text. |
| `showHeader`, `showDescription`, `showCloseButton` | `true`; close target is 44 × 44. |
| `showGrabber` | Follows mobile presentation. Dragging this area down can dismiss. |
| `preferredWidth`, `preferredHeight` | Desktop 560 × 420; constrained by viewport and `desktopMargin` (24). |
| `topSafeInset`, `bottomSafeInset` | Live `WindowSafeAreaObserver` measurements; override for embedded previews. |
| `scrollContent` | `true`; false lets the content view own scrolling and fixed actions. |
| `dismissOnBackground`, `dismissOnEscape`, `dismissOnDrag` | `true`; programmatic `close()` always remains available. |
| `animationDuration` | Mobile 320 ms / desktop 200 ms; set to zero for reduced-motion hosts or tests. |
| `contentComponent`, `loadedContent`, `content` | Component input, instantiated view, and inline data slot. |

Use the standard `Popup` lifecycle: `open()`, `close()`, `visible`, `opened`, `aboutToShow`, `aboutToHide`, and `closed`. This differs from the existing `LV.Modal.open` boolean. Modal focus containment, overlay stacking, outside-click blocking, Escape handling, and focus return use Qt Quick Controls. Override `modal`/`dim` only for embedded catalog demonstrations. Content actions and unsaved-change policy remain application responsibilities.

Mobile height is bounded by the available top safe area; bottom safe area is reserved inside the sheet. Desktop remains centered as its parent resizes. Sheet does not draw a simulated OS home indicator.

## Dependencies and verification

Uses the existing Qt Quick Controls `Popup`, Flickable, LVRS labels/buttons/tokens and `WindowSafeAreaObserver`. No additional library, package, license, or deployment dependency is introduced. Keeping interaction in the maintained Qt implementation avoids a separate modal input stack. Qt reference: [Popup](https://doc.qt.io/qt-6.8/qml-qtquick-controls-popup.html).

`LVRSTests_sheet` covers measured-radius changes, invalid/zero/oversized inputs, responsive placement, Fit/Medium/Large sizing, Component and inline content, bounded scrolling, modal input blocking, dismissal, focus return, and the Visual Catalog example. Build and run from the repository root:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Visual Catalog → Surfaces → Sheet provides share, export, and folder-selection examples with editable illustrative radius. Desktop tests validate the QML behavior; actual device measurements and native physical-screen appearance need the consumer application's device integration.

To save the five rendered catalog examples, run `build/tests/LVRSTests_sheet gallery_loads` with `LVRS_SHEET_CAPTURE_DIR` set to a path below `build/`. Native macOS verification uses `QT_QPA_PLATFORM=cocoa` and `QSG_RHI_BACKEND=metal`; keep `DYLD_LIBRARY_PATH`, `QML_IMPORT_PATH` and `QML2_IMPORT_PATH` pointed at the current `build/` to avoid loading an older installed SDK.

## Shared motion

Mobile slides from the bottom and rebounds to its detent; desktop gently scales in. Dragging remains direct. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.
