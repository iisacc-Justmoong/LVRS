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

Helper methods:

- `cellAt(index)`
- `cellText(index)`

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
- Row spacing is computed from available width and fixed cell width.
- Spacing never goes negative (`Math.max(0, computed)`).

## Practical Tip

For exact Figma matching, keep `cellWidth: 234`, `cellHeight: 24`, and row width `717`.

## Extended Example: Mixed Primitive/Object Cells

```qml
import LVRS 1.0 as LV

LV.TableRow {
    cellItems: [
        "Renderer",
        { text: "Active", dividerColor: LV.Theme.panelBackground03 },
        { title: "Core", textColor: LV.Theme.bodyColor }
    ]
}
```

## FAQ

Q. Does `cells` still work?  
A. Yes. `cells` is preserved for compatibility, but `cellItems` is the preferred API.
