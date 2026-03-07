# InputField

Location: `qml/components/control/input/InputField.qml`

`InputField` is a single-line text input built on `AbstractInputBar`.

## Purpose

- Provide compact text entry with optional search affordance.
- Support both the standard filled frame and the Figma `Inline` plain variant.
- Include built-in clear button behavior with focus restore.

## Core API

Mode:

- `defaultMode`, `searchMode`
- `mode`
- `searchIconVisible`
- `clearButtonVisible`
- `showClearButton` (readonly)

Style:

- `filledStyle`, `inlineStyle`
- `style`

Search/clear visuals:

- `searchIconColor`, `searchIconStrokeWidth`
- `clearIconBackgroundColor`, `clearIconBackgroundColorHover`, `clearIconBackgroundColorPressed`, `clearIconBackgroundColorDisabled`
- `clearIconForegroundColor`

Inherited text/input API (from `AbstractInputBar`):

- `text`, `placeholderText`, `readOnly`, `validator`, `maximumLength`, `inputMethodHints`, `echoMode`, `accepted()`, `textEdited(...)`

## Behavior Contract

- Search icon is drawn by `Canvas` and shown when `searchIconVisible == true`.
- Clear button appears only when `clearButtonVisible && enabled && !readOnly && text.length > 0`.
- Clear click sets `text = ""` and calls `forceInputFocus()`.
- `style == inlineStyle` keeps spacing and affordances intact, but forces the field background fill to stay transparent across default, hover, pressed, focused, and disabled states.

## Usage

```qml
import LVRS 1.0 as LV

LV.InputField {
    style: inlineStyle
    mode: searchMode
    placeholderText: "Filter"
}
```
