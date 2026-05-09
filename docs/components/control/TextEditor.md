# TextEditor

Location: `qml/components/control/input/TextEditor.qml`

`TextEditor` is a file-connected plain-text editor. Its public API is intentionally small: bind a required file path, let the component read that file, and let edits synchronize back to the same path without a separate save step.

## Purpose

- Bind an editable plain-text surface directly to a filesystem path.
- Load UTF-8 text lazily through the internal C++ document engine and synchronize edits through atomic write-through semantics.
- Keep QML as the visible line viewport and input dispatcher.
- Keep whole-document storage, line indexing, cursor state, pending-sync state, and file I/O out of QML.
- Avoid `TextArea`/`TextEdit` as the document model.

## Core API

File connection:

- `filePath` (required): connected filesystem path. `LV.TextEditor` must be created with this property.
- `chunkSize`: byte budget per lazy read step. The internal model clamps very small or very large values.

Read/sync state:

- `dirty`: true while local edits are waiting for filesystem synchronization; false after successful `read()` or automatic sync.
- `reading`: true while the connected file is being decoded in chunks.
- `bytesRead`, `bytesTotal`, `progress`: byte-level read progress.
- `error`: most recent read/sync error.
- `empty`: true when the internal document contains no characters.

Methods:

- `read()`: reload `filePath`. Normal construction and `filePath` changes already read automatically.

Signals:

- `readFinished(path)`
- `readFailed(path, error)`
- `readProgress(path, bytesRead, bytesTotal)`
- `syncFinished(path)`
- `syncFailed(path, error)`

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
    filePath: "/tmp/notes.txt"
}
```

### Read the Connected File

The editor reads the connected file automatically after construction:

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.txt"
}
```

Call `read()` only when the caller explicitly wants to reload `filePath`. It returns `true` when the file stream was opened and lazy loading was scheduled. Completion is reported through `readFinished(path)`.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.txt"

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

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.txt"
    onSyncFinished: console.log("Synchronized", path)
    onSyncFailed: console.warn("Sync failed", path, error)
}
```

`Enter`, `Ctrl+Enter`, and `Cmd+Enter` insert newlines. They do not trigger a save command because saving is not a separate user action.

### Track State

Use the read/sync state properties instead of reading the whole document out of the component.

```qml
LV.TextEditor {
    id: editor
    filePath: "/tmp/notes.txt"
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

State properties:

- `dirty`: local edits are waiting for write-through synchronization.
- `reading`: `read()` is still loading chunks.
- `bytesRead`, `bytesTotal`, `progress`: byte-level read progress.
- `error`: most recent read/sync error.
- `empty`: no document characters are loaded.

### Work with Large Files

Set `chunkSize` to tune lazy loading. The default is suitable for normal use; larger chunks reduce scheduling overhead, smaller chunks keep each event-loop step shorter.

```qml
LV.TextEditor {
    filePath: "/var/log/system.log"
    chunkSize: 262144
}
```

While `reading` is true, keyboard edits are ignored and write-through synchronization is not scheduled, so a partial document is not written accidentally.

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

Set `readOnly` when the editor should load and display a file without accepting text edits.

```qml
LV.TextEditor {
    filePath: "/tmp/report.txt"
    readOnly: true
}
```

### Not Public API

Application code should not depend on internal document objects or whole-text compatibility aliases. These names are intentionally not part of `LV.TextEditor`:

- `text`
- `documentModel`
- `editorItem`, `editorViewport`
- `write()`, `loadFile(path)`, `saveFile(path)`, `reloadFile()`
- `insertText()`, `clear()`, `selectAll()`, `submit()`
- mode or preview APIs such as `mode`, `markdownMode`, `richTextMode`, `renderedOutput`

Layout/visual:

- `placeholderText`, `readOnly`
- `fieldMinHeight`, `editorHeight`, `resolvedEditorHeight`
- `insetHorizontal`, `insetVertical`
- `shapeStyle`, `cornerRadius`
- `showScrollBar`, `autoFocusOnPress`
- `preferNativeGestures`
- `viewportFlickDeceleration`, `viewportMaximumFlickVelocity`
- font and color tokens for the visible line viewport

## Behavior Contract

- The editor automatically calls `read()` after construction and after `filePath` changes.
- `read()` opens UTF-8 text from the required `filePath`, clears `dirty`, clears `error`, and schedules chunked line loading.
- `read()` returns whether the file stream was opened and scheduled; `readFinished` reports completion.
- Reading a nonexistent path connects an empty document to that path without creating the file. The first edit creates the file through synchronization if the parent directory is writable.
- During lazy reading, visible rows are appended to the `ListView` model incrementally and progress is exposed through `readProgress` and `progress`.
- Clean loaded lines remain file-backed line records; visible delegate text decodes individual lines on demand.
- Editing promotes only touched lines to memory-backed records, while untouched lines stay file-backed.
- Local edits schedule automatic write-through synchronization to `filePath` through `QSaveFile`; successful sync clears `dirty`, clears `error`, and emits `syncFinished`.
- Synchronization is skipped while `reading` is true so a partial document is not written accidentally.
- Constructing `LV.TextEditor` without `filePath` is invalid QML because the connected file is part of the component contract.
- Empty paths and file read/sync failures do not intentionally replace the current document; they set `error` and emit the matching failure signal.
- `Enter`, `Ctrl+Enter`, and `Cmd+Enter` insert newlines.
- Uses a vertically constrained `ListView` backed by an internal `TextDocumentModel`; visible delegates are text-line views, not document storage.
- The edit surface does not install a full-cover `MouseArea`; focus is handled by pointer handlers and edits are routed through model methods.
- Mobile-target defaults follow `Theme.mobileTarget`; focused mobile viewports suspend touch flicking so cursor/key interactions stay on the editor path.
- Markdown, rich text, rendered preview, mode switching, submit events, text compatibility aliases, selection API, and save-as path parameters are intentionally outside this component.

## Internal Engine

`TextDocumentModel` is owned by `LV.TextEditor` and is not part of the public QML component API. Tests may locate it by object name to verify storage behavior, but application code should treat `LV.TextEditor` as the file read/write surface.

## Usage

```qml
import LVRS 1.0 as LV

LV.TextEditor {
    filePath: "/tmp/notes.txt"
    chunkSize: 65536

    onSyncFinished: console.log("Synchronized", path)
}
```
