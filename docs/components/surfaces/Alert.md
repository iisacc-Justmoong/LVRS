# Alert

Location: `qml/components/surfaces/Alert.qml`

`Alert` is a glass overlay with one, two, or three actions. The visual contract
comes from [Figma Alert 106:283](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=106-283),
updated on 2026-09-06.

## API

- State/content: `open`, `imageSource`, `title`, `description`,
  `buttonCount` (`0=auto`, `2`, `3`).
- Action arguments: `button1Text`, `button1Method`, `button2Text`, `button2Method`,
  `button3Text`, `button3Method`.
- Enabled states: `primaryEnabled`, `secondaryEnabled`, `tertiaryEnabled`.
- Behavior: `dismissOnBackground`, `useOverlayLayer`, `secondaryDestructive`.
- Size/shape: `minWidth`, `maxWidth`, `preferredWidth`, `cardCornerRadius`,
  `shapeStyle`, `resolvedCardCornerRadius`.
- Material: `backdropColor`, `cardBackgroundColor`, `glassEnabled`,
  `glassBlurRadius`, `backdropSource`, read-only `resolvedBackdropSource` and `glassActive`.
- Icon: `showIcon`, `appIconSource`, `appIconSize`, `iconFrameSize`,
  `appIconBackgroundColor`, `appIconFrameColor`, `appIconInnerColor`.
- Signals: `primaryClicked()`, `secondaryClicked()`, `tertiaryClicked()`, `dismissed()`.

Existing action signals and auto-count behavior remain compatible: tertiary text
selects three actions, secondary text selects two, and no secondary/tertiary text
selects one. Actions run the supplied method and emit the existing signal; the
caller controls closing.

### Content and action arguments

| Argument | Type | Existing property / action |
| --- | --- | --- |
| `imageSource` | URL | Alias of `appIconSource`; rendered in the central image frame |
| `title` | string | Centered title |
| `description` | string | Alias of `message`; centered description |
| `button1Text` | string | Alias of `primaryText` |
| `button1Method` | callable, default `null` | Primary action; `primaryClicked()` still fires |
| `button2Text` | string | Alias of `secondaryText` |
| `button2Method` | callable, default `null` | Secondary action; `secondaryClicked()` still fires |
| `button3Text` | string | Alias of `tertiaryText` |
| `button3Method` | callable, default `null` | Tertiary action; `tertiaryClicked()` still fires |

Aliases share the same value with the existing properties, including updates
after creation. Button numbers identify their action roles: button 1 is the
primary action on the right in the two-button layout and at the top in the
three-button layout. Button 2 is secondary; button 3 is the final/cancel action.
Set `button3Text: ""` for two actions and also `button2Text: ""` for one action
when `buttonCount` is automatic. Supplying a method does not change the count.

Each method is passed directly to the existing `AlertButton.method` API. It
accepts a JavaScript function or an object exposing `invoke(eventData)` or
`trigger(eventData)`, using the shared `ButtonMethodRegistry`. `eventData.source`
is the clicked button, `eventData.trigger` is `"clicked"`, and the enabled-state
and tone fields follow [AbstractButton](../control/AbstractButton.md).
Methods may omit the argument. Wrap an application method when it needs its
own receiver, for example `function() { documentController.save() }`.
Methods can be replaced or cleared with `null` after creation. An omitted method
leaves the signal-only API available. Keep each operation in either its method
or its signal handler to avoid running the same application operation twice.

## Figma layout and typography

At desktop scale, the two/three-action references are 500 × 417 / 500 × 517.
The card shrinks to the available host width, including hosts narrower than
`minWidth`. It defaults to a 36px rounded rectangle. The existing opt-in
`shapeCylinder` remains available for compatibility; it is not used by default.

The content has 46px top and 36px bottom padding. An 86px icon frame with a 28px
corner radius precedes the copy by 28px. The exact Figma SVG exports are bundled:
64px adjustments for two actions, 56px file-text for three. `showIcon: false`
removes the frame and its 28px gap. An explicit custom icon URL still works;
an empty URL retains the configurable fallback.

Typography uses the existing `Label.title` (26px Bold) and `Label.body`
(13px Medium) tokens and the framework's bundled Pretendard fonts. No new text
style is introduced. The measured Figma title/message boxes reserve at least
34px/56px with a 14px gap and centered text. Extra lines grow these boxes.

- Two actions: secondary/cancel on the left, primary on the right; 14px gap,
  24px sides, 28px above and 32px below the buttons.
