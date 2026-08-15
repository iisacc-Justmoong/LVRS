# InputField

Location: `qml/components/control/input/InputField.qml`

`InputField` is a single-line text input built on `AbstractInputBar`.

## Purpose

- Provide compact text entry with optional search affordance.
- Support both the standard filled frame and the Figma `Inline` plain variant.
- Include built-in clear button behavior with focus restore.

## Core API

Mode:

- `search`
- `defaultMode`, `searchMode`, `mode` (legacy compatibility path)
- `searchIconVisible`
- `clearButtonVisible`
- `showClearButton` (readonly)

Style:

- `filledStyle`, `inlineStyle`
- `style`

Search/clear visuals:

- `searchIconSize`, `searchIconSource`
- `clearIconSize`, `clearIconMarkLength`, `clearIconMarkThickness` (readonly proportional geometry)
- `clearIconBackgroundColor`, `clearIconBackgroundColorHover`, `clearIconBackgroundColorPressed`, `clearIconBackgroundColorDisabled`
- `clearIconForegroundColor`

Inherited text/input API (from `AbstractInputBar`):

- `text`, `placeholderText`, `readOnly`, `validator`, `maximumLength`, `inputMethodHints`, `echoMode`, `renderType`, `preferNativeGestures`, `preferNativeTextInteraction`, `accepted()`, `textEdited(...)`

## Behavior Contract

- `search: true` enables the leading search affordance; legacy `mode: searchMode` still resolves to the same visual state.
- Search icon uses the shipped `generalsearch.svg` asset and is shown when `searchIconVisible == true`.
- Search and clear icon frames follow the common `Theme.iconSm` contract: `18 x 18` on desktop and `23 x 23` on mobile targets.
- The clear icon's X mark retains the former `8 / 14` length and `1.4 / 14` thickness ratios as the frame grows.
- Search icon rendering uses a supersampled `Image.sourceSize` from `RenderQuality` and the active device pixel ratio instead of custom canvas painting.
- Clear button appears only when `clearButtonVisible && enabled && !readOnly && text.length > 0`.
- Clear click sets `text = ""` and calls `forceInputFocus()`.
- Mobile-target defaults now follow `Theme.mobileTarget`; on iOS-target runs the underlying `TextInput` uses `NativeRendering` so software-keyboard repeat delete and keyboard-driven edit gestures stay on the platform-native path.
- The input surface does not install a full-cover `MouseArea`; pointer, IME, selection, and keyboard gestures are handled by the underlying `TextInput`.
- `style == inlineStyle` keeps spacing and affordances intact, but forces the field background fill to stay transparent across default, hover, pressed, focused, and disabled states.

## Usage

```qml
import LVRS 1.0 as LV

LV.InputField {
    style: inlineStyle
    search: true
    placeholderText: "Filter"
}
```
