# ColorPicker

Location: `qml/components/control/input/ColorPicker.qml`

`LV.ColorPicker` is an embeddable color editing view. Its parent owns the panel, modal or popover surface, positioning, opening and dismissal. It has no title bar, background, shadow, overlay or automatic reparenting. The reference is [Figma ColorPickerPanel, 885:44522](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=885-44522).

## Variants and layout

| `type` | Content | Default size |
| --- | --- | --- |
| `LV.ColorPicker.Wheel` | H/S/B channels, hue ring and saturation/brightness triangle | 320 × 461 |
| `LV.ColorPicker.HueSaturation` | Hue/saturation plane, brightness rail and B channel | 320 × 437 |
| `LV.ColorPicker.SaturationBrightness` | Saturation/brightness plane, hue rail and H channel | 320 × 437 |
| `LV.ColorPicker.Grayscale` | K channel and spectrum | 320 × 361 |
| `LV.ColorPicker.RGB` | R/G/B channels and spectrum | 320 × 413 |
| `LV.ColorPicker.CMYK` | C/M/Y/K channels and spectrum | 320 × 439 |

The transparent content uses 12px padding/gaps, LVRS Body 13 and Caption 11, 22px `LV.InputField` and Mini `LV.Slider`, a 28px comparison/Hex/alpha row, 20px recent swatches, and 22px actions. Widths of 320px or greater are supported; square planes grow with width. `padding` can be changed when a host already provides an inset. Hiding recent colors reduces height by 32px; hiding actions reduces it by another 34px.

## API

| Property / signal / method | Contract |
| --- | --- |
| `type` | One of the six enums; changing it preserves color state. |
| `currentColor` | Editable `color` property, default `#7A5AF8`. Native edits preserve an external QML binding. |
| `previousColor` | Cancel baseline and left half of the comparison swatch. Defaults to the initial externally supplied color. Pass it explicitly when supporting live document preview. |
| `hex` | Read-only uppercase RRGGBB text. |
| `recentColors` | Caller-owned color array; the first eight entries are shown. Default is the eight Figma swatches. No global history is persisted. |
| `showRecentColors`, `showActions` | Both default to `true`. |
| `applyText`, `cancelText` | Translatable action text. |
| `colorEdited(color)` | User edit, including selecting a recent color or restoring on Cancel. External property changes do not emit it. |
| `accepted(color)`, `canceled()` | Host handles persistence/dismissal. Neither signal closes a view. |
| `eyedropperRequested()` | Host starts document/screen sampling and supplies the result with `setColor(color)`. |
| `setColor(color)` | Edit color through the same state model; ignores invalid colors. |
| `setHex(text)` | Returns success. Accepts optional `#`, six-digit RRGGBB, or eight-digit RRGGBBAA. Six-digit input preserves alpha. |
| `accept()` | Commits pending input, updates the comparison baseline and emits `accepted`. |
| `cancel()` | Restores the comparison baseline and emits `canceled`. Escape calls this method. |

Edited numeric fields commit on Enter or focus loss. Merely focusing a rounded display value does not quantize the underlying color. Values are clamped to H 0–360, RGB 0–255 and other channels 0–100; invalid/non-finite input is discarded. Alpha survives edits to the color channels. Selecting a recent color uses that swatch's alpha.

HSV edits retain the chosen hue at zero saturation and retain saturation at zero brightness. CMYK edits retain all four entered channels, including nonzero K, until editing in another model. Conversion uses Qt's sRGB/CMYK arithmetic; it is not an ICC print-profile conversion. The footer therefore identifies the working space as sRGB. The Grayscale K channel uses `1 − qGray(rgb)/255` and editing it produces an achromatic color. Switching to Grayscale alone does not discard the current color.

The spectrum contains hue horizontally and white → saturated color → black vertically. For colors outside that two-dimensional domain, its cursor projects to the closest tint/shade branch in saturation/brightness; the precise channels, Hex and comparison swatch still show the actual color.

## Pass the view to a host

```qml
import QtQuick
import LVRS as LV

Item {
    id: editor
    property color documentColor: "#7A5AF8"
    property color originalColor: "#7A5AF8"

    Component {
        id: colorView
        LV.ColorPicker {
            type: LV.ColorPicker.SaturationBrightness
            currentColor: editor.documentColor
            previousColor: editor.originalColor
            onColorEdited: color => editor.documentColor = color
            onAccepted: color => {
                editor.originalColor = color
                dialog.open = false
            }
            onCanceled: dialog.open = false
        }
    }

    LV.Modal {
        id: dialog
        minWidth: 360
        maxWidth: 360
        frameMinHeight: 0
        showIcon: false
        primaryText: ""
        contentComponent: colorView
        onCanceled: editor.documentColor = editor.originalColor
    }

    LV.LabelButton {
        text: "Edit color"
        onClicked: {
            editor.originalColor = editor.documentColor
            dialog.open = true
        }
    }
}
```

The same `Component` can be supplied to a `Loader` in another panel/popover. A direct `LV.ColorPicker` child is also supported. Hosts destroy the view through their usual lifecycle; the picker never moves itself to another parent. Separate instances have independent editor state. When the host dismisses its surface directly (for example, a Modal backdrop click), it restores the saved document color itself: the content has already been unloaded by the time `Modal.canceled` is emitted.

## Input and rendering

Color domains support pointer and single-finger dragging. Tab focuses the domains, sliders, fields, swatches and actions. Arrow keys adjust the focused domain: hue/saturation for the HS plane, saturation/brightness for the SB plane, and hue/brightness for the wheel (Shift+Up/Down adjusts wheel saturation). Shift accelerates horizontal and rail changes. Existing LVRS sliders retain their native keyboard behavior.

The hue ring is the exported Figma asset. Interactive triangles and planes use Qt `QColor` and cached `QQuickPaintedItem` color domains; no extra rendering dependency is introduced. `ColorPickerModel` and `ColorPickerSurface` are implementation helpers, not separate catalog components.

The QML module declares `DEPENDENCIES QtQuick` so compiler/lint tooling can resolve native item inheritance and QColor properties, as required by [Qt's module dependency contract](https://doc.qt.io/qt-6/qt-add-qml-module.html).

Validation: `LVRSTests_colorpicker` covers conversion, edits, geometry, host lifecycle, bindings, input and rendering. `LVRSTests_examples` checks the registered gallery. Run `ctest --test-dir build --output-on-failure`; set `LVRS_COLORPICKER_CAPTURE_DIR` when running the focused test directly to save reference captures.
