# ColorPickerButton

색상 액션은 Figma 에서 `892:62` 로 트리거됩니다. `buttonSize` 는 Small (22), Medium (28, 기본값), 또는 Large (36) 를 받습니다. `currentColor` 는 #7A5AF8 로 기본값이며, `showHueRing` 는 true 로 기본값입니다. 패딩은 4px, 반지름은 4, 선택된 테두리는 2px, 비활성화 불투명도는 0.32입니다. 링은 Figma 내보내기를 정확히 사용합니다. 호출자는 `onClicked` 에서 피커를 엽니다.

```qml
import LVRS as LV
LV.ColorPickerButton {
    buttonSize: LV.ColorPickerButton.Small
    currentColor: selectedColor
    onClicked: colorPopup.open()
}
```

Pressable 유형은 공유 AbstractButton 압축, 리바운드 및 키보드 활성화를 사용합니다. `Motion.reducedMotion`는 보간을 제거합니다. 소스 노드, 상태 값 및 검증 범위는 [Figma audit](../../figma-parity.md)를 참조하세요.
