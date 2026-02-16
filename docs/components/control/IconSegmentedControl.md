# IconSegmentedControl

Location: `/Users/ymy/Developer/LVRS/qml/components/control/buttons/IconSegmentedControl.qml`

`IconSegmentedControl` is a segmented container that arranges `IconButton` instances as borderless icon segments.

## Purpose

- Provide the Figma segmented container shell for icon segments.
- Derive segment count and icon content from child `IconButton` instances.
- Keep visual consistency for compact icon-only segmented groups.

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

LV.IconSegmentedControl {
    LV.IconButton { iconName: "projectStructure" }
    LV.IconButton { iconName: "projectStructure" }
    LV.IconButton { iconName: "projectStructure" }
}
```

## How It Works

- Child items are scanned from the internal row and tone-capable children are treated as segment buttons.
- In default mode, each segment button tone is synchronized to `LV.AbstractButton.Borderless`.
- The container computes implicit size from the row and applies segmented paddings/border.
- Icon rendering remains the responsibility of each child `IconButton`.

## Practical Notes

- Use `IconButton` children to decide icon source and segment count.
- Keep icon size/token usage consistent between siblings to avoid uneven rhythm.
- This component is intended for compact icon action groups with a shared segmented background.

## Mistake Patterns

- mixing arbitrary `Item` children and expecting icon segment behavior,
- manually setting inconsistent icon button paddings inside one segmented group,
- disabling `forceBorderlessTone` and unintentionally introducing mixed tone states.

## Validation Checklist

- segmented background and border tokens match design values,
- number of rendered segments equals supplied `IconButton` children,
- inter-segment spacing remains constant (`2`),
- default tone behavior stays borderless for all segment buttons.
