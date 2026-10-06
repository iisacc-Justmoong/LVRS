# DropdownButton

Figma 키보드 포커스 링은 1.5px 기본이며 기본 배율에서 3px 시작 및 11px 반경을 갖습니다. 키를 놓으면 Space와 Enter가 활성화됩니다. 메뉴 콜백과 기존 라벨/아이콘/갈매기 모양 크기는 변경되지 않습니다.

위치: `src/qml/components/control/buttons/DropdownButton.qml`

`DropdownButton`는 `AbstractButton`를 기반으로 구축된 독립 메뉴 트리거 제품군입니다. 이는 2026-09-06에서 측정된 [Figma DropdownButton 700:337](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=700-337)를 구현합니다. `LabelMenuButton` 및 `IconMenuButton`는 레이블 및 아이콘 사전 설정입니다.

## API

- `iconMode: false`는 라벨 + 갈매기 모양을 선택합니다. `true`는 아이콘 + 갈매기 모양을 선택합니다.
- `text`, `tone`, `enabled`, `method`, `methods` 및 `clicked()`는 `AbstractButton` 계약을 유지합니다. `Primary`는 기본 톤입니다.
- 아이콘 입력: `iconSource` / `url`, `iconName`, `icon.name`, `iconGlyph`, 및 `iconSize`. 프로젝트 구조 대체 경로와 톤별 체브론이 재사용됩니다.
- `indicatorSize`, `resolvedIndicatorName`, 및 `renderedIndicatorSource`는 슈퍼샘플링을 포함한 기존 체브론 계약을 노출합니다.
- 호출자는 `method` 또는 `onClicked`를 통해 메뉴를 연다. 버튼은 애플리케이션의 메뉴 모델을 소유하거나 생성하지 않는다.

<a id="geometry"></a>

## 기하학

|변형|데스크탑 크기|패딩 L/T/R/B|간격|반경|
| --- | --- | --- | --- | --- |
|라벨, 텍스트 `Open`| 60 × 22 | 8 / 2 / 2 / 2 | 0 | 8 |
|아이콘| 40 × 22 | 4 / 2 / 2 / 2 | -2 | 8 |

라벨은 ( 8, 4.5 ) 에서 32 × 13 를 차지하며, 그 18 × 18 체vron은 ( 40, 2 ) 에서 시작합니다. 아이콘은 ( 4, 2 ) 에서 18 × 18 를 차지하며, 그 chevron은 ( 20, 2 ) 에서 시작합니다. `rightPadding` 는 `horizontalPadding` 와 독립적으로 `Theme.gap2` 로 기본값을 가지며, 후자는 왼쪽 인셋을 `Theme.gap8` 또는 `Theme.gap4` 로 설정합니다. 간격은 `spacing` 를 통해 계속 오버라이드 가능하며, 반경은 `Theme.radiusMd` 를 사용합니다.

모바일은 동일한 지오메트리와 본문 13px 타이포그래피를 사용합니다: 라벨 60 × 22, 아이콘 40 × 22, 반경 8및 아이콘 갈매기형 간격 -2. 사용자 정의 레이블 버튼 너비가 더 좁아지면 텍스트가 사라지고 갈매기 모양과 오른쪽 삽입이 유지됩니다.

현재 Figma 세트는 악센트, 기본값 및 경계선 없는 참조를 제공합니다. 기존 소비자는 상속된 파괴적인 음색과 장애 행동을 계속 사용할 수 있습니다. 그들은 동일한 측정된 형상을 사용합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.DropdownButton {
    text: "Open"
    method: function(eventData) { menu.open() }
}
LV.DropdownButton { iconMode: true; iconName: "projectStructure" }
```

`menu`는 소비 애플리케이션에 속합니다. 기존 `LabelMenuButton` 및 `IconMenuButton` 가져오기는 업데이트된 형상을 자동으로 상속합니다. 공유 검증은 [PushButton 검증](PushButton.md#validation)를 참조하세요.

<a id="shared-motion"></a>

## 공유 모션

`releaseOnSignal` 는 콘텐츠 모드와 LabelMenuButton / IconMenuButton 프리셋 모두에서 true 로 기본값입니다. 라벨 또는 아이콘, 체vron 및 배경은 `released()` 에서 180ms 탄성 복귀를 수행합니다. 릴리스별 필리 또는 테두리는 없습니다. 기존 호버/프레스 톤, 클리핑, 메뉴 디스패치 및 키보드 초점은 계속 유효합니다. 빠른 탭은 피드백을 받습니다; 취소 및 비활성화/감소된 동작/로컬 옵트아웃은 리바운드를 하지 않습니다. [PushButton 릴리스 검증](PushButton.md#shared-motion) 에서 공유 회귀 회귀 사례를 참조하세요.

완전한 트리거는 하나의 표면으로 리바운드됩니다. 모든 ContextMenu는 독립적으로 애니메이션됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
