<a id="card"></a>

# 카드

`LV.Card`는 파일, FilePreview, 폴더, 프로젝트, 장치, 모델, 멤버 및 링크의 8가지 카드 디자인을 구현합니다. `LV.AbstractButton`를 확장하여 `clicked`, 키보드 포커스, `method` 및 `methods` 계약을 유지합니다. `AppCard`는 별도의 범용 제목 컨테이너로 유지됩니다.

```qml
import QtQuick
import LVRS as LV

LV.Card {
    type: LV.Card.File
    size: LV.Card.Large
    detail: LV.Card.Detailed
    previewSource: "file:///photos/coastal-house.png"
    filename: "Coastal house.png" // alias of title
    metadata: "PNG · 1536 × 1024"
    details: "Warm limestone, curved walls and a quiet view of the sea."
    showMenu: true
    onMenuRequested: fileMenu.open()
    onClicked: selectionController.setSelected(filename, selected)
}
```

<a id="types-and-measured-sizes"></a>

## 종류 및 측정 사이즈

| `type` |Figma 노드|기본 논리적 크기|내용|
|---|---|---|---|
| `Card.File` | [852:997](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=852-997) |중형 210 × 240|이미지가 카드를 채웁니다. 캡션이 하단에 유지됩니다.|
| `Card.FilePreview` | [852:993](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=852-993) | 480 × 320 |꾸미지 않은 이미지 미리보기|
| `Card.Folder` | [786:414](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-414) | 256 × 280 |아이콘 행, 요약 및 작업|
| `Card.Project` | [786:566](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-566) | 256 × 280 |마일스톤 진행 상황, 행 및 참가자|
| `Card.Device` | [786:696](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-696) | 256 × 280 |저장소 진행 상황, 연결 및 동기화 세부 정보|
| `Card.Model` | [786:830](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-830) | 256 × 280 |모델 사양 행 및 호환성 요약|
| `Card.Member` | [786:966](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-966) | 256 × 280 |구성원의 역할, 프로젝트, 팀 및 위치|
| `Card.Link` | [786:1084](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=786-1084) | 256 × 280 |도메인 미리보기, 제목, 설명 및 방문 조치|

File은 `Card.Small`(140 × 160), `Card.Medium`(210 × 240), `Card.Large`(360 × 280)를 지원하며 각각 `Card.Brief` 또는 `Card.Detailed`를 사용한다. 이 크기는 2026-09-13에 Figma 노드 `852:997`의 18개 변형 전체에서 측정했으며, 구성요소 설명에는 이전 크기가 남아 있다. 7개 카드 유형 전체에는 기본·호버·선택 시각 상태가 있다. 현재 Theme는 데스크톱과 모바일에서 같은 논리 크기를 사용한다. 명시적인 너비/높이 덮어쓰기는 반응형 동작을 유지한다. 정보 카드는 행을 더 제공하면 암시적 높이가 늘어나며, 소비자는 콘텐츠 최소 높이보다 낮은 높이를 강제해서는 안 된다.

<a id="content-api"></a>

## 콘텐츠 API

|속성|목적|
|---|---|
| `title`, `filename` |카드 제목; 두 항목 모두 동일한 문자열을 다룹니다.|
| `description`, `showDescription` |정보 카드의 지원 라인|
| `details`, `metadata`, `detail` |파일 캡션 본문 및 메타데이터; 상세 텍스트는 Small/Medium/Large 에 따라 2 / 3 / 2 행으로 제한됩니다.|
| `previewSource` |Local, resource, network 또는 이미지 제공자 URL ; `Image.PreserveAspectCrop` ( Figma   FILL )를 사용합니다.|
| `previewComponent` |이미지 교체 옵션 `Component`; 전체 미리보기 영역으로 크기가 조정되었습니다.|
| `previewItem`, `previewStatus` |읽기 전용 현재 이미지/사용자 정의 항목 및 이미지 로딩 상태|
| `asynchronous` |이미지 로딩 플래그, 기본값은 true|
| `iconName`, `iconSource`, `iconComponent` |기본 유형 아이콘을 재정의합니다. 구성 요소/소스/이름이 해당 순서대로 우선 적용됩니다.|
| `statusText`, `statusColor`, `showStatus` |헤더 상태; 색상은 Device/Model/Member 에 따라 Theme.success 로 기본값이 됩니다.|
| `rows` |`{label, value, iconName?, iconSource?}` 의 배열; 레이블과 값은 독립적으로 잘립니다.|
| `progressLabel`, `progressText`, `progress`, `progressColor` |프로젝트/장치 진행률, 표준화된 0–1 및 고정됨 LV.ProgressBar|
| `summary`, `footnote` |하단 본문 컨텍스트 및 바닥글 텍스트|
| `domain`, `previewTitle` |링크 미리보기 내용|
| `showMenu`, `menuAccessibleName` |메뉴 가시성 및 액세스 가능한 이름; 메뉴는 기본적으로 파일에 숨겨져 있습니다.|
| `showAction`, `actionText` |푸터 버튼 가시성/텍스트; 유형에 따라 Open/View/Manage/Use/Profile/Visit 으로 기본값이 됩니다.|

콘텐츠 문자열과 행은 빈 값으로 기본값이 됩니다. 애플리케이션 데이터를 공급하면 라이브러리는 하드코딩된 샘플 사진이나 도메인 기록을 포함하지 않습니다. 카탈로그는 Figma 에서 가져온 정확한 아키텍처, 풍경 및 문서 이미지 자산들을 번들하며, File 과 FilePreview 로 모든 3 를 보여줍니다.

