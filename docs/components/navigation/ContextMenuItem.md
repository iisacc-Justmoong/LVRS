# ContextMenuItem

Compact row from Figma `331:9282`: 141 × 18px, horizontal padding 4, vertical padding 0, radius 0. It shares MenuItem signals, icon/shortcut layout, selection and submenu APIs. The label uses bundled Inter Regular 12; the shortcut uses Pretendard SemiBold 12. `hasChildItems` defaults false.

Figma's `ContextMenuItem` set now contains complete component input-state variants.
The runtime inherits one owned `interaction` object and readonly
`interactionPhase`/`interactionInput`; no external interaction overlay is required.
See [component instance states](../../instance-states.md).

```qml
import LVRS as LV
LV.ContextMenuItem { label: "Label"; key: "Key" }
```

Pressable types use the shared AbstractButton compression, rebound and keyboard activation; `Motion.reducedMotion` removes interpolation. See [Figma audit](../../figma-parity.md) for the source nodes, state values and verification scope.

The compact row inherits the MenuItem default/hover/press/release/focus contract: neutral hover is `Theme.surfaceAlt`, neutral press is `Theme.accentMuted`, selected fill stays Primary, and Inactive has no activation or focus. Release is a 180ms geometry-only OutBack return. The keyboard ring remains 3px outside the 141 × 18 frame; label and shortcut typography are unchanged.
