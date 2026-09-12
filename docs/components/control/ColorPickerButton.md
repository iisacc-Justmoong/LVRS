# ColorPickerButton

Color action trigger from Figma `892:62`. `buttonSize` accepts Small (22), Medium (28, default), or Large (36). `currentColor` defaults to #7A5AF8; `showHueRing` defaults to true. Padding is 4px, radius 4, focused border 2px and disabled opacity 0.32. The ring uses the exact Figma export. The caller opens its picker from `onClicked`.

```qml
import LVRS as LV
LV.ColorPickerButton {
    buttonSize: LV.ColorPickerButton.Small
    currentColor: selectedColor
    onClicked: colorPopup.open()
}
```

Pressable types use the shared AbstractButton compression, rebound and keyboard activation; `Motion.reducedMotion` removes interpolation. See [Figma audit](../../figma-parity.md) for the source nodes, state values and verification scope.
