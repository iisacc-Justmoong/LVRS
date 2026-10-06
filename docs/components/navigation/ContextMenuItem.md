# ContextMenuItem

Figma 에서 `331:9282` 의 컴팩트 행이 141 × 18px, 가로 패딩 4, 세로 패딩 0, 반지름 0입니다. MenuItem 신호, 아이콘/단축키 레이아웃, 선택 및 서브메뉴 API 를 공유합니다. 레이블은 번들 Inter Regular 12 를 사용하며 단축키는 Pretendard SemiBold 12를 사용합니다. `hasChildItems` 기본값은 false 입니다.

Figma의 `ContextMenuItem` 세트에는 이제 완전한 구성 요소 입력 상태 변형이 포함됩니다. 런타임는 하나의 소유된 `interaction` 개체와 읽기 전용 `interactionPhase`/`interactionInput`를 상속합니다. 외부 상호 작용 오버레이가 필요하지 않습니다. [구성 요소 인스턴스 상태](../../instance-states.md)를 참조하세요.

```qml
import LVRS as LV
LV.ContextMenuItem { label: "Label"; key: "Key" }
```

Pressable 유형은 공유 AbstractButton 압축, 리바운드 및 키보드 활성화를 사용합니다. `Motion.reducedMotion`는 보간을 제거합니다. 소스 노드, 상태 값 및 검증 범위는 [Figma audit](../../figma-parity.md)를 참조하세요.

컴팩트 행은 MenuItem 기본/호버/누름/릴리스/초점 계약을 상속받습니다: 중립적 호버는 `Theme.surfaceAlt` , 중립적 누름은 `Theme.accentMuted` , 선택된 채우기는 Primary 를 유지하며 비활성화 상태는 활성화 또는 초점이 없습니다. 릴리스는 180ms 기하학적 OutBack 반환입니다. 키보드 링은 141 × 18 프레임 바깥에 3px 유지됩니다. 레이블 및 단축키 타이포그래피는 변경되지 않았습니다.
