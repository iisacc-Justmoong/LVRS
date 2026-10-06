# ListFooter

3 동작 슬롯은 기본값으로 Figma 에서 `209:9199` : 88 × 26px, 2px 외부 패딩, 22 / 22 / 40px 슬롯 너비입니다. 아이콘 동작은 2px 가로 패딩을 가지며, 메뉴 동작은 4px 왼쪽과 2px 오른쪽 패딩을 가집니다. 버튼 반지름은 8입니다. 슬롯 구성은 horizontalPadding, leftPadding, rightPadding 및 기타 기존 버튼 값에 대한 최상위 또는 중첩 속성을 지원하며, 명시적인 측면 값이 우선합니다.

```qml
import LVRS as LV
LV.ListFooter {
    button3: ({ type: "menu", iconName: "settings", props: { leftPadding: 4 } })
}
```

Pressable 유형은 공유 AbstractButton 압축, 리바운드 및 키보드 활성화를 사용합니다. `Motion.reducedMotion`는 보간을 제거합니다. 소스 노드, 상태 값 및 검증 범위는 [Figma audit](../../figma-parity.md)를 참조하세요.
