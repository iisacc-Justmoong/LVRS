# InputField

Location: `qml/components/control/input/InputField.qml`

`InputField` is LVRS's public single-line `TextInput` wrapper. It implements the Figma `TextField` component at node `114:179` without replacing Qt's native editing, IME, selection, clipboard, or accessibility behavior.

## Purpose

- Provide stable one-line text entry in `Rounded`, `Cylinder`, and `Inline` styles.
- Cover the Figma default, disabled, cursor-active, selected, active, and search states with native `TextInput` state.
- Use the same field geometry and Body `13px / 13px` typography on desktop and mobile.
- Include a measured search affordance and integrated clear action.

## Core API

Mode:

- `defaultMode`, `searchMode`, `mode`
- `search` (convenience binding for `mode == searchMode`)
- `searchIconVisible`
- `clearButtonVisible`, `showClearButton` (readonly)

Style:

- `roundedStyle`, `cylinderStyle`, `inlineStyle`
- `filledStyle` (compatibility alias of `roundedStyle`)
- `style`, `resolvedStyle` (readonly)

Material:

- `glassEnabled` (default `true`), `glassBlurRadius` (desktop `8`, Inline `6`)
- `backdropSource`: defaults to the enclosing Controls/LVRS ApplicationWindow's background
- `resolvedBackdropSource`, `glassActive` (readonly)
- Inherited `backgroundColor` and state variants still override the material tint.
- Inherited `backgroundComponent` replaces decoration without changing input or slot geometry.

Search/clear visuals:

- `searchIconSize`, `searchIconSource`
- `clearIconSize`, `clearIconMarkLength`, `clearIconMarkThickness`
- `clearIconBackgroundColor`, hover/pressed/disabled variants
- `clearIconForegroundColor`

Inherited text/input API from `AbstractInputBar` includes `text`, `placeholderText`, `readOnly`, `validator`, `maximumLength`, `inputMethodHints`, `echoMode`, `renderType`, `inputItem`, selection/cursor aliases, `accepted(text)`, and `textEdited(text)`.

## Figma Geometry Contract

| Metric | Desktop | Mobile |
| --- | ---: | ---: |
| Stable field frame | `206 × 19` | `412 × 38` |
| Left inset; right inset without clear button | `7` | `14` |
| Clear button right inset | `8` | `8` |
| Vertical spacing token | `3` | `6` |
| Search/clear frame | `12 × 12` | `24 × 24` |
| Rounded radius | `5` | `10` |
| Search/text/clear gap | `2` | `4` |
| Body font and line box | `13 / 13` | `13 / 13` |

Figma's active, selected, and search instances report `205px` while default and disabled report `206px`; that difference comes from variant-local auto-layout content. The runtime field deliberately stays `206px` so typing, selecting, searching, and showing the clear button never shift surrounding layout by one pixel.

On mobile, the frame, general insets, radii, and icon frames match desktop. Body remains `13px / 13px`, and the clear button's right inset remains `8` logical pixels; the line box is vertically centered in the `19px` frame.

The clear-button inset is the explicit 2026-09-06 LVRS adjustment to the Figma reference. When the clear button is visible, its right edge remains exactly `8` logical pixels from the field's right edge, including when the width or `insetHorizontal` changes. `AbstractInputBar.insetRight` defaults to `insetHorizontal`; `InputField` binds it to `8` while showing the clear button. Both the trailing anchor and text reservation use that right inset, preserving the text-to-button gap and the left/search layout.

## Behavior Contract

- `style: roundedStyle` uses the recessed glass material with a `5px` desktop radius.
- `style: cylinderStyle` uses the same material with a half-height radius.
- `style: inlineStyle` uses a lighter translucent material and shallower inset lighting with the same insets and affordances.
- Disabled text uses `Theme.disabledColor`; default/active text uses `Theme.titleHeaderColor`.
- Selection uses `Theme.accent` and selected text remains `Theme.titleHeaderColor`.
- Search uses the component-specific `inputFieldSearch.svg`, measured from the Figma `12 × 12` vector instead of stretching the generic `16 × 16` icon.
- Clear is visible only when `clearButtonVisible && enabled && !readOnly && text.length > 0`; clicking it empties the field and restores input focus.
- Leading and trailing intrinsic widths remain part of the inset contract even while the field or an ancestor is hidden, preventing delayed geometry shifts when a view becomes visible.
- On iOS targets, the underlying `TextInput` still uses `NativeRendering` and retains platform-native keyboard, IME, selection, and pointer behavior.

