# TextEditor

Location: `qml/components/control/input/TextEditor.qml`

`TextEditor` is a file-connected rich text editor. It uses a native Qt Quick `TextEdit` surface fixed to `TextEdit.RichText`, so the editing behavior is closer to Mac TextEdit-style rich document editing than to a code or plain-text buffer.

## Purpose

- Bind an editable rich text surface directly to a filesystem path.
- Load UTF-8 HTML/rich text through the internal C++ document engine and synchronize edits through atomic write-through semantics.
- Use the native `TextEdit` editor for rich text rendering, wrapping, IME, selection, cursor movement, clipboard commands, and platform text gestures.
- Native Return/Enter handling leaves the caret on the inserted line; wrappers should not replace the document in a way that clamps the cursor back to the previous line.
- Keep file I/O, read progress, dirty state, and sync errors out of the visual editor.
- Leave plain text editing to `CodeEditor` or simpler input components.

## Core API

File connection:

- `filePath` (required): connected filesystem path. `LV.TextEditor` must be created with this property.
- `chunkSize`: byte budget per lazy read step. The internal model clamps very small or very large values.

Rich text surface:

- `editorItem` (readonly alias): the underlying `TextEdit` for advanced rich text integration.
- `text`: the rich text/HTML document string shown by the editor.
- `textFormat` (readonly): always `TextEdit.RichText`.
- `textDocument`: the native document object exposed by the underlying `TextEdit`.
- Selection, cursor, and clipboard compatibility aliases mirror the native `TextEdit` surface.

Read/sync state:

- `dirty`: true while local edits are waiting for filesystem synchronization; false after successful `read()` or automatic sync.
- `reading`: true while the connected file is being decoded in chunks.
- `bytesRead`, `bytesTotal`, `progress`: byte-level read progress.
- `error`: most recent read/sync error.
- `empty`: true when the editor contains no rich text characters.
- `documentRevision`: monotonically increases for editor-originated document edits. File reads do not advance it.

Methods:

- `read()`: reload `filePath`. Normal construction and `filePath` changes already read automatically.
- `forceEditorFocus()`, `insertText(value)`, `selectAll()`, `copy()`, `paste()`, `undo()`, and `redo()` forward to the native editor.

Signals:

- `readFinished(path)`
- `readFailed(path, error)`
- `readProgress(path, bytesRead, bytesTotal)`
- `syncFinished(path)`
- `syncFailed(path, error)`
- `textEdited(text)`
- `documentEdited(documentText, documentRevision)`

## API Usage Manual

### Import

`TextEditor` is exported by the LVRS QML module:

```qml
import LVRS 1.0 as LV
```

### Create an Editor

`filePath` is required. The component is invalid without it because the editor is defined as a direct filesystem editor, not as a detached text buffer.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
}
```

### Rich Text Editing

The edit surface is always `TextEdit.RichText`. Bind or assign HTML when programmatically setting content:

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
    text: "<h1>Notes</h1><p>Hello <b>bold</b> text.</p>"
}
```

Use `editorItem` when application code needs lower-level rich text behavior from Qt Quick `TextEdit`:

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"

    Component.onCompleted: {
        editor.editorItem.selectAll()
    }
}
```

### Read the Connected File

The editor reads the connected file automatically after construction:

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
}
```

Call `read()` only when the caller explicitly wants to reload `filePath`. It returns `true` when the file stream was opened and lazy loading was scheduled. Completion is reported through `readFinished(path)`.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"

    Component.onCompleted: {
        if (!read())
            console.warn(error)
    }

    onReadFinished: console.log("Read", path)
    onReadFailed: console.warn("Read failed", path, error)
}
```

### Realtime Output

There is no public save API. Text edits are synchronized back to `filePath` automatically. The editor emits `syncFinished(path)` after a successful write-through step and `syncFailed(path, error)` if the filesystem write fails.
For consumers that need the exact edited document in the same event turn as the native editor change, use `documentEdited(documentText, documentRevision)`. The signal carries the latest rich document payload and a monotonically increasing revision so downstream code can treat that pair as authoritative instead of rereading a binding that may settle later in the turn. While native input method composition is active, the signal and file write-through are deferred until the committed document payload is visible on the editor surface.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
    onDocumentEdited: function(documentText, documentRevision) {
        console.log("Document revision", documentRevision, documentText.length)
    }
    onSyncFinished: console.log("Synchronized", path)
    onSyncFailed: console.warn("Sync failed", path, error)
}
```

