<a id="figma-parity-audit--2026-09-12"></a>

# Figma 패리티 감사 — 2026-09-12

이후 사용자 지향적 수정: WindowMaterial 와 ApplicationWindow 는 이제 50% 에서 균일한 #0B0B0B 채색을 사용하며 그라디언트를 사용하지 않습니다. 현재 윈도우 처리는 [마테리얼 계약](components/surfaces/Materials.md)을 따릅니다. 메뉴 수정: ContextMenu 와 메뉴는 이제 더 가벼운 WindowMaterial 코팅 ( 12% 틴트 / 64px 블러 / 8% 추가 강조 강도) 을 사용합니다. 아래 Figma 재고는 역사적 소스 스냅샷이며, 현재 메뉴 코팅은 [수정된 계약](components/navigation/ContextMenu.md#window-derived-frosted-menu)을 따릅니다.

기준 원본: [레이어드 비주얼 렌더 시스템](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System). 감사에서는 현재 구성 요소와 바인딩된 변수를 읽습니다. Figma는 변경되지 않습니다.

카드 개정(2026-09-13): 파일 노드 `852:997`는 이제 전체에 걸쳐 소형 140 × 160, 중형 210 × 240및 대형 360 × 280를 측정합니다. 18 변형. 현재
[Card contract](components/surfaces/Card.md)는 아래 인벤토리의 과거 파일 크기를 대체합니다.

[가족 인벤토리](figma-family-inventory.json) 는 65 컴포넌트 가족과 그 변형체의 루트 기하 구조, 페인트 및 효과를 기록합니다. 감사 또한 21 색상 변수, 재료/경고/카드/ ListItem /슬라이더/ ColorPicker /시트 변수, 모든 2,349 아이콘 세트 컴포넌트 이름, 그리고 관련 하위 노드 타이포그래피 및 간격을 읽습니다. 이는 모든 아이콘이나 모든 애플리케이션 화면의 픽셀 비교가 아닌 구조적이고 표적화된 시각 감사입니다.

<a id="corrections"></a>

## 수정

| Figma |소스 변경|작성된 값|
| --- | --- | --- |
| HelpButton `44:819` |새로운 `HelpButton`|21 × 21, 반경 100, 수평 패딩 4, 수직 패딩 2, Pretendard SemiBold 12 물음표 텍스트, 설명 색상, panelBackground04|
| ColorPickerButton `892:62` |새로운 `ColorPickerButton`|22 / 28 / 36, 패딩 4, 반경 4; 기본 panel06, hover panel12, 눌려진 panel03; 0.5px 흰색 20% 테두리, 초점 2px 기본 테두리, 불투명도 비활성화 0.32|
|색상 웰 `892:4`|정확하게 내보낸 PNG 채도 고리|320px는 20px 벡터 링의 내보내기이며, 중앙은 `currentColor`를 따릅니다. 기본값은 #7A5AF8입니다.|
| ContextMenuItem `331:9282` |새로운 컴팩트 행; ContextMenu는 기본적으로|141 × 18, 수평 패딩 4, 수직 패딩 0, 반경 0를 사용합니다. Inter 일반 12 화이트 라벨 및 Pretendard SemiBold 12 설명 바로가기|
| ContextMenuDivider `331:9283` |새 컴팩트 구분선|145 × 3 독립형, 4px 라인 인셋, 1px 라인, 흰색 30%|
| ContextMenu `331:9332` |수정된 컨테이너 및 너비 프로브|141px 최소 콘텐츠 너비 + 각 측면의 8px 패딩; 8px 수직 패딩; 5개 행 참조 + 나누기 = 157 × 119|
|메뉴 `110:857`|새로운 일반 메뉴 유형|24px 행, 145px 콘텐츠 너비, 패딩 8 가로 / 4 세로; 8개 행 + 2개의 구분선 참조 = 161 × 224|
| ListFooter `209:9199` |드롭다운 버튼의 비대칭 패딩을 유지했습니다|88 × 26, 슬롯 22 / 22 / 40, 메뉴 패딩 4 왼쪽 / 2 오른쪽; 반지름 8 ; 명시적인 슬롯별 오버라이드가 계속 지원됩니다|
|색상 변수|Figma 부동소수점 RGB 및 알파 값을 유지했습니다|모든 12 패널 색상과 5 흰색 텍스트 알파 토큰; panelBackground04 는 #191919 대신 #181919 로 표시됩니다|
|아이콘 세트 이름|Case-safe 별칭과 신선한 Figma SVG 내보내기를 위한 nodesTest / wechat|nodesTest → nodestest, wechat → weChat ; “ volume” 의 앞쪽 공백은 이미 정규화됩니다|
|재료 확산 `975:5`, `975:9`|누락된 삽입 강조 표시|흰색, 오프셋(0, 1), 흐림 0를 추가했습니다. 고밀도 6%, 유리 14%; 기존 64 / 16px 흐림 및 40 / 11% 방사형 값 유지|

