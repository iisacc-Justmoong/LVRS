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
- `itemDelegate`: optional per-row component delegate. The default delegate renders the existing compact button row.

Toolbar/footer:

- `toolbarVisible`, `toolbarIcon1`, `toolbarIcon2`, `toolbarIcon3`
- `footerVisible`, `footerButton1`, `footerButton2`, `footerButton3`
- Stock toolbar/footer buttons use `Theme.iconSm` icon frames (`18 x 18` desktop, `23 x 23` mobile) and a scaled `Theme.scaleMetric(1)` compact inset (`ListFooter.stockButtonPadding`).

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
- Direct model input is read through the C++ `ModelSource` type; QML no longer owns per-source count/index/role dispatch.
- C++ item model changes (`rowsInserted`, `rowsRemoved`, `rowsMoved`, `modelReset`, `layoutChanged`, `dataChanged`) invalidate `ModelSource.revision` so `entryCount` and rows refresh.
- Primitive rows render with `String(value)`.
- Object/model rows render label text from `labelRole`, then `textRole`, then `titleRole`, then `display`, then `edit`.
- `enabledRole` and `selectedRole` are optional. Missing `enabledRole` means enabled; missing `selectedRole` falls back to `selectedIndex`.
- `itemTriggered` emits the resolved row entry returned by `entryAt(index)`.
- Every rendered row instantiates `itemDelegate` and injects one `modelData` object. The descriptor includes `index`, `entry`, `label`, `enabled`, `selected`, and `trigger()`.
- Custom delegates should declare `property var modelData` and call `modelData.trigger()` when they want the list to emit the normalized `itemTriggered` signal.

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

```qml
Component {
    id: customRow

    LV.AbstractButton {
        property var modelData: ({})
        text: modelData.label || ""
        enabled: modelData.enabled === true
        onClicked: modelData.trigger()
    }
}

LV.List {
    model: [{ label: "Overview" }, { label: "Settings" }]
    itemDelegate: customRow
}
```
