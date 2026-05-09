# TableHeader

Location: `qml/components/control/display/TableHeader.qml`

`TableHeader` renders the header row for `Table`.

## Purpose

- Render column labels with uniform or caller-supplied column widths.
- Provide separator styling independent from row body.

## API

- `cellItems` (preferred; array/list-model of TableCellItem-like entries)
- `columns`
- `rowHeight`
- `cellHorizontalPadding`
- `columnWidths`
- `fallbackCellWidth`
- `minColumnWidth`
- `textColor`
- `separatorHeight`
- `separatorColor`
- `cellDelegate`: optional per-header-cell component delegate. The default delegate renders the existing text label.

Helper methods:

- `columnAt(index)`
- `columnText(index)`
- `columnType(index)`
- `normalizeColumnType(value)`
- `inferredColumnType(value)`
- `columnPadding(index)`
- `columnWidth(index)`
- `columnX(index)`

Computed:

- `resolvedColumnSource`
- `resolvedColumnCount`

## Usage

```qml
import LVRS 1.0 as LV

LV.TableHeader {
    columnWidths: [160, 80, 120]
    cellItems: [
        { label: "Name", type: "string" },
        { label: "Count", type: "int" },
        { label: "Enabled", type: "bool" }
    ]
}
```

## How It Works

- `TableHeader` delegates source resolution, typing, and geometry to the C++ `TableHeaderModel`.
- `cellItems` is the primary contract; legacy `columns` remains fallback.
- Column text accepts primitive or object entry (`label/text/title/value` fallback).
- Column type accepts object keys `type`, `valueType`, `cellType`, or `dataType`.
- Primitive header entries infer type: `string -> string`, integer number -> `int`, non-integer number -> `float`, boolean -> `bool`.
- Optional per-column `contentSpacing`/`horizontalPadding` overrides are supported.
- Repeater delegates use `columnWidths` when provided, otherwise `fallbackCellWidth`, otherwise equal auto widths.
- Every rendered header cell instantiates `cellDelegate` and injects one `modelData` object. The descriptor includes `index`, `descriptor`, `cellData`, `text`, `valueType`, `x`, `width`, `height`, and `padding`.
- Custom delegates should declare `property var modelData`; their root item is sized to the resolved header cell bounds.
- Bottom separator is rendered as dedicated rectangle (`panelBackground10` default).

## Backend Model

`TableHeaderModel` owns `resolvedColumnSource`, column descriptors, type normalization, type inference, padding, column width, and x-offset calculation. `TableHeader.qml` renders model descriptors and keeps legacy helper methods as pass-through API.

## Typed Header Contract

`columnType(index)` returns one of `string`, `int`, `float`, or `bool`. It is intentionally header-only metadata; `Table` consumes the same contract to validate body cell edits.

## Practical Tip

Keep header labels short and semantic (field type/category), because dense 12px semi-bold text is optimized for compact metadata.

## Extended Example: Object-Based Header Definition

```qml
import LVRS 1.0 as LV

LV.TableHeader {
    cellItems: [
        { label: "Service", type: "string" },
        { text: "State", type: "bool", contentSpacing: LV.Theme.gap8 },
        { title: "Latency", type: "float" }
    ]
}
```

## Delegate Example

```qml
Component {
    id: compactHeaderCell

    Item {
        property var modelData: ({})

        LV.Label {
            anchors.left: parent.left
            anchors.leftMargin: modelData.padding || 0
            anchors.verticalCenter: parent.verticalCenter
            text: modelData.text
        }
    }
}

LV.TableHeader {
    cellItems: [{ label: "Name" }, { label: "Count", type: "int" }]
    cellDelegate: compactHeaderCell
}
```

## FAQ

Q. Can per-column alignment be defined in `TableHeader` directly?  
A. Current contract keeps uniform left-aligned header text. Per-column layout requires custom derivative component.

## Validation Checklist

- header count matches expected column count,
- `columnType(index)` matches declared or inferred field type,
- separator thickness/color comply with theme contract,
- object-based column labels resolve via fallback keys.
