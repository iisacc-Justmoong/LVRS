# TextEditor

Location: `qml/components/control/input/TextEditor.qml`

`TextEditor` is the multi-line rich/plain editor with optional rendered preview output.

## Purpose

- Provide stable multi-line editing in fixed viewport height.
- Support mode-driven text normalization and preview rendering.
- Isolate nested scroll and IME behavior for robust input handling.

## API

Mode constants:

- `plainTextMode`
- `markdownMode`
- `richTextMode`

Mode/control properties:

- `mode`
- `enforceModeDefaults`
- `wrapMode`
- `textFormat`

Editing API:

- `text`, `placeholderText`, `readOnly`
- `cursorPosition`, `selectionStart`, `selectionEnd`
- `selectByMouse`, `persistentSelection`, `overwriteMode`

Layout/preview:

- `fieldMinHeight`, `editorHeight`, `resolvedEditorHeight`
- `showRenderedOutput`, `outputMinHeight`, `previewHeight`
- `showScrollBar`, `autoFocusOnPress`

Signals/methods:

- `textEdited(text)`
- `submitted(text)`
- `forceEditorFocus()`, `insertText(value)`, `clear()`, `undo()`, `redo()`, `submit()`

## Usage

```qml
import LVRS 1.0 as LV

LV.TextEditor {
    mode: markdownMode
    editorHeight: 240
    showRenderedOutput: true
    onSubmitted: save(text)
}
```

## How It Works

- Editing happens inside `Flickable` with fixed outer height; content grows internally.
- `Ctrl+Enter` or `Cmd+Enter` emits `submitted`.
- Preview text is produced through `TextMarkup.renderHtml(text)`.
- `InputMethodGuard` and `WheelScrollGuard` are embedded for composition safety and scroll isolation.

## Advanced Example: Submission Shortcut Workflow

```qml
import LVRS 1.0 as LV

LV.TextEditor {
    mode: plainTextMode
    onSubmitted: function(text) {
        console.log("submit", text.length)
    }
}
```

Submit is triggered by `Ctrl+Enter` or `Cmd+Enter`.

## Practical Notes

- Keep preview enabled only when users need immediate rendered feedback.
- For large documents, monitor scroll behavior and ensure parent containers do not also capture wheel events.

## Recipe: Markdown Preview Toggle

```qml
import LVRS 1.0 as LV

Item {
    property bool previewOn: true

    LV.TextEditor {
        mode: markdownMode
        showRenderedOutput: previewOn
    }
}
```

## Failure Pattern

Large markdown previews can increase layout work.
For very large documents, provide manual preview toggle or lazy preview update policy.
