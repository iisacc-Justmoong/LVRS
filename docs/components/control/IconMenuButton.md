# IconMenuButton

Location: `qml/components/control/buttons/IconMenuButton.qml`

`IconMenuButton` combines a main icon and trailing menu indicator chevron.

## Purpose

- Provide compact menu-trigger button for icon-centric surfaces.
- Apply consistent tone-aware policy to both main icon and chevron indicator.

## API

Main icon API:

- `iconSource` (alias)
- `iconName`
- `iconGlyph`
- `iconSize`

Indicator behavior:

- indicator name is computed from tone + effective enabled state
- rendered indicator source uses `SvgManager.icon(...)`

Layout:

- fixed height: `Theme.gap20`
- `horizontalPadding: Theme.gap2`
- `verticalPadding: Theme.gap2`
- `spacing: Theme.gap4`

## Main Icon Resolution Order

1. explicit `iconSource`
2. explicit `iconName`
3. grouped `icon.name`
4. tone/disabled fallback icon

## Usage

```qml
import LVRS 1.0 as LV

LV.IconMenuButton {
    tone: LV.AbstractButton.Default
    iconName: "viewMoreSymbolicDefault"
}
```

## Practical Notes

- `IconMenuButton` is best for compact menu affordances where text labels are unnecessary.
- For text-first menus, use `LabelMenuButton` for clearer affordance.

## FAQ

Q. Which icon wins when both `iconSource` and `iconName` are set?  
A. `iconSource` has higher priority.

Q. Is text label supported?  
A. No explicit text label is part of this component contract. For text + indicator, use `LabelMenuButton`.

## Validation Checklist

- main icon fallback order works for empty/partial inputs,
- indicator icon changes correctly on tone/enable transitions,
- compact geometry remains stable in toolbar rows.
