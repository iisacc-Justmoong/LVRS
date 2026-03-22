# TextEditor

Location: `qml/components/control/input/TextEditor.qml`

`TextEditor` is a multi-line editor with optional rendered preview pane.

## Purpose

- Provide stable multi-line editing with fixed edit viewport.
- Expose rendered output (`TextMarkup`) for preview-driven flows.

## Core API

Mode and compatibility:

- constants: `plainTextMode`, `markdownMode`, `richTextMode`
- `mode`, `enforceModeDefaults`
- `wrapMode`, `textFormat`
- resolved: `effectiveWrapMode`, `effectiveTextFormat`

Editor aliases:

- `editorItem` (readonly alias)
- `text`, `readOnly`, `cursorPosition`, `selectionStart`, `selectionEnd`, `selectedText`
- `contentWidth`, `contentHeight`, `lineCount`, `textDocument`, `canPaste`

Preview:

- `showRenderedOutput`
- `renderedOutput`, `renderedPlainText`, `normalizedInput` (readonly)
- `outputSpacing`, `outputMinHeight`, `previewHeight` (readonly)
- `outputBackgroundColor`, `outputTextColor`

Layout/visual:

- `fieldMinHeight`, `editorHeight`, `resolvedEditorHeight`
- `insetHorizontal`, `insetVertical`
- `shapeStyle`, `cornerRadius`
- `showScrollBar`, `autoFocusOnPress`, `preferNativeGestures`, `preferNativeTextInteraction`

Signals and methods:

- `textEdited(text)`, `submitted(text)`
- `forceEditorFocus()`, `insertText(value)`, `clear()`, `select()`, `selectAll()`, `deselect()`, `cut()`, `copy()`, `paste()`, `undo()`, `redo()`, `submit()`

## Behavior Contract

- Submit shortcut: `Ctrl+Enter` or `Cmd+Enter`.
- Preview pane is shown only when `showRenderedOutput == true`.
- Uses a `ScrollView`-hosted `TextArea` for the edit surface, plus `InputMethodGuard` and a `WheelScrollGuard` for the preview pane.
- Mobile-target defaults now follow `Theme.mobileTarget`; on iOS-target runs the underlying `TextEdit` switches to `NativeRendering` so platform software-keyboard editing gestures and repeat delete stay on the native interaction path.
- Current defaults force plain text wrapping (`resolvedWrapMode`, `resolvedTextFormat`) unless component logic is changed.

## Usage

```qml
import LVRS 1.0 as LV

LV.TextEditor {
    text: "# Title"
    showRenderedOutput: true
}
```