`LVRSTests_import_api::input_field_figma_contract_loads` checks the actual clear-button bounds across all three styles and search mode on desktop/mobile, custom field width and horizontal inset, text visibility changes, and click-to-clear with focus restoration.

## TextField Material

The 2026-09-06 [Figma TextField](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=114-179)
adapts visionOS material and inset lighting to LVRS's existing compact dimensions.
All enabled interaction states share the material; hover and focus do not substitute opaque fills.
The disabled state removes blur, inner shadows, and the border.

| Desktop material value | Rounded / Cylinder | Disabled Rounded / Cylinder | Inline |
| --- | --- | --- | --- |
| `panelBackground10` tint alpha | 64% | 36% | 16% |
| White reflection alpha | 1.8% | 1.8% | 1.8% |
| Vertical gradient: top black / 52% transparent / bottom white | 20% / 0% / 3.5% | 4.5% / 0% / 1.5% | 6% / 0% / 1.5% |
| Backdrop blur radius | 8 | Off | 6, off when disabled |
| Black inner shadow: x / y / blur / alpha | 0.5 / 1.2 / 2 / 30% | Off | 0 / 0.65 / 1 / 12% |
| White inner shadow: x / y / blur / alpha | 0 / -0.5 / 0.8 / 16% | Off | 0 / -0.5 / 0.6 / 5.5% |
| Inside edge | 0.5 | None | None |

The inside edge uses a vertical black 28% → white 1.5% → white 17% gradient
at 0%, 50%, and 100%. Effect distances and color alpha use the same values on
desktop and mobile. Inline keeps its fill when disabled but removes its inner shadows.
The Figma paint stack is drawn from the concave gradient through reflection to tint,
followed by inner lighting and the edge.

`InputField.qml` keeps decoration in a private inline `MaterialSurface` component. It uses the existing
[Qt Quick MultiEffect](https://doc.qt.io/qt-6.8/qml-qtquick-effects-multieffect.html)
and `ShaderEffectSource` approach used by Alert; no new package dependency is added.
The capture includes blur padding, is masked to the field, and stays live while
visible and enabled. Repositioning or scrolling the field updates the capture region.
Text, selection, search, and clear controls are outside the effect.

The source must contain only the background to be sampled. A separate panel or image
behind the field can be assigned to `backdropSource`; the field, its descendants, and
ancestors containing it are rejected to avoid capture feedback. A plain `Window`
has no default source. Without a source, with `glassEnabled: false`, or on Qt's
software renderer, tint, gradient, inner lighting, and the inside edge remain visible;
actual background blur requires a Qt RHI renderer.

Subpixel inner lighting and the edge use a cached, device-pixel-ratio-aware
[Qt Canvas](https://doc.qt.io/qt-6.8/qml-qtquick-context2d.html). Only appearance or
geometry changes repaint it; typing does not rerasterize the decoration.
`input_field_material_rendering_contract` renders all 18 style/state combinations,
checks inset contrast and the absence of exterior paint, verifies actual blur against
a detailed backdrop, and covers live source changes, moved ancestors, unsafe sources,
hidden/disabled capture, tint overrides, native typing, selection, and clear focus.

## Usage

```qml
import LVRS 1.0 as LV

LV.InputField {
    style: cylinderStyle
    search: true
    placeholderText: "Filter"
}
```

```qml
Item {
    Rectangle {
        id: panelBackdrop
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#31425b" }
            GradientStop { position: 1; color: "#252629" }
        }
    }
    LV.InputField {
        backdropSource: panelBackdrop
        placeholderText: "Filter"
    }
}
```

```qml
LV.InputField {
    style: inlineStyle
    text: "Editable value"
}
```