5개의 새로운 공개 유형은 모두 기존 LVRS 모션 정책을 상속합니다. 상태 변경 및 조치는 즉시 유지됩니다. 프레젠테이션만 보간됩니다. ColorPickerButton는 작업 트리거입니다. 소비자는 색상 선택 팝업 또는 기타 작업을 소유합니다. 새로운 애니메이션 패키지는 필요하지 않습니다.

<a id="family-coverage"></a>

## 가족 보장

Alert, ApplicationWindow, Button, Card, Checkbox, ColorPicker, ComboBox, ContextMenu, Hierarchy, Input, List, Menus, Popover, Radio, SegmentedControl, Sheet, Slider, Stepper, Table, Toggle 및 Tooltip 은 목록에 포함되었습니다 기존 Card, ListItem, Slider, Sheet 및 input 변형 API 는 포착된 루트 지표와 기존 런타임 계약과 대조되었습니다 일반 호스트의 콘텐츠 기반 너비/높이는 필수 고정 윈도우 크기가 아닌 참조 예시입니다 내부 중첩 변형 집합과 Sheet 데모 콘텐츠는 추가적인 공개 UI 유형이 아닙니다

아바타 페이지 `786:213` 에는 자식 노드가 없습니다. 구현할 작성된 값은 없습니다. 아이콘 목록 커버리지는 이름 분해 확인을 검증하며, 2 는 명시적으로 내보낸 글리프와 색조 고리를 기록된 SHA-256 값과 자산 동일성을 위해 대조합니다. Figma GLASS 렌더러와 Qt 배경 흐림은 다른 렌더러이며, 네이티브 포착은 광학 흐림의 픽셀 동일성을 주장하지 않고 LVRS 동작을 검증합니다

<a id="dependency-and-licensing"></a>

## 종속성 및 라이센스

