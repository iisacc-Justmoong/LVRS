# CodeEditor

Location: `qml/components/control/input/CodeEditor.qml`

`CodeEditor` is a snippet-oriented editor specialized for code input (`PlainText + NoWrap`).

## Purpose

- Keep code entry deterministic with monospaced defaults.
- Provide optional snippet header metadata.
- Preserve stable component height under large content.

## API

Core editing:

- `text`, `placeholderText`, `readOnly`
- `cursorPosition`, `selectionStart`, `selectionEnd`
- `selectByMouse`, `persistentSelection`, `overwriteMode`

Snippet header:

- `snippetTitle`
- `snippetLanguage`
- `showSnippetHeader`

Layout:

- `fieldMinHeight`, `editorHeight`, `resolvedEditorHeight`
- `headerHeight`, `headerSpacing`, `topInset`
- `showScrollBar`, `autoFocusOnPress`

Signals/methods:

- `textEdited(text)`
- `submitted(text)`
- `forceEditorFocus()`, `insertText(value)`, `clear()`, `undo()`, `redo()`, `submit()`

## Usage

```qml
import LVRS 1.0 as LV

LV.CodeEditor {
    snippetTitle: "main.cpp"
    snippetLanguage: "C++"
    text: "int main() { return 0; }"
}
```

## How It Works

- Uses `TextEdit.NoWrap` + plain text format by contract.
- Monospace font default is platform-aware (`Menlo` on macOS, `Monospace` otherwise).
- Embedded `InputMethodGuard` and `WheelScrollGuard` provide text integrity and nested scroll safety.

## Advanced Example: Read-Only Snippet View

```qml
import LVRS 1.0 as LV

LV.CodeEditor {
    snippetTitle: "Build Command"
    snippetLanguage: "bash"
    readOnly: true
    text: "cmake -S . -B build && cmake --build build"
}
```

## Troubleshooting

If horizontal scrolling feels inconsistent, verify parent scroll containers are guarded by `WheelScrollGuard`.

## Recipe: Controlled Submit with Validation

```qml
import LVRS 1.0 as LV

LV.CodeEditor {
    id: editor
    snippetTitle: "policy.json"

    onSubmitted: function(text) {
        try {
            JSON.parse(text)
            savePolicy(text)
        } catch (e) {
            console.warn("invalid json")
        }
    }
}
```

## Operational Tip

For auditability, pair `CodeEditor` submission with explicit version metadata in surrounding view model.
