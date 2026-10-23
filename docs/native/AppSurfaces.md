# 네이티브 AppHeader와 AppCard

두 타입은 iiGUI의 자체 C++ 컨테이너·레이블·버튼과 GPU 패널을 합성한다. Cocoa/Win32
위젯이나 Qt 컨트롤로 대체하지 않는다. 같은 위젯 세대와 기존 LVUI 백엔드 스코프를
사용하며 AppHeader의 기본 자식은 액션 행, AppCard의 기본 자식은 본문 슬롯에 들어간다.

## AppHeader

title·subtitle·menu_visible·compact와 메뉴 콜백, 가로/세로 여백·행/액션 간격·메뉴
버튼 패딩을 제공한다. 기본 가로/세로 여백은 16, compact에서는 10이며 행 간격은
12/8, 액션 간격은 8/6, 메뉴 버튼 패딩은 10/8이다. 명시적인 값은 compact 기본값보다
우선한다. 자연 높이는 자식 행의 높이+32와 최소 56 중 큰 값이다. 실제 자식 여백과
자연 크기 여백을 분리하여 compact에서도 이 높이 계약을 유지한다.

제목은 title2 22·bold·행 높이 22, 부제는 description 12·semibold·행 높이 12이며
부제가 비면 숨긴다. 메뉴는 description 12, 패딩을 포함해 높이 32/28이고 기본 표면과
눌림 표면은 원본 Theme의 surfaceSolid/surfaceAlt이다. 메뉴가 없으면 해당 행 간격도
제외한다. 제목 영역은 남은 폭을 쓰고 사용자 액션은 오른쪽에 배치한다.

set_options·set_title·set_subtitle·set_menu_visible·set_compact·set_enabled·
set_background와 set_menu_clicked는 같은 인스턴스를 갱신한다. compact 전환과 메뉴
패딩 변경도 버튼과 액션 자식을 다시 만들지 않는다. actions_root()와 선택적인
AppHeaderActionFactory를 사용할 수 있다. factory는 처음 구성할 때만 실행한다.

## AppCard

기존 고정 480×320 배경 스택을 제목·부제·구분선·본문 슬롯을 가진 합성 객체로 확장한다.
기본 패딩은 18, 섹션 간격은 10, 둥근 반경은 12이며 자연 폭의 최소는 280이다.
명시적인 width/height가 0이면 자연 크기를 사용한다. 기존 명시 치수·padding·spacing·
corner_radius·background 옵션과 Widgets::app_card 호출은 계속 사용할 수 있다.
기본 인자가 원본의 자동 크기 계약으로 바뀐 점을 기존 고정 치수와 구분한다.

제목은 header 17·semibold·행 높이 17, 부제는 description 12·semibold·행 높이 12이다.
둘 다 비면 헤더와 구분선을 제거한다. 빈 본문은 최소 높이 1이며 기본 빈 카드의 자연
높이는 37, 제목만 있는 경우 75, 제목·부제가 있는 경우 91이다. 본문의 자식은 같은
원점에 배치하므로 행·열 구성에는 HStack/VStack을 사용한다. 원본 Item 슬롯처럼
숨긴 자식도 자연 크기에 참여한다. 원통형은 폭·높이 중 작은 값의 절반을 반경으로 쓴다.

AppCard::create와 선택적인 AppCardContentFactory는 root와 content_root를 제공한다.
구성 후 root에 일반 자식을 붙여도 실제 부모는 본문 슬롯이다. 부모의 사각 clip은
본문에 적용하지만 별도로 소유한 팝업을 카드 내부에 가두지 않는다. 제목·부제·색상·
형태·치수를 바꿔도 사용자 본문·편집 값·위젯 핸들과 스코프를 유지한다.

## LVUI와 수명

```text
import vm Backend
Window {
    VStack {
        AppHeader(title: vm.title, compact: vm.compact, menuVisible: true,
                  onMenuClicked: vm.menu) { LabelButton(text: "Action") }
        AppCard(title: vm.cardTitle, width: vm.width, shapeStyle: .cylinder) {
            VStack { InputField(text: "Retain me") }
        }
    }
}
```

LVUI는 AppHeader/AppCard::create를 직접 호출하는 C++를 생성한다. title·subtitle·
enabled·치수·스타일과 compact·메뉴 상태는 Core Observable이면 기존 BindingScope에
구독하고 plain C++ 멤버이면 구성 시 읽는다. 동적 소스 평가를 사용하지 않는다.
범위를 종료하면 구독도 종료하며 늦은 closed 갱신을 무시한다.

메뉴 콜백은 복사한 뒤 호출하고 소스가 끝나도 후속 내부 접근을 하지 않는다. 팩토리
오류·예외는 바깥 마운트 트랜잭션으로 전파해 기존 뷰를 유지한다. 개별 구성 요소가
이미 추가한 노드만 따로 되돌리는 API는 아니다. 검증 실패는 옵션을 바꾸지 않으며,
유효한 변경 도중 사용자 취소 콜백이 소유자를 종료하면 closed로 반환할 수 있다.
내부 ID는 `<id>/title`, `<id>/subtitle`, `<id>/content` 또는 `<id>/actions` 등을 사용한다.

## 검증 범위와 남은 계약

AppCard/AppHeader 20개·iiGUI 컨테이너 11개·LVUI 생성 1개 검사와
example/NativeAppSurfaces의 별도 Core 앱 예제를 등록한다. 예제는 하나의 스코프,
반응형 compact/제목/형태/치수, 본문 편집 값 유지, 작업자 42·프레임·크기 변경·정상
종료를 포함한다. 모든 앱·테스트·self-test 실행은 사용자 지시에 따라 전체 작업의
마지막 일괄 단계에 모은다. 빌드·계측 빌드 성공은 실행·메모리 안전성 통과가 아니다.

일반 StateColorBehavior와 모션, Theme의 실행 중 전역 배율/테마 교체, 임의 상속
기하 속성·모든 QML signal 순서·전체 플랫폼 접근성/입력 동등성은 남아 있다.
미구현 motionEnabled를 무시해 성공 처리하지 않고 LVUI에서 진단한다. AppCard와
AppHeader는 이 범위까지 부분 구현으로 기록하며 전체 LVRS 완성을 주장하지 않는다.
