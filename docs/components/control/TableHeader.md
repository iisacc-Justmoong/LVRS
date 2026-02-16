# TableHeader

Location: `qml/components/control/display/TableHeader.qml`

`TableHeader` renders the header row for `Table`.

## Purpose

- Render column labels with uniform width distribution.
- Provide separator styling independent from row body.

## API

- `columns`
- `rowHeight`
- `cellHorizontalPadding`
- `textColor`
- `separatorHeight`
- `separatorColor`

Helper methods:

- `columnAt(index)`
- `columnText(index)`

## Usage

```qml
import LVRS 1.0 as LV

LV.TableHeader {
    columns: ["Column", "Column", "Column"]
}
```

## How It Works

- Column text accepts primitive or object entry (`label/text/title` fallback).
- Repeater delegates use `Layout.fillWidth` for equal distribution.
- Bottom separator is always rendered as dedicated rectangle block.

## Practical Tip

For mixed locale tables, prefer explicit short header labels and rely on tooltip/help text outside the header row for verbose explanations.

## Extended Example: Object-Based Header Definition

```qml
import LVRS 1.0 as LV

LV.TableHeader {
    columns: [
        { label: "Service" },
        { label: "State" },
        { label: "Latency" }
    ]
}
```

## FAQ

Q. Can per-column alignment be defined in `TableHeader` directly?  
A. Current contract keeps uniform left-aligned header text. Per-column layout requires custom derivative component.

## Validation Checklist

- header count matches expected column count,
- separator thickness/color comply with theme contract,
- object-based column labels resolve via fallback keys.
