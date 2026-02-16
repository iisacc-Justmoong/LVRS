# TableCellItem

Location: `qml/components/control/display/TableCellItem.qml`

`TableCellItem` is the smallest table primitive: leading divider + single text body.

## Purpose

- Render one compact table cell that matches Figma row geometry.
- Resolve text and visual overrides directly from a table-cell data object.

## API

- `itemData` (object-like cell descriptor; optional)
- `text`
- `cellHeight`
- `contentSpacing`
- `dividerColor`
- `textColor`
- `showDivider`
- `clipContent`
- `textStyle` (Label style enum value)

Resolved read-only values:

- `resolvedText`
- `resolvedCellHeight`
- `resolvedContentSpacing`
- `resolvedDividerColor`
- `resolvedTextColor`
- `resolvedShowDivider`
- `resolvedClipContent`
- `resolvedTextStyle`

## Usage

```qml
import LVRS 1.0 as LV

LV.TableCellItem {
    itemData: ({
        text: "Renderer",
        dividerColor: LV.Theme.panelBackground03
    })
}
```

## How It Works

- `itemData` supports object keys such as `label`, `text`, `title`.
- If object keys are missing, component-level fallback props are used.
- Divider and text rendering stay dense (`24px` cell height, ellipsis text).

## Practical Tip

Use `itemData` for model-driven tables and keep primitive `text` assignment for static/one-off cases.

## Extended Example: Header-like Cell Styling

```qml
import LVRS 1.0 as LV

LV.TableCellItem {
    itemData: ({ label: "Column" })
    showDivider: false
    textStyle: description
    textColor: LV.Theme.descriptionColor
}
```

## FAQ

Q. Which text key is used when `itemData` contains multiple label fields?  
A. Resolution order is `label -> text -> title`, then falls back to component `text`.

## Validation Checklist

- divider visibility matches design requirement,
- text elision works under narrow widths,
- color tokens match table design (`panelBackground03` divider, body text).
