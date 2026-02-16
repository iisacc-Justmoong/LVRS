# TableRow

Location: `qml/components/control/display/TableRow.qml`

`TableRow` renders one data row by repeating `TableCellItem` delegates.

## Purpose

- Convert row input into visual cell delegates.
- Preserve compact Figma-style row geometry.

## API

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

- `resolvedCellCount`
- `resolvedSpacing`

## Usage

```qml
import LVRS 1.0 as LV

LV.TableRow {
    cells: ["A", "B", "C"]
}
```

## How It Works

- Cell text resolves from primitive values or object fallback keys.
- Row spacing is computed from available width and fixed cell width.
- Spacing never goes negative (`Math.max(0, computed)`).

## Practical Tip

When using fixed `cellWidth`, ensure row container width is large enough to avoid excessive zero spacing from clamping.

## Extended Example: Mixed Primitive/Object Cells

```qml
import LVRS 1.0 as LV

LV.TableRow {
    cells: [
        "Renderer",
        { text: "Active" },
        { title: "Core" }
    ]
}
```

## FAQ

Q. Why does row spacing collapse to zero?  
A. Container width is smaller than `cellCount * cellWidth`; computed spacing is clamped at zero.
