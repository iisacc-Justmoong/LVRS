# CodeEditor

Location: `qml/components/control/input/CodeEditor.qml`

`CodeEditor` is a code-oriented editor (`TextEdit.NoWrap`, `TextEdit.PlainText`) with optional snippet header.

## Purpose

- Keep monospaced code editing deterministic.
- Provide snippet metadata header (`title/language`).
- Expose low-level `TextEdit` API through aliases.

## Core API

Primary aliases:

- `editorItem` (readonly alias)
- `text`, `readOnly`, `cursorPosition`, `selectionStart`, `selectionEnd`, `selectedText`
- `contentWidth`, `contentHeight`, `lineCount`, `textDocument`, `canPaste`

Code-specific:

- readonly `wrapMode` (`NoWrap`)
- readonly `textFormat` (`PlainText`)
- `snippetTitle`, `snippetLanguage`, `showSnippetHeader`

Layout/visual:

- `fieldMinHeight`, `editorHeight`, `resolvedEditorHeight`
- `headerHeight`, `headerSpacing`, `topInset`
- `insetHorizontal`, `insetVertical`
- `shapeStyle`, `cornerRadius`
- `showScrollBar`, `autoFocusOnPress`, `preferNativeGestures`

Signals and methods:

- `textEdited(text)`, `submitted(text)`
- `forceEditorFocus()`, `insertText(value)`, `clear()`, `select()`, `selectAll()`, `deselect()`, `cut()`, `copy()`, `paste()`, `undo()`, `redo()`, `submit()`

## Behavior Contract

- Submit shortcut: `Ctrl+Enter` or `Cmd+Enter`.
- Header area height is included in top inset only when `showSnippetHeader` is true.
- Includes `InputMethodGuard` + `WheelScrollGuard` for IME/scroll safety.

## Usage

```qml
import LVRS 1.0 as LV

LV.CodeEditor {
    snippetTitle: "main.cpp"
    snippetLanguage: "C++"
    text: "int main() { return 0; }"
}
```
