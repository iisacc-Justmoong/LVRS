# MenuDivider

위치: `src/qml/components/navigation/MenuDivider.qml`

`MenuDivider`는 메뉴 그룹 사이에 사용되는 1축 구분선입니다.

<a id="purpose"></a>

## 목적

- 수평 또는 수직이 될 수 있는 최소한의 구분선을 제공하십시오.
- 단일 축 입력으로 구분 기호 렌더링을 결정적으로 유지합니다.

## API

- `axis`: `"horizontal"` 또는 `"vertical"`(기본값: `"horizontal"`)
- `dividerColor`(기본값: `Theme.contextMenuDivider` = `Theme.panelBackground08`)
- `thickness`(기본값: `1`)
- `crossPadding`(기본값: `1`)
- `linePadding`(기본값: `0`)
- `lineLength`(기본값: `220`)

계산됨:

- `verticalAxis`(읽기 전용)

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.MenuDivider {
    axis: "horizontal"
}
```

```qml
import LVRS 1.0 as LV

LV.MenuDivider {
    axis: "vertical"
    lineLength: 80
}
```

<a id="how-it-works"></a>

## 동작 원리

- `axis`는 대소문자를 구분하지 않고 정규화됩니다.
- 수평 모드는 `220 x 3`로 확인되고 `x=0, y=1`에 중심 `220 x 1` 규칙을 그립니다.
- 수직 모드는 교환된 축과 동일한 패딩 계약을 적용합니다.
- `crossPadding`는 교차축에 규칙 중심을 맞춥니다. `linePadding`는 계속 사용자 정의 가능하지만 현재 Figma 구분선이 할당된 전체 너비에 걸쳐 있으므로 기본값은 `0`입니다.
- Figma 소스: 노드 `110:853`(`MenuDivider`). `ContextMenu`로 늘어난 구분선은 메뉴 콘텐츠 너비(`145px`)를 사용합니다.

<a id="practical-notes"></a>

## 실용적인 참고 사항

- 방향을 기본 제어로 선택하려면 `axis`만 사용하십시오.
- 다른 구분 기호가 명시적으로 필요하지 않은 한 Figma 패리티에 대해 `thickness: 1`, `crossPadding: 1` 및 `linePadding: 0`를 유지합니다.

<a id="shared-motion"></a>

## 공유 모션

구분선은 표면 표현에 참여하고 부드러운 불투명도 변경을 지원합니다. 아무런 조치도 취하지 않습니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

일반 분배기는 `Theme.menuDivider`(panelBackground08)를 소비합니다. ContextMenuDivider는 4px 삽입이 포함된 30% 흰색을 별도로 사용합니다.
