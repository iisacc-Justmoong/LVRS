# TextDocumentModel

Location: `backend/text/textdocumentmodel.h`, `backend/text/textdocumentmodel.cpp`

`TextDocumentModel` is the internal C++ document engine behind `LV.TextEditor`.

## Purpose

- Own the connected `filePath`, document line index, cursor position, dirty state, lazy load state, and file errors.
- Expose document lines as a `QAbstractListModel` for a virtualized QML viewport.
- Load files incrementally in byte chunks and keep clean file lines as file-offset records instead of retained `QString` line storage.
- Keep QML out of the document storage role; QML renders and dispatches input only.

## API

Properties:

- `filePath`, `hasFilePath`
- `text`, `characterCount`
- `lineCount`
- `fileBackedLineCount`, `memoryLineCount`
- `cursorLine`, `cursorColumn`, `cursorPosition`
- `dirty`
- `lastError`
- `loading`
- `loadedByteCount`, `totalByteCount`, `loadProgress`
- `loadChunkSize`

Model roles:

- `lineIndex`
- `lineNumber`
- `text`
- `length`

Methods:

- `lineText(line)`, `lineLength(line)`
- `loadFile(path?)`, `reloadFile()`, `saveFile(path?)`, `cancelLoad()`
- `markClean()`, `clear()`
- `moveCursor(line, column)`, `moveCursorLeft()`, `moveCursorRight()`, `moveCursorUp()`, `moveCursorDown()`
- `moveCursorLineStart()`, `moveCursorLineEnd()`
- `insertText(value)`, `insertNewline()`
- `removePreviousCharacter()`, `removeNextCharacter()`

Signals:

- `fileLoaded(path, length)`, `fileLoadFailed(path, error)`
- `fileLoadProgress(path, loadedBytes, totalBytes)`
- `fileSaved(path, length)`, `fileSaveFailed(path, error)`

## Behavior Contract

- Files are read as UTF-8 text.
- `loadFile()` opens the target file and returns after scheduling chunked loading; it does not build one whole-document `QString` up front.
- `loadFile()` treats a nonexistent target path as an empty connected document so `LV.TextEditor` can create the file on the first edit.
- Loading reads at most `loadChunkSize` bytes per event-loop turn and appends decoded lines into the list model as chunks arrive.
- Clean loaded lines are stored as `(byteOffset, byteLength, length)` records and decoded only when `lineText()`, a model delegate, or `text` asks for the content.
- Edits promote only the touched line records to memory-backed text records; untouched file-backed lines stay connected to the source file.
- `loading` is true until the final chunk is decoded or `cancelLoad()`/failure stops the stream.
- `loadedByteCount`, `totalByteCount`, and `loadProgress` report byte-level load progress.
- The `text` property is an internal engine surface and materializes all loaded lines when read; large-document UI should prefer model rows, `lineText(line)`, and progress/storage properties.
- Saves use `QSaveFile` so successful writes are atomic from the caller perspective.
- Newlines are normalized to `\n` inside the model.
- The model always contains at least one line, even for an empty document.
- Edit operations mutate line storage and set `dirty`.
- `loadFile()` resets line storage, resets cursor to the document start, clears `dirty`, streams file content into rows, and emits `fileLoaded` when the stream is complete.
- Edit operations call `cancelLoad()` before mutating loaded text; saving while `loading` fails instead of writing a partial document.
- `saveFile()` streams current line records to a `QSaveFile`, rebuilds file-backed line records for the saved file, clears `dirty`, and emits `fileSaved`.

## Consumer

- `LV.TextEditor` owns one `TextDocumentModel` instance and uses it as the backing model for its QML `ListView`.
- Application code should use the smaller `LV.TextEditor` file API (`filePath`, optional `read()`, automatic sync signals) instead of reaching into this model.
