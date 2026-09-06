# Alert

Location: `qml/components/surfaces/Alert.qml`

`Alert` is a glass overlay with one, two, or three actions. The visual contract
comes from [Figma Alert 106:283](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=106-283),
updated on 2026-09-06.

## API

- State/content: `open`, `title`, `message`, `buttonCount` (`0=auto`, `2`, `3`),
  `primaryText`, `secondaryText`, `tertiaryText`.
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
selects one. Actions emit signals; the caller controls closing.

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
Disabled actions do not emit click signals.

## Usage

```qml
import LVRS 1.0 as LV

LV.Alert {
    open: true
    buttonCount: 3
    title: "Save changes?"
    message: "You have unsaved changes.\nSave them before closing?"
    primaryText: "Save changes"
    secondaryText: "Discard changes"
    tertiaryText: "Cancel"
    onPrimaryClicked: { /* save, then close */ }
    onSecondaryClicked: { /* discard, then close */ }
}
```

## Validation

`LVRSTests_import_api` checks the Figma geometry, Title/Body tokens, action
ordering, non-capsule corners, icon exports, width containment, icon visibility,
long-copy growth, isolated button defaults, actual backdrop blur, red Discard
pixels, click signals, modal input blocking, and capture lifecycle.

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
