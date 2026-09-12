# ContextMenuItem

Compact row from Figma `331:9282`: 141 × 18px, horizontal padding 4, vertical padding 0, radius 0. It shares MenuItem signals, icon/shortcut layout, selection and submenu APIs. The label uses bundled Inter Regular 12; the shortcut uses Pretendard SemiBold 12. `hasChildItems` defaults false.

```qml
import LVRS as LV
LV.ContextMenuItem { label: "Label"; key: "Key" }
```

Pressable types use the shared AbstractButton compression, rebound and keyboard activation; `Motion.reducedMotion` removes interpolation. See [Figma audit](../../figma-parity.md) for the source nodes, state values and verification scope.
