# HierarchyItem

Location: `qml/components/navigation/HierarchyItem.qml`

`HierarchyItem` is one row of the hierarchy list, with icon, label, and tree-depth indentation.

## Purpose

- Render one node row in tree hierarchy.
- Display icon and label with Figma-sized rhythm (`28` row height, `16` icon).
- Reflect hierarchy depth with computed left padding.

## API

Primary data:

- `label` (alias of `text`)
- `iconName`
- `iconSource`
- `iconGlyph`
- `showChevron`
- `expanded`
- `enabled`

Hierarchy context:

- `indentLevel`
- `indentStep` (default `13`)
- `baseLeftPadding` (default `8`)
- `computedLeftPadding` (`baseLeftPadding + indentLevel * indentStep`)

Identity:

- `itemId`
- `itemKey`
- `parentItemKey`
- `pathLabel`
- `nodeData`

Signals/interaction:

- row click activates item through owning `HierarchyList`
- chevron click toggles `expanded`

## Usage

```qml
import LVRS 1.0 as LV

LV.HierarchyItem {
    label: "Camera"
    iconName: "projectStructure"
    indentLevel: 2
    showChevron: true
}
```

## Operational Notes

- Default chevron color follows Figma tone via `Theme.darkGrey10`.
- When used inside `HierarchyList`, activation/visibility is managed by list state.
