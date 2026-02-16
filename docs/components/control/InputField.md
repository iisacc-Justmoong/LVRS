# InputField

Location: `qml/components/control/input/InputField.qml`

`InputField` is the single-line text input built on `AbstractInputBar`.

## Purpose

- Provide compact single-line text entry with optional search mode.
- Include built-in search icon and clear-button behavior.
- Reuse IME safety and focus behavior from shared input base.

## API

Mode constants:

- `defaultMode`
- `searchMode`

Mode properties:

- `mode`
- `searchIconVisible` (defaults to `mode === searchMode`)
- `clearButtonVisible`
- `showClearButton` (computed)

Visual customization:

- `searchIconColor`, `searchIconStrokeWidth`
- `clearIconBackgroundColor`, `clearIconBackgroundColorDisabled`
- `clearIconForegroundColor`

Inherited text API (`AbstractInputBar`):

- `text`, `placeholderText`, `readOnly`, `validator`, `maximumLength`, `inputMethodHints`, and others.

## Usage

```qml
import LVRS 1.0 as LV

LV.InputField {
    mode: searchMode
    placeholderText: "Search"
}
```

## How It Works

- Search icon is painted via `Canvas` and repainted on mode/style changes.
- Clear button appears only when editable and text is non-empty.
- Clear action empties text and immediately restores input focus.
- Cursor uses custom blink delegate and inherits IME guard from base component.

## Advanced Example: Validated Input Field

```qml
import QtQuick
import LVRS 1.0 as LV

LV.InputField {
    placeholderText: "Project ID"
    validator: RegularExpressionValidator { regularExpression: /[A-Z0-9_-]{3,20}/ }
    onAccepted: console.log("submitted", text)
}
```

## Troubleshooting

If clear button does not appear, verify:

- `clearButtonVisible == true`
- `readOnly == false`
- current text length > 0

## Recipe: Search Field with Debounced Query

```qml
import QtQuick
import LVRS 1.0 as LV

Item {
    property string pendingQuery: ""

    Timer {
        id: debounce
        interval: 180
        repeat: false
        onTriggered: runSearch(pendingQuery)
    }

    LV.InputField {
        mode: searchMode
        placeholderText: "Search logs"
        onTextEdited: function(value) {
            pendingQuery = value
            debounce.restart()
        }
    }
}
```

## Failure Pattern

Using expensive synchronous filtering inside `onTextEdited` causes keystroke latency.
Debounce or offload heavy work.
