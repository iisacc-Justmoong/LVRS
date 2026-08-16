# Label

Location: `qml/components/control/display/Label.qml`

`Label` is the LVRS text wrapper that maps style tokens to typography metrics.

## Purpose

- Provide consistent typographic style mapping from `Theme`.
- Expose text-related `Text` APIs through aliases.
- Validate style compliance through debug warning hook.

## Style Constants

- `title`
- `title2`
- `header`
- `header2`
- `body`
- `description`
- `caption`
- `disabled`

## Core API

- `style`
- `sizeToContentHeight` (default `false`; enables natural wrapped-content measurement)
- `text`, `color`, `font`
- `elide`, `wrapMode`
- `horizontalAlignment`, `verticalAlignment`
- `lineHeight`, `lineHeightMode`, read-only `contentHeight`, read-only `lineCount`
- `maximumLineCount`, `fontSizeMode`
- `textFormat`, `linkColor`

Computed style outputs:

- `resolvedStyleColor`
- `stylePixelSize`
- `styleWeight`
- `styleName`
- `styleLineHeight`
- `styleLetterSpacing`

## Usage

```qml
import LVRS 1.0 as LV

LV.Label {
    text: "Status"
    style: body
}
```

## How It Works

- Style enum maps to `Theme` token groups for color, weight, size, line height, and style name.
- Internal `Text` item is the rendering source of truth.
- On style changes, non-compliant text styles can emit debug warning through `Debug.warn`.

## Advanced Example: Two-Line Clamped Description

```qml
import LVRS 1.0 as LV

LV.Label {
    width: 280
    style: description
    wrapMode: Text.WordWrap
    maximumLineCount: 2
    text: "This text is clamped to two lines and then elided by text metrics."
}
```

## Practical Tip

Use semantic `style` constants instead of hard-coded font metrics for maintainability.

## FAQ

Q. Should text style be configured by custom font overrides or style constants?  
A. Prefer style constants first, then apply minimal overrides only when unavoidable.