ContextMenu 레이블은 명시적으로 Inter Regular 를 지정합니다 LVRS 번들은 [Inter v4.1](https://github.com/rsms/inter/releases/tag/v4.1)에서 필요한 정적 Regular 면만 포함하며, 411,640 바이트와 기존 Pretendard 가족과 함께 있습니다 그것의 [SIL 오픈 폰트 라이선스](../resources/font/Inter-OFL.txt) 는 리소스 번들에 포함됩니다 그것은 런타임 패키지나 네트워크 요구사항을 추가하지 않습니다 애플리케이션 전체 폰트 선호도는 Pretendard 로 유지되며, Inter 는 컴팩트 메뉴 레이블에 의해 선택됩니다

<a id="verification"></a>

## 검증

`tests/fixtures/figma-contract.json`는 구현과 독립적으로 검사한 변수와 구성요소 노드 값을 저장한다. `LVRSTests_figma_parity`는 정확한 색상·별칭 해석·5개 구성요소 크기·두 메뉴 밀도와 글꼴·입력 활성화·비활성/포커스/색상 상태·ListFooter 덮어쓰기·네이티브 내부 강조 렌더링을 다룬다. `LVRSTests_catalog`는 소스 유형의 포함 범위를 검사하고 모든 상세 프리뷰를 로딩한다. 가져오기 API 스위트는 일반 메뉴의 기존 동작을 올바른 새 이름인 Menu로 유지하면서 간결한 메뉴 검사도 추가한다.

VisualCatalog 는 HelpButton, ColorPickerButton, ContextMenuItem, ContextMenuDivider 및 메뉴를 포함하며, 지침과 라이브 상호작용을 제공합니다. 이 항목들을 선택하여 수정된 치수와 운동을 검사하고, “Open ContextMenu” 와 “Open Menu” 를 사용하여 2 팝업 밀도를 비교합니다.

macOS 에서 Qt 6.8.3 로 검증되었으며, 전체 LVRS CTest 53/53 가 통과되었고, 모든 84 카탈로그 항목이 로드되었으며, 네이티브 Figma 동등성 12/12 가 통과했습니다(렌더링된 인셋 하이라이트 포함). 기존 메뉴는 렌더링된 텍스트와 공유된 TextMetrics 폰트 바인딩을 통해 기존 33px 자연 레이블 지표를 유지합니다. 네이티브 카탈로그 커버는 HelpButton, ColorPickerButton, ContextMenuItem 및 메뉴를 포함합니다.

설치된 소비자 검증: 단계별 라이브러리는 모든 83 현재 QML 리소스와 일치합니다. 해당 패키지에 대해 다시 빌드한 후 Society 16/16 및 Dreamscapes 5/5 CTest 가 통과했습니다. 두 macOS 번들이 재시작되었으며, 그들의 라이브 로드된 LVRS 경로가 검증되었고, Dreamscapes 는 새로운 컴팩트 ContextMenuItem 를 표시했습니다.

<a id="button--menu--list-interaction-states"></a>

## 버튼/메뉴/목록 상호작용 상태

2026-09-30 버전은 Button/Menu/List/ ContextMenu /Hierarchy 헬퍼 오버레이를 완전한 컴포넌트 변형으로 대체합니다. 모든 6 가족은 이제 `Interaction state=default/hover/press/release/focus` ; 행 집합에 별도의 포인터/키보드 입력 축을 갖습니다. 소비자 마이그레이션 후 5 이전 보조 집합이 제거되었습니다. [인스턴스 상태 계약](instance-states.md) 는 25 집합, 740 변형, 검토 프레임 및 감사 범위를 기록합니다. ContextMenuItem 는 컴팩트한 MenuItem 런타임 계약을 상속합니다. 코드는 네이티브 입력/모델 API 를 유지하며, 모든 17 ListItem 프리셋, 원본 자산 및 차원을 유지합니다. 중립 메뉴/목록 호버는 surfaceAlt 이며 누르는 것은 accentMuted 입니다; 선택 채움은 변하지 않습니다. 초점 윤곽선은 1.5px 기본값으로 3px 밖으로 나옵니다.

사용자의 최종 릴리스 사양은 이전 버튼 Figma 프로토타입의 릴리스 에지/120ms 처리를 대체합니다: 프로덕션은 genuine 포인터, 터치, 스페이스 또는 엔터 릴리스에서 기하학만 180ms OutBack 를 사용하며 새 채움/테두리가 없습니다. Figma 의 키 다운 미리보기는 코드에서 실제 키 업 활성화로 구현됩니다. 취소, 재 누르기, 감소된 동작 및 비활성화 동작은 `LVRSTests_motion` 에 의해 처리되며; 별도의 목록/메뉴 스위트는 콘텐츠 및 보조 계약을 보호합니다.

`LVRSTests_instance_state`는 수정된 계약에 대해 인스턴스별 소유권, 모든 17 ListItem 입력 수명 주기, 취소 및 모델 상태 독립성을 확인합니다. 날짜가 지정된 Figma 픽스처는 런타임 테스트와 별도로 설계 감사를 기록합니다.
