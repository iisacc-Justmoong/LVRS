# LabelSegmentedControl

Location: `/Users/ymy/Developer/LVRS/qml/components/control/buttons/LabelSegmentedControl.qml`

`LabelSegmentedControl` is a segmented container that arranges `LabelButton` instances as borderless segments.

## Purpose

- Provide the Figma segmented container shell for text segments.
- Derive segment count and visible labels from child `LabelButton` instances.
- Keep segmented group styling and spacing consistent across 2~7 segment cases.

## API

Container styling:

- `horizontalPadding` (default: `Theme.gap4`)
- `verticalPadding` (default: `Theme.gap4`)
- `spacing` (default: `Theme.gap2`)
- `borderWidth` (default: `2`)
- `cornerRadius` (default: `Theme.radiusMd`)
- `backgroundColor` (default: `Theme.panelBackground08`)
- `borderColor` (default: `Theme.panelBackground12`)

Behavior:

- `forceBorderlessTone` (default: `true`)
- `segmentCount` (readonly, computed from child segment buttons)

Composition:

- `default property alias buttons: segmentRow.data`

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelSegmentedControl {
    LV.LabelButton { text: "Button" }
    LV.LabelButton { text: "Button" }
    LV.LabelButton { text: "Button" }
}
```

## How It Works

- Child items are collected from the internal row.
- Any child exposing `tone` is treated as a segment button.
- When `forceBorderlessTone` is enabled, each segment button tone is synchronized to `LV.AbstractButton.Borderless`.
- Implicit size is derived from row implicit size plus segmented container paddings.

## Practical Notes

- Segment text and segment count should be controlled only by supplied `LabelButton` children.
- Keep segment labels short to preserve compact segmented density.
- Use this component for text toggles/grouped actions that visually share one capsule container.

## Mistake Patterns

- adding non-button visual children and expecting them to participate as segments,
- setting inconsistent paddings/radius per segment instead of using the segmented container shell,
- forcing non-borderless tones while `forceBorderlessTone` is still enabled.

## Validation Checklist

- container fill/border/radius matches segmented design token values,
- all visible segments are rendered from supplied `LabelButton` children,
- segment spacing is constant (`2`) across different segment counts,
- effective button tone is borderless in the default configuration.
