# ContextMenuDivider

Compact separator from Figma `331:9283`: standalone 145 × 3px, 1px white line at 30% opacity, 4px line inset and 1px cross padding. It inherits axis, lineLength, linePadding, crossPadding, thickness and motionEnabled from MenuDivider. It has no input target.

```qml
import LVRS as LV
LV.ContextMenuDivider {}
```

Pressable types use the shared AbstractButton compression, rebound and keyboard activation; `Motion.reducedMotion` removes interpolation. See [Figma audit](../../figma-parity.md) for the source nodes, state values and verification scope.
