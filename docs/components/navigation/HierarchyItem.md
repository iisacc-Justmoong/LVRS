# HierarchyItem

Location: `qml/components/navigation/HierarchyItem.qml`

`HierarchyItem` is a single row component for `HierarchyList`.

## Purpose

- Render tree row with indentation, icon, label, and optional chevron.
- Coordinate activation/expansion state with owning `HierarchyList`.

## Core API

Identity and hierarchy context:

- `itemId`, `itemKey`, `parentItemKey`, `pathLabel`
- `nodeData`
- `hierarchyList`
- `generatedByTreeModel`

Display:

- `label` (alias of `text`)
- `iconName`, `iconSource`, `iconGlyph`
- `showChevron`
- `hasChildItems`
- `effectiveShowChevron` (readonly)
- `expanded`, `selected`

Layout:

- `indentLevel`, `indentStep`, `computedLeftPadding`
- `rowHeight`, `itemWidth`
- `iconSize`, `chevronSize`
- `baseLeftPadding`, `rowRightPadding`, `leadingSpacing`

State visuals:

- `interactionStateName` (readonly: `Idle|Hover|Active`)
- `rowBackgroundColor`, `rowBackgroundColorHover`, `rowBackgroundColorPressed`
- `rowVisible` (readonly, from `_rowVisibleInternal`)

## Behavior Contract

- Row click calls `hierarchyList.requestActivate(control)` when available.
- Chevron click toggles `expanded` and requests activation.
- Chevron visibility is gated by `showChevron && hasChildItems`.
- Property changes notify list helper hooks (`scheduleRefreshState`, `notifyExpansionChanged`, `scheduleNormalizeActiveItem`).

## Usage

```qml
import LVRS 1.0 as LV

LV.HierarchyItem {
    label: "Camera"
    indentLevel: 2
    showChevron: true
    hasChildItems: true
}
```
