# ListFooter

Three action slots from Figma `209:9199`: 88 × 26px by default, 2px outer padding, 22 / 22 / 40px slot widths. Icon actions have 2px horizontal padding; the menu action has 4px left and 2px right padding. Button radius is 8. Slot configuration supports top-level or nested props for horizontalPadding, leftPadding, rightPadding and other existing button values; explicit side values take precedence.

```qml
import LVRS as LV
LV.ListFooter {
    button3: ({ type: "menu", iconName: "settings", props: { leftPadding: 4 } })
}
```

Pressable types use the shared AbstractButton compression, rebound and keyboard activation; `Motion.reducedMotion` removes interpolation. See [Figma audit](../../figma-parity.md) for the source nodes, state values and verification scope.
