# List

Location: `qml/components/navigation/List.qml`

`List` renders all 17 [ListItem variants](ListItem.md), including mixed compact and compound rows, from a direct model or a legacy `items` array. A Mini-only list retains the measured Figma `SmallList` surface and `ListFooter`.

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
- `iconRole` (default `iconName`)
- `enabledRole` (default `enabled`)
- `selectedRole` (default `selected`)
- `typeRole` (default `type`): accepts a ListItem enum or exact Figma type name; falls back to legacy `size`, then `defaultItemType` (`Mini`).
- `descriptionRole` (default `description`)

Layout and state:

- `selectedIndex` (default `-1`), `interactive`
- `listWidth` (`170` desktop), `minimumListHeight` (`140` desktop), `itemHeight` (`22` desktop), `itemLabelLeftPadding` (`4` desktop)
- `defaultItemIconName` (default `nodesfolder`)
- `expandToContent` (default `false`): grows to the sum of actual row heights, row gaps and toolbar/footer.
- `scrollable` (default `true`): enables vertical scrolling when content exceeds the viewport.
- `itemSpacing` (default `0`); `itemHeight` is a per-row minimum, not a fixed stride.
- `listWidth` defaults to the widest required preset: 170 for Mini, 194 for Detail, 280 for Navigation/Toggle/Checkable, and 400 for composite types. Explicit width overrides remain supported.
- `backgroundColor`, `selectedRowColor`, `separatorColor`, `separatorOpacity`
- `itemDelegate`: optional per-row component delegate. The default uses `ListItem` with the model's type, falling back to `Mini`.

Toolbar/footer:

- `toolbarVisible`, `toolbarIcon1`, `toolbarIcon2`, `toolbarIcon3`
- `footerVisible`, `footerButton1`, `footerButton2`, `footerButton3`
- Default footer slots are `addFile`, `generaldelete`, and a `settings` menu button.
- Stock footer buttons use `Theme.iconSm` icon frames (`18 x 18` desktop, `36 x 36` mobile), `Theme.gap2` insets, and `ListFooter.stockMenuButtonSpacing` (`-2` desktop, `-4` mobile) between the settings icon and chevron.

Signals:

- `itemTriggered(index, item)`
- `itemEdited(index, item, field, value)`
- `itemActionTriggered(index, item, action, payload)`
- `toolbarIconTriggered(index, source)`
- `footerButtonTriggered(index, config)`

## Figma Visual Contract

- Sources: `203:5161` (`SmallList`), `209:9199` (`ListFooter`), and `241:9253` (`ListItem` component set).
- Desktop `SmallList` is `170 x 140` with `Theme.panelBackground03`. Its item viewport is `170 x 114`; six `22px` rows occupy `132px`, so the final row is intentionally clipped above the `26px` footer.
- The default row is the `Mini` variant: horizontal/vertical padding `4/2`, `nodesfolder` icon `18 x 18`, `1px` icon-label gap, and Body `13px Medium / 13px` text. Body remains fixed at `13px` on desktop and mobile.
- `ListFooter` is `86 x 26`: outer padding `2`, button frames `22 x 22`, and slot widths `22`, `22`, and `38`. The exact existing `addFile`, `generaldelete`, `settings`, and `generalchevronDownBorderless` SVG assets are reused.
- Desktop and mobile share the same geometry: list `170 x 140`, Mini row `170 x 22`, Detail row `194 x 106`, and footer `86 x 26`; Body text stays `13px`.

## Behavior Contract

- `model` takes precedence over `items`.
- Supported direct model inputs:
  - JavaScript arrays and primitive arrays,
  - object arrays,
  - QML `ListModel`/list-like objects with `count` and `get(index)`,
  - C++ `QAbstractItemModel` instances exposed to QML.
- Direct model input is read through the C++ `ModelSource` type.
- C++ item model changes (`rowsInserted`, `rowsRemoved`, `rowsMoved`, `modelReset`, `layoutChanged`, `dataChanged`) invalidate `ModelSource.revision` so `entryCount` and rows refresh.
- Primitive rows render with `String(value)` and use `defaultItemIconName`; their label is never interpreted as an icon name.
- Object/model rows resolve label text from `labelRole`, then `textRole`, then `titleRole`, then `display`, then `edit`.
- `enabledRole` and `selectedRole` are optional. Missing `enabledRole` means enabled; missing `selectedRole` falls back to `selectedIndex`.
- `itemTriggered` emits the resolved row entry returned by `entryAt(index)`.
- Every rendered row instantiates `itemDelegate` and injects one `modelData` object. The descriptor includes `index`, `entry`, `label`, `iconName`, `enabled`, `selected`, `type`, `description`, `properties` (the original object/role map), and `trigger()`.
- The default delegate forwards ListItem content, state, configuration objects and visibility properties from the row entry. See [ListItem](ListItem.md) for the supported names.
- Model refresh updates the existing delegate's `modelData`. It does not destroy/recreate an editor merely because its text or selection changed. Changing `itemDelegate` replaces the delegate.
- User changes emit `itemEdited`; the application owns model persistence. Update an array, call QML `ListModel.setProperty`, or call the C++ model's setter in that handler. No arbitrary application model is mutated automatically. Refreshed model data remains authoritative.
- A Column positions delegates using each delegate's actual height; compound rows do not overlap subsequent rows. A Flickable exposes overflow while the footer remains outside the scrolling viewport.
- Custom delegates should declare `property var modelData` and call `modelData.trigger()` when they want the list to emit the normalized `itemTriggered` signal.

## Legacy ListItem Variants

- `size: ListItem.Mini` is `170 x 22` on desktop and retains the existing `inputable`, `inputResult`, `inputEdited`, and `inputSubmitted` API. The inline editor uses a fixed Body `13 / 13` line box on desktop and mobile, with the same row geometry on both.
- `size: ListItem.Detail` is `194 x 106` on desktop. It uses `12px SemiBold / 12px` title and date text, `11px Regular / 11px` metadata, `12/8` horizontal/vertical padding, and `8px` section gaps.
- Detail data is exposed through `detail`, `dateText`, `folderLabel1`, `folderLabel2`, `tagLabel1`, and `tagLabel2`; its icon names default to `bookmarksbookmark`, `folder@14x14`, and `vcscurrentBranch`.
- `separatorVisible` defaults to `false` because neither Figma variant contains a separator. Legacy separator color/size properties remain available for explicit opt-in.

The remaining 15 presets and their interactive configuration are documented in [ListItem](ListItem.md). The Visual Catalog List section demonstrates all 17 types and persists edits back to its example model.

## Usage

```qml
LV.List {
    id: mixedList
    expandToContent: true
    footerVisible: false
    items: [
        { type: "Navigation", label: "Library", value: "24" },
        { type: "Form", label: "Metadata", inputText1: "Project" }
    ]
    onItemEdited: function(index, item, field, value) {
        const next = items.slice()
        next[index] = Object.assign({}, item)
        next[index][field] = value
        items = next
    }
    onItemActionTriggered: function(index, item, action, payload) {
        console.log(index, action, payload)
    }
}
```

```qml
import LVRS 1.0 as LV

LV.List {
    model: [
        { label: "Overview", iconName: "nodesfolder" },
        { label: "Settings", enabled: false }
    ]
}
```

```qml
LV.ListItem {
    size: LV.ListItem.Detail
    detail: "Two-line note title"
    dateText: "2026-08-16"
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
