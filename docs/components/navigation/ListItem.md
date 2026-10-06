# ListItem

위치: `src/qml/components/navigation/ListItem.qml`

`ListItem`는 Figma 구성요소 세트 `241:9253`의 17 변형을 구현합니다. 기존 LVRS 컨트롤을 구성합니다. 입력, 선택기, 버튼 및 선택 컨트롤은 대화형입니다.

<a id="variants-and-geometry"></a>

## 변형 및 기하학

`type`를 `ListItem` 열거형 값으로 설정합니다. 레거시 `size` 속성 별칭은 `type`입니다. `Mini` 및 `Detail`는 열거형 값과 기본 크기를 유지합니다.

|유형|데스크탑 크기|내용|
| --- | --- | --- |
|미니| 170 × 22 |아이콘 및 라벨; 선택적 레거시 인라인 편집기|
|세부 정보| 194 × 106 |2줄 제목, 북마크, 날짜, 폴더 및 태그|
|내비게이션| 280 × 44 |라벨, 설명, 값 및 공개|
|토글| 280 × 44 |라벨, 설명 및 스위치|
|검사 가능| 280 × 44 |확인란, 레이블, 설명 및 값|
|조치| 400 × 44 |하나의 후행 액션|
| ActionGroup | 400 × 44 |2 작업 및 드롭다운 메뉴|
|스테퍼| 400 × 44 |수량 및 방향 스테퍼|
|선택| 400 × 44 |값 선택기|
| InlineEdit | 400 × 44 |입력 및 저장 동작|
| DetailActions | 400 × 120 |2줄 설명, 북마크, 메타데이터 및 3 작업|
| DetailQuantity | 400 × 118 |형식, 수량, 단위, 스위치 및 동작|
| DetailSettings | 400 × 159 |스위치, 선택기, 섹먼트, 상태 및 2 작업|
|리소스| 400 × 129 |미리보기, 설명, 메타데이터, 메뉴 및 작업|
|미디어| 400 × 113 |미리보기, 지속 시간, 소스 세그먼트 및 재생 작업|
|작업| 400 × 135 |확인란, 우선 순위, 설명, 메타데이터, 추정 및 작업|
|양식| 400 × 154 |선택기, 레이블이 지정된 입력 2개, 스위치, 상태 및 작업|

위의 측정된 크기는 데스크톱 및 모바일의 기본 콘텐츠와 표시되는 슬롯에 적용됩니다. 복합 높이는 콘텐츠와 선택적 슬롯을 따릅니다. 표준 행의 최소 높이는 44px입니다. 테마 기하학과 모든 타이포그래피는 두 대상 모두에서 작성된 값을 유지합니다. 본체는 13px로 남아 있습니다.

2026-09-10 InputField 조정은 InlineEdit 와 폼 입력에 `22px` 높이와 중앙 정렬된 `13px` 라인 박스를 제공합니다. 폼은 2 입력 행을 포함하여 `148px` 에서 `154px` 로 성장하며, 혼합된 목록은 결과 행 위치와 총 콘텐츠 높이를 자동으로 전파합니다.

너비는 부모 레이아웃에 의해 할당될 수 있습니다. 기본 동작은 72px를, 선택자는 97px를, 입력 필드는 140px를, 미리보기는 48px를 예약합니다. 레이블과 값은 할당된 공간 안에 생략됩니다. 이 사전 설정은 나열된 너비를 위해 설계되었으며, 더 큰 사용자 정의 컨트롤은 더 넓은 행 또는 사용자 정의 대리자를 요구합니다.

<a id="content-and-state"></a>

## 내용과 상태

- 공통: `label`, `description`, `iconName` / `iconSource`, `value`, `selected`.
- 세부 정보: `detail`, `dateText`, `folderLabel1/2`, `tagLabel1/2`, `bookmarkIconName`, `folderIconName`, `tagIconName`.
- 화합물: `metadata1`, `metadata2`, `statusText`, `quantityLabel`, `optionLabel`, `modeLabel`, `scopeLabel`, `durationText`, `mediaDetail`, `fieldLabel1/2`.
- 미리보기: 이미지 URL의 경우 `previewSource`, 테마 자산의 경우 `previewIconName`. 아이콘은 48px 타일 내부 중앙에 네이티브 18px 크기로 유지됩니다.
- 선택: `checked`, `toggleEnabled`, `selectionEnabled`. `selected`는 확인란/스위치 값과 관계없이 행 강조 표시를 제어합니다.
- 수량: `quantity`, `minimumQuantity`(0), `maximumQuantity`(무제한), `stepSize`(1). 사용자 편집은 제한되어 있으며 구성된 범위로 고정됩니다.
- 선택기 상태: `selectorIndex`, `unitIndex`(둘 다 0), `segmentIndex`(0).
- 입력 상태: `inputText1`, `inputText2`(둘 다 `"Value"`). 이는 레거시 Mini `inputResult`와는 별개입니다.

