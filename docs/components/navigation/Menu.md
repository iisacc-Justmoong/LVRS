# Menu

Regular menu from Figma `110:857`. Menu inherits the ContextMenu model, signals, placement, dismissal and frosted WindowMaterial API, with `compactItems: false`, 24px MenuItem rows, 145px minimum content width, 8px horizontal / 4px vertical padding and the panel08 MenuDivider line. Eight reference rows plus two dividers produce 161 × 224px. Use ContextMenu for 18px compact rows.

```qml
import LVRS as LV
LV.Menu {
    items: [{ label: "Open", key: "⌘O" }, { type: "divider" }, { label: "Close" }]
    onItemTriggered: (index, item) => console.log(item.label)
}
```

Pressable types use the shared AbstractButton compression, rebound and keyboard activation; `Motion.reducedMotion` removes interpolation. See [Figma audit](../../figma-parity.md) for the source nodes, state values and verification scope.

The shared menu coating uses 12% tint, 64px blur and 8% additional accent strength. Menu row typography and geometry remain the regular 24px contract. See [ContextMenu](ContextMenu.md#window-derived-frosted-menu).
