# HelpButton

Circular help action from Figma `44:819`. Its 21 × 21px footprint, radius 100, 4/2px padding and SemiBold 12 question mark are fixed authored defaults. `text` can be localized; `Accessible.name` defaults to Help.

```qml
import LVRS as LV
LV.HelpButton { onClicked: helpPopup.open() }
```

Pressable types use the shared AbstractButton compression, rebound and keyboard activation; `Motion.reducedMotion` removes interpolation. See [Figma audit](../../figma-parity.md) for the source nodes, state values and verification scope.
