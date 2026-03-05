# MenuItem

Location: `qml/components/navigation/MenuItem.qml`

`MenuItem` is a context-menu row component used by `ContextMenu` delegates.

## Purpose

- Render icon/label/key rows with selected/inactive states.
- Control key visibility and chevron visibility independently.
- Drive chevron direction from fold/expand state when direction is `auto`.

## Core API

State constants and property:

- `defaultState`, `selectedState`, `inactiveState`
- `state`

Direction constants and property:

- `directionRight`, `directionLeft`, `directionUp`, `directionDown`
- `selectionDirection` (`int` or string: `auto|right|left|up|down`)

Content:

- `label`
- `key` / `shortcut` (alias)
- `keyVisible` (boolean)
- `keyPlaceholder` (default `"key"`)
- `iconName`, `iconSource`
- `showChevron`
- `hasChildItems`
- `expanded`
- `effectiveShowChevron` (readonly: `showChevron && hasChildItems`)

Layout and visuals:

- `itemWidth`, `itemHeight`
- `iconSize`, `chevronSize`
- `iconPlaceholderColor`, `chevronColor`
- resolved: `resolvedIconSource`, `resolvedShortcutText`, `resolvedSelectionDirection`, `resolvedChevronRotation`, `resolvedBackgroundColor`

## Behavior Contract

- `selected`/`inactive` states map to different background colors.
- If icon source cannot be resolved, placeholder block is shown.
- Key text is hidden when `keyVisible` is `false`.
- If `keyVisible` is `true` and `key` is empty, `keyPlaceholder` is rendered.
- Chevron is shown only when `effectiveShowChevron` is `true`.
- `selectionDirection: "auto"` maps `expanded=false` to right and `expanded=true` to down.

## Usage

```qml
import LVRS 1.0 as LV

LV.MenuItem {
    label: "Open Recent"
    key: "Cmd+O"
    keyVisible: true
    hasChildItems: true
    showChevron: true
    expanded: false
    selectionDirection: "auto"
    state: selectedState
}
```