```qml
LV.Card {
    type: LV.Card.Device
    title: device.name
    description: "macOS · Desktop host"
    statusText: device.online ? "Online" : "Offline"
    statusColor: device.online ? LV.Theme.success : LV.Theme.descriptionColor
    progressLabel: "Storage"
    progressText: "128 / 512 GB"
    progress: 128 / 512
    rows: [{label: "Connection", value: "Local network"}]
    summary: "Last synced just now"
    footnote: "384 GB available"
    onActionTriggered: deviceController.manage(device.id)
    onMenuRequested: deviceMenu.open()
}
```

<a id="interaction-and-rendering"></a>

## 상호작용 및 렌더링

`selected` 는 `checked` 의 별칭이며, `selectable` 은 FilePreview 를 제외하고 기본값이 true 입니다. 클릭 또는 Space 가 선택을 토글합니다. 중첩된 버튼은 `menuRequested()` 와 `actionTriggered()` 를 독립적으로 방출하며 카드의 선택/해제 상태를 변경하지 않습니다. 비활성화된 카드는 입력을 억제합니다. `displayState` 는 일반적으로 `Card.Automatic` 로 유지되며, 정적 디자인 미리보기에는 `Card.DefaultState`, `Card.HoverState` 또는 `Card.SelectedState` 를 사용합니다. 이는 외관만 오버라이드하며 애플리케이션 선택 상태는 변경하지 않습니다.

FilePreview 는 캡션, 테두리, 메뉴 또는 선택 마커를 갖지 않습니다. 그 소스는 중앙에서 잘려 나갑니다. 커스텀 미리보기는 표시 표면이며 카드의 메뉴/액션 처리에 상호작용 가능한 액션을 배치합니다. 누락됨/로딩 중/오류 이미지는 파일 이미지 아이콘을 사용하고 캡션을 유지합니다. 네이티브 `Image` 상태는 관찰 가능합니다.

카드는 테마 색상을 사용하고, 12 픽셀 반지름, 1 픽셀 테두리 (선택 시 2 픽셀), 18 픽셀 패딩 (소스 파일 12 픽셀), 그리고 측정된 Pretendard 타이포그래피를 사용합니다. 패딩은 내부 스트로크를 포함하며 Figma 와 일치합니다. 파일 캡션 높이는 간략형 37/39/39 픽셀, 상세형 75/99/81 픽셀입니다. 3스톱 하단 스krim 은 텍스트 대비를 보존합니다. `QtQuick.Effects.MultiEffect` 는 하드웨어 시나리오 그래프에서 미리보기와 scrim 에 둥근 알파 마스크를 적용합니다. 소프트웨어 렌더러 Qt 에서 캔버스는 이미지 URL 를 캐시에 로드하고 동일한 중앙 잘림, 둥근 클립 및 scrim 을 그립니다. 커스텀 미리보기 컴포넌트는 소프트웨어에서 계속 활성 상태이지만 자체 둥근 클립을 제공해야 하며, 하드웨어 렌더러는 임의의 커스텀 컴포넌트를 마스크합니다. 추가 라이브러리가 추가되지 않습니다. 내부 QML 파일은 모듈 루트 리소스エイリア스를 사용하므로 컴파일된, 파일 시스템 및 설치된 임포트는 동일한 URL 을 해결합니다.

<a id="validation"></a>

## 검증

`LVRSTests_card` 는 Qt 테스트를 통해 실제 QML 생성, 포인터/키보드 이벤트 및 Qt 퀵 이미지 캡처를 사용합니다. 그것은 데스크톱과 모바일 테마 타겟의 Figma 크기/상태 행렬, 풀 사이즈 미리보기 및 수정된 파일 크기의 하단 캡션/스크림 기하학, 커스텀 미리보기, 진행률 클램프, 실패한 이미지 복구, 독립적인 중첩 액션 및 둥근 필 렌더링을 포함합니다. 픽셀 캡처는 창 장치 픽셀 비율을 반영하므로 Retina 캡처는 1× 디스플레이와 동일한 논리적 파일 차원과 캡션 scrim 을 검증합니다.

```sh
cmake -S . -B build -DLVRS_BUILD_TESTS=ON -DLVRS_BUILD_EXAMPLES=ON
cmake --build build -j 8
ctest --test-dir build --output-on-failure
LVRS_CARD_CAPTURE_DIR="$PWD/build/card-captures" ctest --test-dir build -R '^LVRSTests_card$' --output-on-failure
```

라이브 8개 디자인 갤러리를 보려면 VisualCatalog에서 **Surfaces → Card**를 엽니다.

또한 기존 설치된 소비자 테스트는 8가지 유형을 모두 인스턴스화하고 내부 QML 리소스 별칭을 포함하여 해당 차원을 확인합니다. macOS에 대한 네이티브 Metal 렌더링 유효성 검사를 위해서는 `QT_QPA_PLATFORM=cocoa`, `QSG_RHI_BACKEND=metal` 및 `DYLD_LIBRARY_PATH="$PWD/build"`와 함께 `build/tests/LVRSTests_card`를 실행하세요.

<a id="shared-motion"></a>

## 공유 모션

카드 변형은 픽셀 단위로 제한되고, 선택 채우기 혼합 및 중첩 작업은 독립적으로 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