### Track State

Use the read/sync state properties for file status:

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.html"
    chunkSize: 65536

    onReadProgress: console.log(bytesRead, bytesTotal)
}

LV.ProgressBar {
    value: editor.progress
    visible: editor.reading
}

LV.Label {
    text: editor.dirty ? "Syncing" : "Synced"
}
```

### Work with Large Files

Set `chunkSize` to tune lazy loading. The default is suitable for normal rich text documents; larger chunks reduce scheduling overhead, smaller chunks keep each event-loop step shorter.

```qml
LV.TextEditor {
    filePath: "/tmp/large-notes.html"
    chunkSize: 262144
}
```

While `reading` is true, editor-originated synchronization is skipped so a partial document is not written accidentally.

### Change Files

Changing `filePath` changes the connected file and automatically reads the new path. Because edits synchronize continuously, callers normally only need to guard path changes when they are handling a previous sync failure:

```qml
function openPath(path) {
    if (editor.dirty)
        return

    editor.filePath = path
}
```

### Read-Only View

Set `readOnly` when the editor should load and display a rich text file without accepting text edits.

```qml
LV.TextEditor {
    filePath: "/tmp/report.html"
    readOnly: true
}
```

### Not Public API

Application code should not depend on the internal `TextDocumentModel`; it exists to provide file loading, progress, dirty state, and atomic synchronization. Use `editorItem` for rich text editor behavior and the read/sync properties for file state.

These names are intentionally not part of `LV.TextEditor`:

- `write()`, `loadFile(path)`, `saveFile(path)`, `reloadFile()`
- mode or preview APIs such as `mode`, `markdownMode`, `plainTextMode`, `renderedOutput`
- save-as path parameters

Layout/visual:

- `placeholderText`, `readOnly`
- `fieldMinHeight`, `editorHeight`, `resolvedEditorHeight`
- `insetHorizontal`, `insetVertical`
- `shapeStyle`, `cornerRadius`
- `showScrollBar`, `autoFocusOnPress`
- `preferNativeGestures`, `preferNativeTextInteraction`
- `viewportBoundsBehavior`, `viewportBoundsMovement`
- `viewportFlickDeceleration`, `viewportMaximumFlickVelocity`
- font and color tokens for the rich text viewport

## Behavior Contract

- The editor automatically calls `read()` after construction and after `filePath` changes.
- `read()` opens UTF-8 rich text/HTML from the required `filePath`, clears `dirty`, clears `error`, and schedules chunked loading.
- Reading a nonexistent path connects an empty document to that path without creating the file. The first edit creates the file through synchronization if the parent directory is writable.
- Local edits schedule automatic write-through synchronization to `filePath` through `QSaveFile`; successful sync clears `dirty`, clears `error`, and emits `syncFinished`.
- Local edits emit `documentEdited(documentText, documentRevision)` after the internal document model receives the latest editor payload. `documentText` is the authoritative rich document text for that edit turn, and `documentRevision` increases once per emitted editor-originated edit.
- During input method composition, local edit publication is held until composition settles. The internal document model, file sync, and `documentEdited(...)` then receive the committed document payload together, avoiding partial preedit text such as a Korean syllable that has not been committed yet.
- Synchronization is skipped while `reading` is true so a partial document is not written accidentally.
- The edit surface is a native `TextEdit` with `textFormat: TextEdit.RichText`, `wrapMode: TextEdit.Wrap`, mouse selection, persistent selection, IME handling, and clipboard behavior.
- Mobile-target defaults follow `Theme.mobileTarget`; mobile touch drags scroll the internal viewport whenever rich text content overflows, including while the editor is focused. Taps, IME input, cursor placement, and selection remain on the underlying native `TextEdit` path.
- Constructing `LV.TextEditor` without `filePath` is invalid QML because the connected file is part of the component contract.
- Empty paths and file read/sync failures do not intentionally replace the current document; they set `error` and emit the matching failure signal.

## Internal Engine

`TextDocumentModel` is owned by `LV.TextEditor` and is not part of the public QML component API. Tests may locate it by object name to verify storage behavior, but application code should treat `LV.TextEditor` as the file read/write state engine and use the native rich `editorItem` for editing behavior.

## Usage

```qml
import LVRS 1.0 as LV

LV.TextEditor {
    filePath: "/tmp/notes.html"
    chunkSize: 65536

    onSyncFinished: console.log("Synchronized", path)
}
```
