# MenuItem

Location: `qml/components/navigation/MenuItem.qml`

`MenuItem` is a context-menu row component used by `ContextMenu` delegates.

## Purpose

- Render icon/label/shortcut rows with selected/inactive states.
- Render chevron with configurable submenu direction.

## Core API

State constants and property:

- `defaultState`, `selectedState`, `inactiveState`
- `state`

Direction constants and property:

- `directionRight`, `directionLeft`, `directionUp`, `directionDown`
- `selectionDirection` (`int` or string: `right|left|up|down`)

Content:

- `label`
- `key` / `shortcut` (alias)
- `iconName`, `iconSource`
- `showChevron`

Layout and visuals:

- `itemWidth`, `itemHeight`
- `iconSize`, `chevronSize`
- `iconPlaceholderColor`, `chevronColor`
- resolved: `resolvedIconSource`, `resolvedSelectionDirection`, `resolvedChevronRotation`, `resolvedBackgroundColor`

## Behavior Contract

- `selected`/`inactive` states map to different background colors.
- If icon source cannot be resolved, placeholder block is shown.
- Chevron rotation follows resolved selection direction.

## Usage

```qml
import LVRS 1.0 as LV

LV.MenuItem {
    label: "Open Recent"
    key: "Cmd+O"
    showChevron: true
    selectionDirection: "right"
    state: selectedState
}
```
