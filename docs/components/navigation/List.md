# List

Location: `qml/components/navigation/List.qml`

`List` is a compact navigation list that renders rows from a direct model or a legacy `items` array.

## Purpose

- Render simple one-column navigation rows.
- Accept direct model inputs without requiring callers to transform data into child components.
- Keep toolbar/footer affordances compatible with existing LVRS navigation surfaces.

## Core API

Model input:

- `model`: preferred direct row source.
- `items`: legacy row source used only when `model` is `null`/`undefined`.
- `usingModel` (readonly): true when `model` is the active source.
- `entryCount` (readonly), `entryAt(index)`.
- `modelColumn`: column used when `model` is a C++ `QAbstractItemModel`.

Role mapping:

- `labelRole` (default `label`)
- `textRole` (default `text`)
- `titleRole` (default `title`)
- `enabledRole` (default `enabled`)
- `selectedRole` (default `selected`)

Layout and state:

- `selectedIndex`, `interactive`
- `listWidth`, `minimumListHeight`, `itemHeight`, `itemLabelLeftPadding`
- `backgroundColor`, `selectedRowColor`, `separatorColor`, `separatorOpacity`

Toolbar/footer:

- `toolbarVisible`, `toolbarIcon1`, `toolbarIcon2`, `toolbarIcon3`
- `footerVisible`, `footerButton1`, `footerButton2`, `footerButton3`

Signals:

- `itemTriggered(index, item)`
- `toolbarIconTriggered(index, source)`
- `footerButtonTriggered(index, config)`

## Behavior Contract

- `model` takes precedence over `items`.
- Supported direct model inputs:
  - JavaScript arrays and primitive arrays,
  - object arrays,
  - QML `ListModel`/list-like objects with `count` and `get(index)`,
  - C++ `QAbstractItemModel` instances exposed to QML.
- C++ item models are read through `ModelAdapter` and converted to a role-name map per row.
- C++ item model changes (`rowsInserted`, `rowsRemoved`, `rowsMoved`, `modelReset`, `layoutChanged`, `dataChanged`) invalidate the list projection so `entryCount` and rows refresh.
- Primitive rows render with `String(value)`.
- Object/model rows render label text from `labelRole`, then `textRole`, then `titleRole`, then `display`, then `edit`.
- `enabledRole` and `selectedRole` are optional. Missing `enabledRole` means enabled; missing `selectedRole` falls back to `selectedIndex`.
- `itemTriggered` emits the resolved row entry returned by `entryAt(index)`.

## Usage

```qml
import LVRS 1.0 as LV

LV.List {
    model: [
        { label: "Overview", selected: true },
        { label: "Settings", enabled: false }
    ]
}
```

```qml
LV.List {
    model: backendModel
    labelRole: "name"
}
```
