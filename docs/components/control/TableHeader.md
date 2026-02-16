# TableHeader

Location: `qml/components/control/display/TableHeader.qml`

`TableHeader` renders the header row for `Table`.

## Purpose

- Render column labels with uniform width distribution.
- Provide separator styling independent from row body.

## API

- `cellItems` (preferred; array/list-model of TableCellItem-like entries)
- `columns`
- `rowHeight`
- `cellHorizontalPadding`
- `textColor`
- `separatorHeight`
- `separatorColor`

Helper methods:

- `columnAt(index)`
- `columnText(index)`
- `columnPadding(index)`

Computed:

- `resolvedColumnSource`
- `resolvedColumnCount`

## Usage

```qml
import LVRS 1.0 as LV

LV.TableHeader {
    cellItems: [
        { label: "Column" },
        { label: "Column" },
        { label: "Column" }
    ]
}
```

## How It Works

- `cellItems` is the primary contract; legacy `columns` remains fallback.
- Column text accepts primitive or object entry (`label/text/title` fallback).
- Optional per-column `contentSpacing`/`horizontalPadding` overrides are supported.
- Repeater delegates use `Layout.fillWidth` for equal distribution.
- Bottom separator is rendered as dedicated rectangle (`panelBackground10` default).

## Practical Tip

Keep header labels short and semantic (field type/category), because dense 12px semi-bold text is optimized for compact metadata.

## Extended Example: Object-Based Header Definition

```qml
import LVRS 1.0 as LV

LV.TableHeader {
    cellItems: [
        { label: "Service" },
        { text: "State", contentSpacing: LV.Theme.gap8 },
        { title: "Latency" }
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
