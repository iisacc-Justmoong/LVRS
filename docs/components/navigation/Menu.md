<a id="menu"></a>

# 메뉴

일반 메뉴는 Figma `110:857` 에서 유래합니다. 메뉴는 ContextMenu 모델, 신호, 배치, 해제 및 안개 처리된 WindowMaterial API 를 상속하며, `compactItems: false` 24px MenuItem 행, 145px 최소 콘텐츠 너비, 8px 가로 / 4px 세로 패딩 및 패널08 MenuDivider 선을 포함합니다. 8 참조 행과 2 구분선은 161 × 224px를 생성합니다. ContextMenu 를 18px 컴팩트 행에 사용합니다.

```qml
import LVRS as LV
LV.Menu {
    items: [{ label: "Open", key: "⌘O" }, { type: "divider" }, { label: "Close" }]
    onItemTriggered: (index, item) => console.log(item.label)
}
```

Pressable 유형은 공유 AbstractButton 압축, 리바운드 및 키보드 활성화를 사용합니다. `Motion.reducedMotion`는 보간을 제거합니다. 소스 노드, 상태 값 및 검증 범위는 [Figma audit](../../figma-parity.md)를 참조하세요.

공유 메뉴 코팅은 12% 색조, 64px 흐림 및 8% 추가 악센트 강도를 사용합니다. 메뉴 행 인쇄술과 기하학은 일반 24px 계약으로 유지됩니다. [ContextMenu](ContextMenu.md#window-derived-frosted-menu)를 참조하세요.
