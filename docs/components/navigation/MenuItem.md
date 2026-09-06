# MenuItem

Location: `qml/components/navigation/MenuItem.qml`

`MenuItem` is a context-menu row component used by `ContextMenu` delegates.

## Purpose

- Render icon/label/key rows with selected/inactive states.
- Control key visibility and chevron visibility independently.
- Drive chevron direction from fold/expand state when direction is `auto`.

## Core API

State constants and property:

- `defaultState`, `selectedState`, `inactiveState`
- `state`

Direction constants and property:

- `directionRight`, `directionLeft`, `directionUp`, `directionDown`
- `selectionDirection` (`int` or string: `auto|right|left|up|down`)

Content:

- canonical inputs: icon string, label string, key visibility boolean, key string
- `label`
- `key` / `shortcut` (alias, default `"key"`)
- `keyVisible` (boolean, default `true`)
- `keyPlaceholder` (default `""`)
- `iconName` (default `"procedure"`), `iconSource`
- `showIconSlot` (boolean, default `true`)
- `showChevron` (default `true`)
- `hasChildItems` (default `true`)
- `expanded`
- `effectiveShowChevron` (readonly: `showChevron && hasChildItems`)

Layout and visuals:

- `itemWidth`, `itemHeight`
- `iconSize`, `chevronSize`
- `iconPlaceholderColor`, `chevronColor`
- resolved: `effectiveShowIconSlot`, `resolvedIconSlotWidth`, `resolvedIconLabelGap`, `resolvedIconSource`, `resolvedShortcutText`, `resolvedSelectionDirection`, `resolvedChevronRotation`, `resolvedBackgroundColor`

## Behavior Contract

- Figma source: node `107:498` (`MenuItem` component set), with the default variant at node `107:495`.
- The standalone defaults are the measured Figma instance: `label: "Label"`, `key: "key"`, `keyVisible: true`, `iconName: "procedure"`, and a visible right chevron. `ContextMenu` model delegates still supply their normalized entry values explicitly.
- Desktop and mobile use the same `161 x 24` frame and `13px` Body typography.
- `selected`/`inactive` states map to different background colors.
- The icon frame is `18 x 18` on desktop and `36 x 36` on mobile. The chevron frame is `16 x 16` on desktop and `32 x 32` on mobile.
- The row uses `4px` horizontal and `3px` vertical padding on desktop and mobile.
- The Figma parity fixture uses `procedure.svg`: icon `x=4, y=3, 18 x 18`, label `x=30, y=5.5, 33 x 13`, shortcut `x=112, y=5.5, 21 x 13`, and `generalchevronRight.svg` at `x=141, y=4, 16 x 16`.
- Label and shortcut both use the fixed Body contract: Pretendard Medium `13px`, `13px` line height, and zero letter spacing. `labelMetricCompensation` remains available for compatibility but resolves to `0`.
- `implicitWidth` expands beyond `itemWidth` when icon/label/key/chevron content requires more space.
- Natural label and shortcut widths are measured independently from the displayed elided text, so constrained rendering does not feed back into `implicitWidth`.
- Row layout is responsive under constrained widths: label and shortcut text elide within the row, trailing metadata stays inside the item frame, and the flexible spacer never resolves to a negative width.
- `showIconSlot: false` removes the left icon slot itself, not just the icon image. The row does not reserve icon width or the icon-to-label gap, and the label starts at the row origin.
- Label and shortcut both use the `body` style token.
- If icon source cannot be resolved, a compact circular accent placeholder is shown.
- Key text is hidden when `keyVisible` is `false`.
- If `keyVisible` is `true` and `key` is empty, `keyPlaceholder` is rendered.
- Chevron is shown only when `effectiveShowChevron` is `true`.
- `selectionDirection: "auto"` maps `expanded=false` to right and `expanded=true` to down.

## Usage

```qml
import LVRS 1.0 as LV

LV.MenuItem {
    label: "Open Recent"
    showIconSlot: false
    key: "Cmd+O"
    keyVisible: true
    hasChildItems: true
    showChevron: true
    expanded: false
    selectionDirection: "auto"
    state: selectedState
}
```
