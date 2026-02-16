# MenuItem

Location: `/Users/ymy/Developer/LVRS/qml/components/navigation/MenuItem.qml`

`MenuItem` is a context-menu row component with icon, label, shortcut, state styling, and chevron direction control.

## Purpose

- Render one context-menu action row with compact spacing.
- Support icon/text binding and key/shortcut binding.
- Expose submenu selection direction through a simple input API.

## API

State:

- `defaultState`
- `selectedState`
- `inactiveState`
- `state`

Content:

- `label`
- `key` / `shortcut`
- `iconName`
- `iconSource`

Chevron:

- `showChevron`
- `selectionDirection` (supports `right|left|up|down` string or numeric enum)
- direction enum constants:
  - `directionRight`
  - `directionLeft`
  - `directionUp`
  - `directionDown`

Layout:

- `itemWidth` (default: `161`)
- `itemHeight` (default: `22`)
- `iconSize` (default: `16`)
- `chevronSize` (default: `16`)

## Usage

```qml
import LVRS 1.0 as LV

LV.MenuItem {
    iconName: "projectStructure"
    label: "Open Recent"
    key: "Cmd+O"
    showChevron: true
    selectionDirection: "right"
    state: defaultState
}
```

## How It Works

- Background is resolved from `state`:
  - default: transparent
  - selected: `Theme.contextMenuItemSelectedBackground`
  - inactive: `Theme.panelBackground08`
- Icon source resolution order:
  1. `iconSource`
  2. `iconName` via `Theme.iconPath(...)`
  3. placeholder block
- Chevron path is painted on `Canvas` and rotates by resolved direction.

## Practical Notes

- Use `selectionDirection` to bind submenu opening side from runtime data.
- Keep label/key strings concise to preserve menu rhythm.
- If a row is final action (no submenu), set `showChevron: false`.

## Validation Checklist

- row size matches `161x22` contract,
- selected and inactive colors render correctly,
- chevron direction follows `selectionDirection`,
- icon and text bindings update without manual refresh.
