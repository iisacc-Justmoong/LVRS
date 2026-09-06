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
- `inputable` (default `false`; toggles inline input overlay at text bounds)
- `selected`, `current`
- `selectionColor`, `currentBorderColor`, `currentBorderWidth`
- `inputResult` (latest editable string value)
- `valueType` (`string`, `int`, `float`, `bool`; used by injected validators)
- `valueValidator` (optional function injected by parent table)
- `typedValue` (latest accepted typed value)
- `inputAccepted` (last validation result)

Input events:

- `inputEdited(text)`
- `inputSubmitted(text)`
- `inputRejected(text, valueType)`
- `applyInputResult(value)` returns normalized `string`

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
- `itemData.value` is also accepted when label/text/title keys are absent.
- If object keys are missing, component-level fallback props are used.
- Divider and text rendering stay dense (`24px` cell height, ellipsis text).
- `selected` fills the cell selection layer; `current` draws the current-cell border. `Table` supplies both states automatically to its default delegate.
- When `inputable` is enabled, `InputField` overlays the text area and keeps geometry aligned with the original label slot. Its embedded line box and native input remain fixed at Body `13 / 13` on desktop and mobile, avoiding the former clipped `16 / 32` line box.
- `valueValidator` is a parent-injected API. It may return `true/false` or `{ accepted, text, value }`; rejected edits leave `inputResult` unchanged and emit `inputRejected`.

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

## Extended Example: Typed Edit Guard

```qml
import LVRS 1.0 as LV

LV.TableCellItem {
    valueType: "int"
    valueValidator: function(value) {
        const text = String(value).trim()
        if (!/^-?\d+$/.test(text))
            return { accepted: false, text: text, value: value }
        const intValue = Number(text)
        return { accepted: true, text: String(intValue), value: intValue }
    }
}
```

## FAQ

Q. Which text key is used when `itemData` contains multiple label fields?  
A. Resolution order is `label -> text -> title -> value`, then falls back to component `text`.

## Validation Checklist

- divider visibility matches design requirement,
- text elision works under narrow widths,
- color tokens match table design (`panelBackground03` standalone divider, body text).

## Figma Contract

Node `203:3863` is `234 × 24`. The leading divider is `1 × 24`; the content line box is `(x: 9, y: 5.5, width: 225, height: 13)`, using fixed 13/13 Body typography. Mobile uses the same `234 × 24` geometry, `8` content spacing, and `13px` Body font.
