# TableRow

Location: `qml/components/control/display/TableRow.qml`

`TableRow` renders one data row by repeating `TableCellItem` delegates.

## Purpose

- Convert row input into visual cell delegates.
- Preserve compact Figma-style row geometry.

## API

- `cellItems` (preferred; array/list-model of TableCellItem-like entries)
- `cells`
- `cellWidth`
- `cellHeight`
- `contentSpacing`
- `dividerColor`
- `textColor`
- `inputable` (default `false`; row-level editable default for cells)

Signals:

- `cellInputEdited(columnIndex, text)`
- `cellInputSubmitted(columnIndex, text)`

Helper methods:

- `cellAt(index)`
- `cellText(index)`
- `cellInputable(index)`

Computed:

- `resolvedCellSource`
- `resolvedCellCount`
- `resolvedSpacing`

## Usage

```qml
import LVRS 1.0 as LV

LV.TableRow {
    cellItems: [
        { text: "A" },
        { text: "B" },
        { text: "C" }
    ]
}
```

## How It Works

- `cellItems` is the primary contract; legacy `cells` remains fallback.
- Each entry is forwarded to `TableCellItem.itemData`.
- Cell text resolves from primitive values or object fallback keys (`label/text/title`).
- Cell inputability resolves by `entry.inputable` override first, then row-level `inputable`.
- Row spacing is computed from available width and fixed cell width.
- Spacing never goes negative (`Math.max(0, computed)`).
- The default leading divider uses `panelBackground10` to match the measured row export.

## Practical Tip

For exact Figma matching, keep `cellWidth: 234`, `cellHeight: 24`, and row width `717`.

## Extended Example: Mixed Primitive/Object Cells

```qml
import LVRS 1.0 as LV

LV.TableRow {
    cellItems: [
        "Renderer",
        { text: "Active", dividerColor: LV.Theme.panelBackground10 },
        { title: "Core", textColor: LV.Theme.bodyColor }
    ]
}
```

## FAQ

Q. Does `cells` still work?  
A. Yes. `cells` is preserved for compatibility, but `cellItems` is the preferred API.

## Figma Contract

Node `203:3648` is `717 × 24`: three `234 × 24` cells separated by `7.5` spacing. Body text is fixed at 13/13. Mobile geometry doubles to `1434 × 48`, `468` cell width, and `16` content spacing; Body font remains 13.
