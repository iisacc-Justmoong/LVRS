# InputField

Location: `qml/components/control/input/InputField.qml`

`InputField` is LVRS's public single-line `TextInput` wrapper. It implements the Figma `TextField` component at node `114:179` without replacing Qt's native editing, IME, selection, clipboard, or accessibility behavior.

## Purpose

- Provide stable one-line text entry in `Rounded`, `Cylinder`, and `Inline` styles.
- Cover the Figma default, disabled, cursor-active, selected, active, and search states with native `TextInput` state.
- Keep field geometry at `@2x` on mobile while Body typography stays fixed at `13px / 13px`.
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
| Horizontal inset | `7` | `14` |
| Vertical spacing token | `3` | `6` |
| Search/clear frame | `12 × 12` | `24 × 24` |
| Rounded radius | `5` | `10` |
| Search/text/clear gap | `2` | `4` |
| Body font and line box | `13 / 13` | `13 / 13` |

Figma's active, selected, and search instances report `205px` while default and disabled report `206px`; that difference comes from variant-local auto-layout content. The runtime field deliberately stays `206px` so typing, selecting, searching, and showing the clear button never shift surrounding layout by one pixel.

On mobile, the frame, insets, radii, and icon frames scale to `@2x`. Body is the explicit exception and remains `13px / 13px`; the line box is vertically centered in the `38px` frame.

## Behavior Contract

- `style: roundedStyle` uses `panelBackground10` with a `5px` desktop radius.
- `style: cylinderStyle` uses the same fill with a half-height radius.
- `style: inlineStyle` uses a transparent, cylinder-clipped frame while preserving the same insets and affordances.
- Disabled text uses `Theme.disabledColor`; default/active text uses `Theme.titleHeaderColor`.
- Selection uses `Theme.accent` and selected text remains `Theme.titleHeaderColor`.
- Search uses the component-specific `inputFieldSearch.svg`, measured from the Figma `12 × 12` vector instead of stretching the generic `16 × 16` icon.
- Clear is visible only when `clearButtonVisible && enabled && !readOnly && text.length > 0`; clicking it empties the field and restores input focus.
- Leading and trailing intrinsic widths remain part of the inset contract even while the field or an ancestor is hidden, preventing delayed geometry shifts when a view becomes visible.
- On iOS targets, the underlying `TextInput` still uses `NativeRendering` and retains platform-native keyboard, IME, selection, and pointer behavior.

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
LV.InputField {
    style: inlineStyle
    text: "Editable value"
}
```
