# ContextMenuDivider

Figma 에서 `331:9283` 로 독립적인 145 × 3px, 1px 흰색 선이 30% 투명도, 4px 선 내측 여백 및 1px 가로 패딩을 가집니다. lineLength , linePadding , crossPadding , 두께 및 motionEnabled 를 MenuDivider 에서 상속받습니다. 입력 대상이 없습니다.

```qml
import LVRS as LV
LV.ContextMenuDivider {}
```

Pressable 유형은 공유 AbstractButton 압축, 리바운드 및 키보드 활성화를 사용합니다. `Motion.reducedMotion`는 보간을 제거합니다. 소스 노드, 상태 값 및 검증 범위는 [Figma audit](../../figma-parity.md)를 참조하세요.
