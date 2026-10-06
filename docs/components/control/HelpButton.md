# HelpButton

Figma `44:819`의 순환 도움말 작업입니다. 21 × 21px 설치 공간, 반경 100, 4/2px 패딩 및 SemiBold 12 물음표는 고정된 작성자 기본값입니다. `text`는 현지화될 수 있습니다. `Accessible.name`의 기본값은 도움말입니다.

```qml
import LVRS as LV
LV.HelpButton { onClicked: helpPopup.open() }
```

Pressable 유형은 공유 AbstractButton 압축, 리바운드 및 키보드 활성화를 사용합니다. `Motion.reducedMotion`는 보간을 제거합니다. 소스 노드, 상태 값 및 검증 범위는 [Figma audit](../../figma-parity.md)를 참조하세요.
