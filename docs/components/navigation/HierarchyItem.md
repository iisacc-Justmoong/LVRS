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
- `inputable` (default `false`; enables inline input overlay at label bounds)
- `inputResult` (latest editable label string)

Layout:

- `indentLevel`, `indentStep` (default `8`), `computedLeftPadding`
- `rowHeight` (default `20`), `itemWidth` (default `200`)
- `iconSize`, `chevronSize`
- `baseLeftPadding` (default `8`), `rowRightPadding` (default `8`), `leadingSpacing` (default `2`)

State visuals:

- `interactionStateName` (readonly: `Idle|Hover|Active`)
- `rowBackgroundColor`, `rowBackgroundColorHover`, `rowBackgroundColorPressed`
- `rowVisible` (readonly, from `_rowVisibleInternal`)

Input events:

- `inputEdited(text)`
- `inputSubmitted(text)`
- `applyInputResult(value)` returns normalized `string`

## Behavior Contract

- Row click calls `hierarchyList.requestActivate(control)` when available.
- Chevron click toggles `expanded` and requests activation.
- Chevron visibility is gated by `showChevron && hasChildItems`.
- Property changes notify list helper hooks (`scheduleRefreshState`, `notifyExpansionChanged`, `scheduleNormalizeActiveItem`).
- When `inputable` is `true`, the label region is overlaid by `InputField` in the same geometry and exposes edited text through `inputResult`.

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
