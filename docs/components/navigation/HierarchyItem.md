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
- `expanded`
- direction constants: `directionRight`, `directionLeft`, `directionUp`, `directionDown`
- `selectionDirection` (`int` or string: `auto|right|left|up|down`)
- `resolvedSelectionDirection` (readonly)
- `selected`
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
- `dragPreviewActive`, `dragPreviewOpacity` (used by editable hierarchy drag preview)

Input events:

- `inputEdited(text)`
- `inputSubmitted(text)`
- `applyInputResult(value)` returns normalized `string`

## Behavior Contract

- Row press/click requests activation immediately; click activation does not depend on expansion.
- When used outside `HierarchyList`, row interaction sets local `selected = true`.
- Chevron click toggles `expanded` and requests activation.
- Chevron visibility is gated by `showChevron && hasChildItems`.
- `selectionDirection: "auto"` maps `expanded=false` to right and `expanded=true` to down.
- Explicit direction (`left|up|down|right`) overrides auto mode.
- Property changes notify list helper hooks (`scheduleRefreshState`, `notifyExpansionChanged`, `scheduleNormalizeActiveItem`).
- When `inputable` is `true`, the label region is overlaid by `InputField` in exactly the same geometry as the label bounds.
- Enter submission (`accepted`) applies the value to `inputResult`/`label`, emits `inputSubmitted(text)`, and closes the input overlay (`inputable = false`).
- Losing input focus while editing also closes the input overlay to restore label visibility.
- When the owning list is in `editable` mode and the row is being dragged, `dragPreviewActive` lowers row opacity for the in-place preview.

## Usage

```qml
import LVRS 1.0 as LV

LV.HierarchyItem {
    label: "Camera"
    indentLevel: 2
    showChevron: true
    hasChildItems: true
    expanded: false
    selectionDirection: "auto"
}
```