선택적 요소는 `showLeadingIcon` , `showDescription` , `showValue` , `showTrailingIcon` , `showBookmark` , `showDate` , `showFolders` , `showTags` , `showMetadata` , `showPrimaryAction` , `showSecondaryAction` , `showMoreMenu` , `showStepper` , `showSelector` , `showUnitSelector` , `showToggle` , `showSelection` , `showSegments` , `showInput1` , `showInput2` , `showPreview` 를 사용합니다. 모두 기본값이 참입니다. 플래그는 해당 요소를 포함하는 변형에만 영향을 미칩니다.

<a id="nested-controls"></a>

## 중첩된 컨트롤

구성 개체는 선택 사항입니다. 지정되지 않은 필드는 미리 설정된 기본값을 유지합니다.

|속성|지원되는 구성|
| --- | --- |
| `primaryAction`, `secondaryAction` | `text`, `tone`, `enabled`, `iconMode`, `iconName`, `method`, `methods`, `onTriggered`, `component` |
| `moreMenu` |동일한 버튼 필드와 ContextMenu `items`|
| `selector`, `unitSelector` |`items`, 대체 경로 `text`, `tone` (ComboBox 열거형), `arrow`(스테퍼 열거형), `enabled`, `component`|
| `stepper` | `tone`, `arrow`, `enabled` |
| `input1`, `input2` |`placeholderText`, `readOnly`, `enabled`, `style` (InputField 열거형), `clearButtonVisible`, `maximumLength`, `validator`, `component`|
| `segments` |문자열 배열 또는 `{ text/label, enabled, method }` 항목|

선택기 옵션은 배열 또는 QML 목록과 유사한 `count/get()` 소스를 허용합니다. 옵션은 문자열 또는 `{label, enabled, ...ContextMenu fields}` 개체일 수 있습니다. 클릭하면 LVRS ContextMenu가 열립니다. 키보드 Up/Down은 선택을 변경하고 Enter/Space는 메뉴를 엽니다. 비활성화된 항목은 건너뜁니다.

수량 스텝은 상반부/하반부를 사용하여 증가/감소합니다. 초점된 수량 컨트롤은 키보드 상/하 방향키도 받습니다. 단일 화살표 스텝은 포인터 입력과 키보드 입력 모두에 한 방향만 있습니다. 비활성화된 스텝은 두 입력 방법을 모두 거부합니다. 텍스트 필드는 LVRS 의 재료 채움, 입력 처리 및 클리어 버튼의 고정된 8px 오른쪽 여백을 유지합니다.

<a id="events-and-methods"></a>

## 이벤트 및 메서드

- `edited(field, value)`는 `checked`, `quantity`, `selectorIndex`, `unitIndex`, `segmentIndex`, `inputText1` 또는 `inputText2`에 대한 사용자 변경 사항을 보고합니다. 직접 속성 할당은 이를 내보내지 않습니다.
- `actionTriggered(action, payload)`는 `primary`, `secondary`, `menu`, `menuItem` 및 `inputSubmitted`를 보고합니다.
- `primaryAction.method` / `secondaryAction.method`는 현재 입력, 선택 및 수량 상태를 포함하는 `action`, `payload` 및 `values`로 보강된 기존 LVRS 메서드 이벤트를 수신합니다. `methods`는 여러 메서드를 호출합니다. `onTriggered`는 ​​`method`의 대안입니다.
- ContextMenu 옵션 콜백은 ContextMenu 이벤트 계약을 유지합니다. `menuItem` 페이로드에는 `{index, item}`가 포함되어 있습니다. `inputSubmitted`에는 `{field, text}`가 포함되어 있습니다.
- 하위 작업은 주변 행을 클릭하지 않습니다. `clicked` 및 상속된 행 `method`는 행 활성화에 계속 사용할 수 있습니다.
- `editValue(field, value)`, `stepQuantity(direction)` 및 `triggerAction(action, payload)`는 사용자 정의 콘텐츠에 대해 동일한 동작을 노출합니다. `typeValue(value)`는 변형 이름 또는 숫자 열거형을 확인합니다.

레거시 Mini `inputable`, `inputResult`, `inputEdited` 및 `inputSubmitted` API는 계속 지원됩니다.

<a id="custom-composition"></a>

## 맞춤 구성