- Three actions: primary, secondary/discard, cancel; 12px gaps, 24px sides,
  18px above and below.
- Main buttons: 56px height and 16px radius. Final cancel: 44px, transparent,
  without an outline. Secondary buttons are transparent with a 1px outline.
- `secondaryDestructive` defaults to true for three actions, applying the
  existing `Theme.danger` red to Discard/No text. Override it for a neutral
  secondary action; two-action Cancel remains neutral by default.
- Alert-specific presentation is enabled through `AlertButton.dialogStyle`.
  Standalone `AlertButton` and `Modal` retain their compact baseline.

## Glass material and rendering

Alert uses the Figma color tokens under `Theme.alert*`: #1D1F21 at 72% opacity,
a 20% white edge, #F4F5F7 titles/actions, #D6D9DF descriptions, #027DFF primary
buttons, #596168 outlines, #363B3F dividers, and the blue icon frame.

The source window is captured with Qt's
[ShaderEffectSource](https://doc.qt.io/qt-6/qml-qtquick-shadereffectsource.html)
and blurred through
[MultiEffect](https://doc.qt.io/qt-6/qml-qtquick-effects-multieffect.html).
The rounded mask, 28px frost, translucent tint, and 32px soft shadow are separate
from the sharp content. These are existing Qt Quick effects already used by
LVRS, so no additional package dependency is introduced.

`ApplicationWindow.contentItem` is discovered automatically when the Alert
moves to `Controls.Overlay`. A custom/plain window can set `backdropSource`
to a sibling background item. Ancestors containing the Alert are rejected to
prevent recursive captures. When a safe source is unavailable, or glass is
disabled, the translucent tint still renders. Closing disconnects the capture
and stops its live updates.

Qt's blur and edge treatment approximate the Figma material; Figma's proprietary
refraction/light simulation is not reproduced. Shader effects require a Qt RHI
graphics backend; software rendering retains the tint but cannot supply frost.

## Interaction

Background input is consumed while open. A backdrop click dismisses only when
`dismissOnBackground` is true; clicking blank card space never dismisses.
Disabled actions do not invoke methods or emit click signals.

## Usage

```qml
import LVRS 1.0 as LV

LV.Alert {
    id: saveAlert
    open: true
    buttonCount: 3
    imageSource: "qrc:/qt/qml/LVRS/resources/images/alert-file-text.svg"
    title: "Save changes?"
    description: "You have unsaved changes.\nSave them before closing?"
    button1Text: "Save changes"
    button1Method: function() {
        documentController.save()
        saveAlert.open = false
    }
    button2Text: "Discard changes"
    button2Method: function() {
        documentController.discard()
        saveAlert.open = false
    }
    button3Text: "Cancel"
    button3Method: function() { saveAlert.open = false }
}
```

`documentController` is supplied by the application. The image URL can point to
a Qt resource, a local file, or a remote image supported by Qt Quick `Image`.

## Validation

`LVRSTests_import_api` checks the Figma geometry, Title/Body tokens, action
ordering, non-capsule corners, icon exports, width containment, icon visibility,
long-copy growth, isolated button defaults, actual backdrop blur, red Discard
pixels, click signals, modal input blocking, and capture lifecycle. It also
checks content arguments and live aliases, image loading, all six rendered
action positions across one/two/three-button layouts, function/command methods,
method replacement, disabled actions, and compatibility with signal handlers.

Run with a supported graphics backend:

```sh
cmake -S . -B build -DLVRS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

CTest uses the offscreen platform, which may choose software rendering. In that
case the pixel test validates the translucent fallback. Run the focused tests on
the native platform as well to verify actual GPU frost. For a macOS build tree:

```sh
DYLD_LIBRARY_PATH="$PWD/build" QT_QPA_PLATFORM=cocoa QSG_RHI_BACKEND=metal \
  QT_QUICK_CONTROLS_STYLE=Basic \
  LVRS_ALERT_CAPTURE_DIR="$PWD/build/alert-preview" \
  ./build/tests/LVRSTests_import_api alert_figma_variant_contract_loads \
  alert_action_button_padding_scopes_to_alert alert_glass_overlay_and_input_contract
```

The explicit library path prevents an older installed LVRS from shadowing the
library being validated. Other platforms should use their native RHI backend.

Set `LVRS_ALERT_CAPTURE_DIR` to a folder under `build/` when running
`alert_glass_overlay_and_input_contract` to save rendered reference images.
Icon notices are bundled in `resources/images/alert-icons-LICENSE.txt`.
