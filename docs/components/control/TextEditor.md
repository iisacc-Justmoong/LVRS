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
- viewport scroll physics: `viewportFlickDeceleration`, `viewportMaximumFlickVelocity`
- readonly viewport policy: `viewportBoundsBehavior`, `viewportBoundsMovement`

Signals and methods:

- `textEdited(text)`, `submitted(text)`
- `forceEditorFocus()`, `insertText(value)`, `clear()`, `select()`, `selectAll()`, `deselect()`, `cut()`, `copy()`, `paste()`, `undo()`, `redo()`, `submit()`

## Behavior Contract

- Submit shortcut: `Ctrl+Enter` or `Cmd+Enter`.
- Preview pane is shown only when `showRenderedOutput == true`.
- Uses a vertically constrained `Flickable`-hosted `TextArea` (`TextArea.flickable`) for the edit surface, plus `InputMethodGuard` and a `WheelScrollGuard` for the preview pane.
- Mobile-target defaults follow `Theme.mobileTarget`; on iOS-target the underlying `TextEdit` switches to `NativeRendering` for rasterization, and the editor viewport suspends touch flicking while the text control holds focus so double-tap selection, selection-handle drag, and OS keyboard edit gestures are not pre-empted by container scrolling.
- Mobile-target scroll defaults keep tuned flick momentum (`viewportFlickDeceleration`, `viewportMaximumFlickVelocity`) while bounds remain clamped (`StopAtBounds`) on both edit and preview viewports.
- Current defaults force plain text wrapping (`resolvedWrapMode`, `resolvedTextFormat`) unless component logic is changed.

## Usage

```qml
import LVRS 1.0 as LV

LV.TextEditor {
    text: "# Title"
    showRenderedOutput: true
}
```