`leadingComponent` 및 `trailingComponent`는 표준 행의 선행 아이콘 또는 전체 후행 클러스터를 대체합니다. `previewComponent`는 리소스/미디어 미리보기를 대체합니다. `footerComponent`는 사전 설정 아래에 콘텐츠를 추가합니다. 버튼, 선택기 및 입력 구성 개체는 `component` 대체품을 제공할 수도 있습니다.

대체품은 자체 암시적 크기를 제공해야 하며 `property var listItem`를 선언하여 소유자를 받을 수 있습니다. 이 속성은 Loader가 생성 후에 제공하므로 선택 사항이어야 합니다. 여기에는 여러 개의 LVRS 컨트롤이 포함될 수 있으며 소유자의 편집/작업 메서드를 호출할 수 있습니다.

```qml
import LVRS 1.0 as LV

LV.ListItem {
    type: LV.ListItem.DetailQuantity
    label: "Export images"
    quantity: 2
    minimumQuantity: 1
    maximumQuantity: 8
    selector: ({ items: ["PNG", "JPEG", "WebP"] })
    unitSelector: ({ items: ["Scale", "Pixels"] })
    primaryAction: ({
        text: "Export",
        method: function(event) { exporter.run(event.values.quantity) }
    })
}
```

<a id="interaction-states"></a>

## 상호작용 상태

각 행은 `interaction` 를 소유하며, 읽기 전용 `interactionPhase` 와 `interactionInput` 는 AbstractButton 에서 상속됩니다. Figma 는 동일한 위상을 가진 완전한 타입별 변형을 사용하며, [컴포넌트 인스턴스 상태](../../instance-states.md)를 참조하십시오. 현대 콘텐츠 클리핑은 콘텐츠 항목 내부에 유지되므로 행 자체에 의해 외부 초점 링이 잘리지 않습니다.

모든 17 타입은 측정된 프레임이나 내장 컨트롤을 변경하지 않고 기본, 호버, 누름, 해제 및 키보드 초점을 공유합니다. 기본은 `listBackgroundColor` 를, 중첩 호버는 `Theme.surfaceAlt` 를, 중첩 누름은 `Theme.accentMuted` 를 사용합니다. 선택된 행은 모든 입력 상태에서 `selectedBackgroundColor` 를 유지합니다. 내장 `checked` 토글 값은 포함 행을 색칠하거나 선택하지 않습니다. 이 색상 속성은 소비자에 의해 재정의 가능합니다.

해제는 실제 해제 이벤트에서 콘텐츠와 표면의 180ms OutBack 반환이며, 독립적인 해제 색상이나 테두리가 없습니다. 취소는 다시 반동하지 않으며, 다시 누르면 반환이 중단됩니다. Space 와 Enter 는 키 릴리스 시 활성화됩니다. Tab 탐색은 기존 행 및 액세서리 순서를 따르며, 행의 1.5px 주 초점 고리는 누름/릴리스 동안 고정 범위를 벗어난 3px 위치로 유지됩니다. 비활성화된 행은 활성화하거나 초점을 받을 수 없습니다. 임베디드 버튼, 입력 필드, 선택기 및 토글은 자체 초점, 편집 및 동작 이벤트를 유지합니다.

<a id="verification"></a>

## 검증

`LVRSTests_motion`는 행 색상, 포커스 유지 및 고정 경계, 포인터/키보드 리바운드 및 Enter 취소에 대한 모든 17 사전 설정을 확인합니다.

`LVRSTests_list_composites` 는 Qt Test 와 Qt Quick 을 사용합니다. 모든 사전 정의된 차원, 데스크톱/모바일 정책, 높이 혼합 목록, 포인터/키보드 편집, 버튼 콜백, 팝업 선택, 대리자 신원, 동적 타입 변경, 고정 푸터, 사용자 정의 후속 컴포지션 및 모든 사전 정의된 항목의 실제 렌더링을 확인합니다. `LVRS_LIST_CAPTURE_DIR` 를 `build/` 하에 있는 디렉토리로 설정하여 `listitem-presets.png` 를 저장합니다. 기존 Mini/Detail 및 모델 계약은 `LVRSTests_import_api` 와 `LVRSTests_platform_integration` 에 의해 계속 포함됩니다.

`cmake --build build`를 사용한 다음 `ctest --test-dir build --output-on-failure`를 사용하여 빌드하고 실행합니다. LVRS 재정의가 설치된 개발 시스템에서 테스트 프로세스가 `build/libLVRS`(macOS: `DYLD_LIBRARY_PATH="$PWD/build"`)를 로드하는지 확인합니다.
