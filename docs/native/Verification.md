# 자체 렌더링 전환 검증

**Hierarchy 데이터·투영 추가 단계:** HierarchyModel과 HierarchyProjection을 Qt 없는
C++23으로 추가했다. 구조 인덱스·요청별 메타데이터·이동 계획/확정과 뷰별 펼침·가변
높이·가시/활성 가능 순번을 제공한다. 모델 37개·투영 32개 검사 소스를 일반/계측 및
재배치한 공개 API 소비자 두 구성에서 컴파일·링크했다. 작업용 설치의 공개 헤더와
각 아카이브 객체, Qt·LVUI 파서·OS 컨트롤 링크, 런타임 검색 경로를 정적으로 검사한다.

이 단계의 기록은 `build/native-hierarchy-model-evidence/`와
`build/native-hierarchy-model-consumer/`이다. 설치 과정에서 갱신될 수 있는 정적
아카이브 컨테이너/인덱스 메타데이터 대신 이름별 객체 본문을 비교한다. 최초의 과도한
컨테이너 전체 바이트 비교 실패도 별도 기록으로 보존한다.

모든 앱·테스트·self-test 실행은 전체 작업의 최종 일괄 단계까지 보류하며 이 단계에서도
실행하지 않는다. Core·LVUI·제공자 내장 SQLite는 sanitizer 계측 범위에 포함하지 않는다.
알고리즘 경계를 실제 레이턴시/RSS 측정 결과로 표시하지 않는다. 실제 HierarchyItem·
HierarchyList 위젯, 선택·입력·드래그·터치·LVUI AOT 연결은 아직 남아 있으며,
[Hierarchy 데이터 계약](Hierarchy.md)과 대응표에서 partial/pending 범위를 구분한다.

**도구 막대 추가 단계:** ListToolbar·ListFooter·HierarchyToolbar·ToolbarButton을 네이티브
객체와 LVUI AOT로 연결한다. 33개 컴포넌트·5개 공통 버튼·AOT 생성 사례와 기존
NativeAppSurfaces 예제의 수명/모델 갱신 검사를 추가했다. 기록은
`build/native-toolbars-evidence/`와 `build/native-toolbars-consumer/`에 보관한다.
모든 앱·테스트·self-test 실행은 전체 작업 최종 일괄 단계까지 보류한다.

**앱 표면 추가 단계:** 고정 배경뿐이던 AppCard를 헤더·본문·자연 크기로 확장하고
AppHeader·메뉴·사용자 액션을 구현했다. 같은 세대의 기본 콘텐츠 슬롯, 실제/implicit
여백·최소 크기와 유지되는 옵션/바인딩을 연결한다. 표면 20개·컨테이너 11개·AOT 1개와
NativeAppSurfaces 예제의 self-test를 등록한다. 기록은 `build/native-app-surfaces-evidence/`와
`build/native-app-surfaces-consumer/`에 보관하며 모든 실제 실행은 최종 일괄 단계로 보류한다.

**방문·뷰 상태 추적 추가 단계:** PageMonitor와 ViewStateTracker를 typed C++ 모델로
구현하고 선택된 Navigator·실제 PageRouter에 연결했다. 모델 29개·Navigator 4개와
실제 라우터 연결 1개 검사 사례를 추가했다. 두 페이지 LVUI 예제는 방문 기록과 논리
뷰 상태를 표시하며 방문 undo가 탐색을 수행하지 않는 경로를 포함한다. 기록은
`build/native-tracking-evidence/`와 `build/native-tracking-consumer/`에 분리한다.
앱·테스트·self-test 실행은 전체 작업의 최종 일괄 단계까지 보류한다.

**Link 합성 추가 단계:** iiGUI ActionSurface·플랫폼 셰이핑 밑줄과 LVRS Link를
구현했다. 텍스트 fallback·자식 컨트롤·패딩·상태 색상·typed 탐색·Core 바인딩을
연결하고 동작 표면 10개·Link 16개·LVUI 생성 1개 사례를 추가했다. 두 페이지 예제의
목적지 변경과 자식 Link도 포함한다. 이 단계 기록은 `build/native-link-evidence/`와
`build/native-link-consumer/`이다. 앱·테스트·self-test 실행은 전체 작업 마지막에
모으며 현재 코드 생성·컴파일·링크·설치 소비자 정적 검증과 구분한다.

**전역 탐색 추가 단계:** NavigationTarget 인터페이스·Navigator 등록 스택·실제 형제
배치·자동 스코프 lease와 공유 도메인을 추가했다. 원본 등록 순서·탐색 위임과
수동 재등록 시 수명을 다루며, 비시각 대상 12개와 실제 라우터 연결 5개의 검증
사례를 추가했다. 두 페이지 AOT 예제도 공유 Navigator를 거친다. 이 단계의 빌드·
패키지·정적 기록은 `build/native-navigator-evidence/`와
`build/native-navigator-consumer/`이다. 모든 테스트·앱·self-test 실행은 전체
작업의 마지막 일괄 단계로 보류한다. Link와 대화형 전환·전체 신호 동등성은
완료로 표시하지 않는다.

**독립 페이지 호스트 추가 단계:** iiGUI content_host, LVUI의 기존 빌더용 `build`
진입점과 LVRS PageRouter를 연결했다. 현재 활성 세대만 유지하며 실패 시 화면/이력을
보존하고 뒤로 가면 새 페이지 스코프를 구성한다. 두 AOT 페이지의 예제와 일반·계측·
설치 소비자 검사의 기록은 `build/native-pages-evidence/`와
`build/native-pages-consumer/`에 분리한다. 각 기록의 status와 실행 여부를 기준으로
판단하며, 소스·패키지 빌드 성공을 실제 실행 통과나 PageRouter 전체 완성으로 해석하지
않는다. 새 content_host 검사 9개·PageRouter 검사 13개·AOT 검사 2개와 페이지 예제의
self-test를 포함하여 테스트/앱 실행은 전체 작업의 최종 일괄 단계까지 보류한다.
Core는 이 작업용 빌드에서 테스트를 끈 상태이며 sanitizer 계측은 iiGUI·코덱·LVRS와
그 소비자 범위이다. Core·LVUI 자체의 sanitizer 실행 통과를 주장하지 않는다.

**탐색 모델 추가 단계:** RouteMatcher·RouteResolver·NavigationStackModel과 LVUI 모델 연결
예제를 추가했다. 빌드·패키지·정적 검사의 기록 위치는 `build/native-navigation-evidence/`와
`build/native-navigation-consumer/`이다. 새 예제의 독립 설치 스크립트도 재배치한 패키지로
빌드·복사만 수행하며 앱이나 테스트를 자동 실행하지 않는다. 경로·이력 표시 예제와
실제 PageRouter 페이지 생성·폐기 호스트의 완성 여부를 구분한다. 새 모델 검사 15개와
예제 self-test, 일반·계측·설치 소비자의 실행은 전체 작업의 최종 일괄 단계까지 유보한다.

**수평 viewport 추가 단계:** 두 축의 논리 범위·원자적 위치·자연 폭 콘텐츠·축별 wheel
bubbling과 Table의 수평 조회·드러내기·편집·hit-test를 추가했다. 일반/계측 빌드와
재배치 소비자의 현재 기록은 `build/native-horizontal-evidence/`와
`build/native-horizontal-consumer/`에 보관한다. 컴파일·정적 검사와 테스트 실행은
구분한다. 이 단계에서도 앱·테스트·self-test 실행은 작업 전체의 최종 일괄 단계까지
유보한다. 이전 단계의 통과 기록은 이 변경의 실행 통과를 증명하지 않는다.

**추가 구축 단계:** 범위 선택·주소/TSV·클립보드·resize·삽입 영역·삭제 메뉴·모델 교체와
통지 중 resize 소유권 반환을 구현했다. 새 화면 14개, 전송 4개, 클립보드 1개 검사와
모델·AOT·hover 검사 변경은 최종 일괄 실행 단계까지 유보한다. 아래 이전 단계의
빌드·실행 기록만으로 이 추가 변경의 실행 통과를 판단하지 않는다.

이 추가 단계의 일반·계측 빌드와 재배치한 소비자 빌드·정적 검사를 완료했다.
현재 기록은 `build/native-table-actions-evidence/final-build-audit.json`과
`build/native-table-actions-consumer/build-only.json`이다. 두 기록의 실제 테스트·앱 실행
여부는 false이며 실제 입력·GPU 픽셀·클립보드·수명·메모리 오류 검사 실행은 대기 상태이다.

**현재 작업 상태:** 테이블 구성 요소·`TableModel`·가상 `Table` 화면이 추가된 뒤의 실행 검증은 작업
마지막의 일괄 단계까지 유보한다. 새 모델·테스트의 컴파일/링크와 아래 과거 실행 결과를
구분해야 한다. 아래 표와 검증 기록은 해당 단계의 결과이며 현재 테이블 변경 전체의
통과를 증명하지 않는다. [TableModel](TableModel.md)과 [Table 화면](TableView.md)에 현재 지원 범위와 미검증 항목을
기록한다. 중간 작업에서는 예제 앱·GUI 테스트를 실행하지 않는다.

## 가상 Table 구축 단계: 빌드와 실행 결과의 구분

`TableModel`에 연결된 가상 `Table`과 `NativeDataTableGallery`의 일반 빌드 및
iiGUI/LVRS의 AddressSanitizer·UndefinedBehaviorSanitizer 빌드를 구성했다. 작업용
경로는 각 모듈의 `build/native-data-table/build`와
`build/native-data-table-sanitizer/build`이다. Core와 LVUI 자체는 해당 sanitizer
계측 범위에 포함하지 않는다. 최신 Core 숫자 처리 헤더를 반영해 다시 빌드했으며,
이 단계에서는 테스트 실행 파일과 예제 앱의 컴파일·링크까지만 수행했다.

`build/native-data-table-consumer/relocated`에 네 모듈과 Vulkan 의존성을 재배치하고
해당 패키지만으로 Gallery, 모델 검사, 화면 검사 실행 파일을 다시 빌드했다.
설치된 Core 앱 진입점의 컴파일, 소스 트리 include 배제, AOT 출력 일치, 정적 라이브러리
객체·공개 헤더·번들 폰트 일치를 검사했다. 정적 바이너리 검사에서 Qt 링크, LVUI 파서
심볼 및 검사 대상 Cocoa 컨트롤 클래스 심볼이 없었으며 런타임 검색 경로는 재배치한
패키지를 참조했다. Core 원본과 설치 헤더 65개도 일치했다. 이 결과는 바이너리의
실제 실행이나 해당 검색 경로에서 라이브러리가 로드된 실행 기록을 의미하지 않는다.

빌드·정적 검사 기록은 `build/native-data-table-evidence/`와
`build/native-data-table-consumer/build-only.json`에 보관한다. 모델·화면·가상 viewport·
AOT·사용자 콜백 재진입·설치 소비자 self-test는 최종 일괄 실행 단계에 대기시킨다.
이 단계의 `tests_executed`와 `gui_apps_launched`는 모두 false이다. 사용자 설치본은
변경하지 않았다. 아래의 과거 실행 결과와 이 기록을 혼합하지 않는다.

macOS arm64, AppleClang 21, C++23에서 검증했다. 이전 AppKit 컨트롤 기반 결과와
이번 iiGUI 자체 위젯 결과는 서로 다른 구현 시점의 기록이다.

| 검사 | 결과 | 기록 |
|---|---:|---|
| iiSDKCore 전체, 직전 프레임 단계 기록 | 55/55 통과 | `iiSDKCore/build/display-frame-full-tests.xml` |
| iiSDKCore 프레임·앱·시간·런타임, 표시 주기 추가 검증 | 13/13 통과 | `iiSDKCore/build/frame-cadence-question-tests.xml` |
| 같은 재배치 패키지의 Core 프레임 설치 소비자 | 1/1 통과 | `iiSDKCore/build/frame-cadence-consumer/build/tests.xml` |
| Core 앱 프레임·주기·예제, 직전 단계 | 3/3 통과 | `iiSDKCore/build/timer-frame-review.xml` |
| Core 프레임 주기, 직전 질의 재확인 | 1/1 통과 | `iiSDKCore/build/timer-frame-current-tests.xml` |
| iiGUI 표시 프레임, 직전 질의 재확인 | 4/4 통과 | `iiGUI/build/timer-frame-current-tests.xml` |
| Core 앱 수명·프레임, 10월 23일 질의 재확인 | 7/7 통과 | `iiSDKCore/build/frame-cadence-current-tests.xml` |
| iiGUI 표시 프레임, 10월 23일 질의 재확인 | 4/4 통과 | `iiGUI/build/frame-cadence-current-tests.xml` |
| iiGUI 전체 | 114/114 통과 | `iiGUI/build/alerts-full-tests.xml` |
| LVUI 전체·인벤토리 | 30/30 통과 | `LVUI/build/alerts-full-tests.xml` |
| LVRS 네이티브 | 30/30 통과 | `LVRS/build/native/build/alerts-full-tests.xml` |
| 자체 위젯·Alert 기초·GPU 흐림·콘텐츠·앱·코덱 ASan/UBSan | 85/85 통과 | `iiGUI/build/aot-sanitizer/build/alerts-sanitizer-tests.xml` |
| LVRS Alert·Modal·선택기·콘텐츠·갤러리 ASan/UBSan | 25/25 통과 | `LVRS/build/native-sanitizer/build/alerts-sanitizer-tests.xml` |
| iiGUI 설치 소비자, 직전 색상 단계 | 1/1 통과 | `iiGUI/build/self-widget-consumer-colors/tests.xml` |
| LVRS 기본 갤러리 설치 소비자, 직전 색상 단계 | 1/1 통과 | `LVRS/build/native-consumer-colors/tests.xml` |
| LVRS 색상 갤러리 설치 소비자, 직전 색상 단계 | 1/1 통과 | `LVRS/build/native-color-consumer-final/tests.xml` |
| LVRS 합성 선택기 설치 소비자, 직전 단계 | 1/1 통과 | `LVRS/build/native-picker-consumer-final/tests.xml` |
| LVRS 팝오버 설치 소비자, 직전 단계 | 1/1 통과 | `LVRS/build/native-popover-consumer/tests.xml` |
| LVRS 색상 버튼·팝오버 설치 소비자, 직전 단계 | 1/1 통과 | `LVRS/build/native-color-button-consumer/tests.xml` |
| LVRS 활성 콘텐츠 설치 소비자, 직전 단계 | 1/1 통과 | `LVRS/build/native-activation-consumer/tests.xml` |
| LVRS Modal 설치 소비자 | 1/1 통과 | `LVRS/build/native-modal-consumer/tests.xml` |
| LVRS Alert 최종 설치 소비자 | 1/1 통과 | `LVRS/build/native-alert-consumer-final/tests.xml` |

최종 검사에서 실패·건너뜀이 0개이다. ASan/UBSan은 iiGUI·LVRS 합성 구현·해당 소비자·SVG 코덱에 적용했으며
별도로 설치된 Core 동적 라이브러리 전체를 계측했다는 뜻은 아니다. PNG 전용 코덱도 이번 계측에 포함했다.
Core 런타임 구현 자체는 이번 Alert 추가에서 변경하지 않았다. Application·FrameLoop 소스 네 파일의
해시가 직전 검증 기록과 같음을 확인했으며, 이번 iiGUI·LVRS 통합과 설치 소비자에서 같은
Core 패키지를 다시 사용했다. 직전 네 컨트롤 단계의 갤러리 3회 반복 실행 기록은
`build/native/build/compound-milestone-tests.xml`에 별도로 보존하며 이번 메뉴 검사 횟수에 합산하지 않는다.

## 실제 검사한 경계

- 창 콘텐츠 트리에서 OS 컨트롤과 OS 스택 레이아웃이 없음을 검사했다.
- Cocoa 마우스·키 이벤트를 단일 표면으로 전달해 C++ 위젯의 행동을 검사했다.
- 가상 접근성 객체의 행동·입력 값·포커스와 OS 키 입력을 검사했다.
- 자체 버튼의 기본·비활성 상태를 GPU 픽셀로 확인했다.
- 클리핑·마스크·그라데이션·이미지의 실제 픽셀과 선형 합성, Metal/Vulkan 오버레이 순서를 검사했다.
- 원본 SVG 2,860개와 별칭 3,169개 이름 중 3,168개 조회를 실제 래스터화했다.
  필터가 필요한 `checkboxCheckedDisabled` 1개는 명시적 미지원이다.
- 포인터 캡처 취소, Tab·Space, 체크·라디오·스위치, UTF-8 삭제,
  IME 미리 편집·확정, 범위 입력·스냅·진행률, 동일 장면 재사용을 검사했다.
- ComboBox의 97×20 크기와 hover 픽셀, Stepper의 18×18 크기·위/아래 절반·단방향·방향키·
  접근성 증가/감소와 눌림 색상, disabled 톤 유지, 누름/해제/클릭/방향 콜백 순서를 검사했다.
- 세그먼트 행의 개수·여백·간격·borderless 적용·테두리/채움 픽셀과 독립 자식 동작을 검사했다.
- Menu·ContextMenu와 각각의 항목·구분선 여섯 타입을 실제 C++ 위젯으로 생성했다.
  일반 24 단위·컨텍스트 18 단위 항목과 Pretendard·Inter 글꼴, 정적 문자열 모델과
  `targetId` 연결을 LVUI AOT 및 실제 갤러리에서 검사했다.
- 팝업의 일반 레이아웃 제외·가장자리 배치·중첩·외부 클릭 차단·비활성 항목·방향키·
  이전 포커스 복귀·150개 항목 스크롤·원래 모델 인덱스·클리핑을 검사했다.
- Metal/Vulkan의 메뉴 오버레이와 하드웨어 클리핑 픽셀, 클리핑만 바꿀 때 전경 텍스처 재사용을
  검사했다. D3D11의 클리핑은 소스에 반영했으나 이 Mac에서 실행 검증하지 않았다.
- 실제 Cocoa 오른쪽 마우스 이벤트, 메뉴의 가상 접근성 자식·키보드 포커스·행동·닫은 뒤
  원래 접근성 요소 재연결과 남은 IME 미리 편집이 Escape를 삼키지 않는 동작을 검사했다.
- 해제 콜백에서 자기 트리를 제거하거나 포커스를 옮겨도 이전 세대의 클릭을 실행하지 않음을 검사했다.
- Core 작업자 결과·타이머·크기 변경·정상 종료·스코프 해제를 검사했다.
- Core 앱 소유 FrameLoop와 실제 CADisplayLink 연결, 고정·가변 업데이트·시간 배율·종료를 검사했다.
- 프레임 콜백 24회 동안 위젯 표시 제출은 8회였으며, 변경하지 않은 프레임의 중복 제출을 억제했다.
- 최종 Mach-O에서 Qt 링크, LVUI 파서 심볼, Cocoa 컨트롤 클래스 심볼이 없음을 검사했다.

## 설치와 소스 없는 실행

Core·LVUI·iiGUI·LVRS를 별도 경로로 설치·재배치한 뒤 소비자를 빌드했다.
소비자는 설치된 Core 진입점을 실제 컴파일하며, 다섯 글꼴의 번들 해시를 검사한다.
실행 중에는 소비자 소스, 설치된 예제 소스, 생성 C++ 헤더, LVUI 컴파일러와
Core 진입점 소스를 모두 사용할 수 없게 했다. 테스트 후에는 해당 테스트 소유 파일을 복구했다.
기본 `~/.local` 설치본은 교체하지 않았다.

첫 설치 갤러리는 650 ms 안에 모든 비동기 완료 조건이 모였는지 검사하는 지점에서 실패했다.
해당 기록은 `build/native-consumer-compound/runtime.log`에 남겼다. 고정 지연을 제거하고
작업자 결과·크기 변경·세 번째 실제 표시 프레임을 모두 관측한 뒤 종료하도록 수정했다.
5초 watchdog은 누락한 조건·실제 프레임 수·관측 크기를 진단한다. 수정본의 반복 실행과
새 설치 경로에서의 소스 없는 실행이 모두 통과했다.

메뉴 단계의 최초 전체 iiGUI 검사는 가상 접근성 요소를 재사용할 때 부모가 다시 연결되지 않아
기존 입력란 포커스 조회에서 1개가 실패했다. 재연결을 수정한 뒤 전체 66개와 ASan/UBSan
37개가 통과했다. 최초 기록은 `iiGUI/build/menu-full-tests-before-ax-fix.xml`에 남겼다.
갤러리 통합 검사에서는 프로그래밍 방식의 `activate()`가 포커스도 이동한다고 가정한 검사가
실패했다. 열기 전 항목에 `focus()`를 명시해 문서의 이전 포커스 복귀 계약을 검사하도록
고쳤으며 전체 5개와 새 설치 소비자가 통과했다. 최초 기록은
`build/native/build/menu-tests-before-focus-fix.xml`에 남겼다.

앱은 Core 동적 라이브러리와 기존 네이티브 Vulkan 키트를 참조한다.
이 결과는 독립 배포용 번들·서명·공증까지 완료했다는 뜻은 아니다.

## 실제 열린 화면

초기 자체 위젯 갤러리에서 버튼·체크·라디오·붙여넣기와 슬라이더 `25 → 26`을 확인했다.
아이콘·탭 추가본에서도 아이콘 버튼 콜백과 탭 선택을 확인했다.
이후 갤러리는 200 ms 타이머 대신 Core FrameLoop에 연결했고, 실제 창에서 증가하는
프레임 수·표본 갱신률과 Core 작업자 결과 `42`를 확인했다. 갱신률은 실제 전달받은 프레임
콜백으로 계산하며 표시 장치의 명목 주기나 물리적 GPU 표시 FPS와 구분한다.
프레임 연결 단계의 실행 파일은 `build/native-consumer-frame-final/build/LVRSNativeGallery.app`이었다.
네 가지 컨트롤과 완료 조건 수정을 포함한 직전 실행 파일은
`build/native-consumer-compound-final/build/LVRSNativeGallery.app`이다.
메뉴 단계의 실행 파일은 `build/native-consumer-menu/build/LVRSNativeGallery.app`이었다.
최신 기본 갤러리는 `build/native-consumer-colors/build/LVRSNativeGallery.app`, 색상 갤러리는
`build/native-color-consumer-final/build/LVRSNativeColorGallery.app`이다.
중복 제출 정리 전의 실제 창에서는 `update: 56 Hz`, 정리 후 최종 창에서는 증가하는
프레임 수와 `update: 60 Hz`를 관측했다. 짧은 실제 화면 관측이며 지속 성능이나
물리적 표시 FPS를 보장하는 벤치마크로 해석하지 않는다.

컨트롤 확장본의 실제 창에서 ComboBox 클릭 결과 `iiGUI action: 1`, 양방향 Stepper의
`+1`, 아래 방향 Stepper의 `-1`, 두 세그먼트 버튼을 각각 누른 결과 `iiGUI action: 3`을
확인했다. Core 작업자 결과 `42`와 프레임 `36 → 737 → 990 → 1410 → 1664 → 1993`을
관측했다. 짧은 표본은 60 Hz와 조작 중 52 Hz를 포함했다. 모든 컨트롤과 창 외형을 실제
화면에서도 확인했으며, 이 관측 역시 지속 성능·물리적 표시 FPS 측정을 뜻하지 않는다.

메뉴 확장본에서는 Dropdown의 자체 팝업을 화면에서 확인했다. 키보드 Down·Right로
하위 메뉴를 열고 Escape로 닫으면 More 항목으로 포커스가 돌아왔다. 입력란의 실제 오른쪽
클릭으로 ContextMenu를 열고 항목을 실행한 뒤 `Menu index: 0`과 팝업 종료를 확인했다.
Core 작업자 결과 `42`와 프레임 `50 → 629 → 2502 → 6254`, 짧은 표본 갱신률 59·60 Hz를
관측했다. 일반 메뉴는 실제 화면도 검사했고, 하위 메뉴·컨텍스트 메뉴의 행동과 포커스는
실제 창의 접근성 트리로 추가 확인했다.

앱과 게임은 같은 Core FrameLoop를 사용한다. 간격 0 콜백은 선택한 단계마다 한 번 실행되며,
고정 시뮬레이션·배속/일시정지 시간·실제 시간 작업을 따로 설정한다. 상태 문구의 250 ms
표본 주기는 프레임 수의 근거가 아니다. 실제 디스플레이 콜백 수로 갱신률을 계산하며,
표시 목표 주기·전달된 갱신률·GPU 제출·물리적 표시 FPS를 구분한다. 직전 메뉴 단계 iiGUI 검사의
목표 주기는 약 59.98 Hz였고 짧은 갱신 표본은 약 47.13 Hz였다. 계측 검사에서는 약 27.35 Hz였다.
이는 목표 주기를 실제 FPS로 간주할 수 없다는 뜻이며 지속 성능 벤치마크가 아니다.

추가한 Core 회귀 검사는 60·120·144 Hz의 결정적 호스트 타임스탬프와 그 순차 전환을
사용한다. 가변 업데이트는 표시 프레임마다 한 번, 10 ms 고정 시뮬레이션은 초당 100회이며,
동일 루프에서 3초 전환 시 초기 기준 프레임을 포함한 325프레임·300고정 업데이트와
주기 타이머 예약 0개를 검사했다. 실제 색상 갤러리의 재배치된 Core 설치 패키지로도 같은
검사가 통과했다. 이는 120/144 Hz 디스플레이의 실제 실행·물리적 표시 검증과는 다르다.

외장 스토리지의 LaunchServices 실행 1회는 `main` 이전 dyld 파일 열기에서 지연했다.
해당 테스트 프로세스를 종료하고 검증 때와 같은 환경으로 바이너리를 직접 실행해
실제 창을 확인했다. 표본은 `build/native/frame-live-sample.txt`에 보존하며
프레임 루프 내부의 멈춤과 구분한다. 독립 배포용 번들 검증이 완료되었다는 뜻은 아니다.

전체 LVRS의 이식 완료를 의미하지 않는다. [대응표](Parity.md)는 원본 149개 선언을
유지하며 현재 부분 구현 38개, 대기 111개, 전체 계약 완료 0개로 기록한다.
메뉴의 원본 배경 흐림·방사형 코팅·노이즈·등장 모션과 동적 객체 모델은 남아 있다.
읽기 전용 접근성 속성 표시 차이 등 확인한 제한은 [아키텍처](Architecture.md)와 JSON 원장에 남겼다.
모든 원본 기능·외형·플랫폼 요구를 검증할 때까지 전체 목표는 진행 상태이다.


## 색상 모델·표면 추가 검증

`ColorPickerModel`, `ColorPickerSurface`와 원본 인라인 `ColorField`를 네이티브 경로에 추가했다.
색상 모델의 RGB·HSV·CMYK·알파·그레이·HEX, 무채색 색조 유지, 잠재 CMYK 잉크,
원본 비교 색상과 적용·취소를 검사했다. 원본 휠 PNG 75,499바이트의 SHA-256은
`92dff08f1011dfbf27f67d2ed9fc5d84d5f50bb587d00a98f1e9819952ad31d4`이며,
내장 결과와 원본 디코딩 결과의 전체 448×448 RGBA 픽셀을 비교했다.

여섯 표면의 입력·가장자리 투영·방향키·Shift·비활성 상태, 공유 모델·모델 교체·이전 연결 해제,
콜백 중 자기 트리 제거, Cocoa 실제 입력·가상 접근성·레일 증감과 Metal 픽셀을 검사했다.
선택점만 움직일 때 배경 이미지가 재사용되고, 배경의 색조 의존성이 변하면 갱신되는지 확인했다.
캐시에 배경이 없을 때 CPU로 래스터화하고 GPU로 합성한다. 절차적 GPU 셰이더나 아틀라스의
완료를 뜻하지 않는다.

PNG는 이미 고정한 STB 헤더를 메모리 전용·PNG 전용으로 컴파일한다. 입력 8 MiB·해상도
2048×2048 제한과 별도로 모든 코덱 할당·재할당을 96 MiB로 제한한다. 작은 해상도로 위장하고
120 MiB로 압축 해제되는 입력을 실제 거부하는 검사도 포함한다. 입력과 최종 RGBA 저장 공간은
코덱 예산과 별개이며, STB MIT 고지를 설치 패키지에 포함했다.

예제 CTest는 성공 문자열을 근거로 종료 코드를 무시하던 정규식 통과 조건을 제거했다.
내부 C++ 자체 검사의 성공 마커를 확인하면서 프로세스 종료 코드 0도 별도로 요구한다.
색상 갤러리는 여섯 표면이 같은 모델을 공유하고, 작업자 결과·표시 프레임·크기 변경을 관측한 뒤
종료하며, 백엔드 1회 생성/해제와 모델의 최종 해제를 확인한다.

실제 화면 검사에서 RGB 진단 표시가 내림되어 HEX와 한 단계 다르게 보이는 문제를 찾았다.
모든 진단 채널을 반올림하도록 수정하고, 표시된 RGB 정수와 HEX의 일치 검사를 추가했다.
최초 설치 색상 갤러리와 검사 기록은 `build/native-color-consumer` 및
`build/native/build/colors-tests-before-caption-fix.xml`에 보존한다.

최종 색상 설치 소비자는 소비자·설치 예제 소스·생성 헤더·컴파일러·Core 진입점 소스를
사용할 수 없는 상태에서 종료 코드 0으로 통과했다. 최종 실제 창에서도 여섯 표면과 원본 휠을
확인했다. `00E71F80`을 넣으면 모든 표면이 `#00E71F`, 알파 슬라이더가 약 50.196%로 바뀌었다.
적용 후 RGB Red를 실행하면 현재 색상은 `#FF0000`이고 비교 원본은 `#00E71F`로 유지됐다.
취소하면 원래 RGBA로 복구됐으며, HS 표면에서 Right를 누르면 `#00E723`과 RGB `0,231,35`가
함께 표시됐다. 기본 `#874AFF`도 RGB `135,74,255`와 일치한다. Core 작업자 결과 `42`와
프레임 `300 → 1065 → 1425 → 1680 → 2070`을 관측했다. 물리적 표시 FPS 측정은 아니다.

색상 표면 단계에서 남았던 합성 UI는 아래 단계에서 추가했다. 모든 입력 스타일·일반 C++ 멤버
자동 재바인딩과 다른 플랫폼 입력 어댑터는 계속 남아 있다. HEX 공백 처리는 현재 ASCII 기준이다.

## 합성 ColorPicker 추가 검증 — 2026-10-23

`lvrs::ColorPicker`, `Widgets::color_picker`와 LVUI `ColorPicker` AOT 경로를 추가했다.
원본 여섯 편집 형태의 기본 크기·형태 전환 시 모델 보존·폭 확장·최근/액션 숨김 높이,
채널 램프·숫자 입력·HEX·알파·비교 체커보드·최근 색상·적용·취소·Escape·스포이트 요청을
검사했다. 상세 API와 남은 차이는 [ColorPicker 계약](ColorPicker.md)에 기록한다.

고정 배치·하위 트리 숨김·불변 이미지 표시·입력 포커스 손실 확정과 UTF-8 문자 수 제한은
iiGUI의 기존 위젯 실행 모델에 추가했다. 램프 트랙 위에 썸 이미지가 표시되는 실제 GPU 픽셀,
숨긴 자식의 포커스 제외와 복원, 포커스 확정 콜백에서 자기 트리를 제거하는 경계를 검사했다.
전체 구현과 공개 API를 C++23으로 빌드했으며 Qt와 다른 런타임은 추가하지 않았다.

첫 LVUI 전체 검사에서는 LVRS 소스 해시를 갱신하지 않아 인벤토리 1개가 실패했다.
`LVUI/build/picker-full-tests.xml`에 최초 기록을 보존하고 생성 목록을 갱신했다.
합성 선택기의 최초 TDD 링크 실패는 `build/native/picker-red-build.log`에 보존한다.

실제 최초 설치 창에서는 접근성 Apply/최근 색상 실행 뒤 HEX 입력에 포커스가 남아
한 입력란이 이전 값을 유지하는 문제가 확인됐다. 적용·취소는 소유 루트로, 최근 색상과
스포이트는 해당 버튼으로 포커스를 이동하도록 고쳤다. 그룹의 명시적 포커스는 새 Tab 정지를
추가하지 않는다. 수정 전 검사는 `*-before-focus-fix.xml`, 최초 설치본은
`build/native-picker-consumer`에 보존한다. 수정 후 전체 79/26/11개, 계측 50/6개와 새 설치
소비자에서 같은 경로가 통과했다.

최종 실행 파일은 `build/native-picker-consumer-final/build/LVRSNativePickerGallery.app`이다.
소비자 소스·설치 예제 소스·생성 헤더·컴파일러·Core 진입점 소스가 없는 실행에서
세 합성 뷰의 모델 공유, 작업자 결과 42, 실제 표시 프레임·크기 변경, 정상 종료와 스코프
1회 생성/해제·모델 해제를 확인했다. Mach-O arm64이며 Qt·OS 컨트롤 클래스 참조와
LVUI 파서 심볼이 없음을 검사했다. 다섯 번들 글꼴 해시와 실제 Core 진입점 컴파일도 검사했다.

최종 실제 창에서는 `00E71F80`을 입력하고 적용한 뒤 최근 빨강을 선택했을 때 세 HEX가 모두
`FF4646`으로 바뀌고 포커스가 최근 색상 버튼으로 이동했다. 취소하면 세 HEX가 `00E71F`,
알파 표시가 50으로 복원됐다. CMYK K에 25를 입력한 뒤 Tab으로 확정하면 세 HEX가
`00BF1A`, 밝기가 75로 바뀌었으며 Escape로 기준 초록과 알파를 복원했다. 체커보드·램프·
휠·평면·스펙트럼·액션의 실제 화면을 확인했다. 작업자 결과 42와 Core 프레임
`225 → 480 → 735 → 1995 → 2190`을 관측했으며 물리적 표시 FPS 측정은 아니다.

검증용 설치는 `build/` 안의 분리 경로이며 기본 설치본은 교체하지 않았다.
일반 반응형 옵션/모델 포인터 재연결, 원본 전체 재질·동작·상속·접근성 메타데이터와
다른 플랫폼 어댑터가 남아 있으므로 전체 목표는 계속 진행 상태이다.

## 일반 콘텐츠 Popover 추가 검증 — 2026-10-23

`Popover`를 LVRS→LVUI AOT→iiGUI 자체 C++ 오버레이로 연결했다. 일반 스택·입력란·슬라이더·
색상 필드를 콘텐츠로 유지하며, 한 백엔드를 재개방 때 재생성하지 않는다. 닫힌 콘텐츠의
자연 크기, 앵커 재배치·화면 경계·클리핑·스크롤·포커스 드러내기, 중첩 메뉴, 자식 Escape
우선 처리, Tab 순환, 모달·비모달 외부 누름과 실제 Metal 색상 도메인 픽셀을 검사했다.
닫기 콜백의 재개방과 트리 제거, 부모 숨김의 입력 확정 콜백이 트리를 제거하는 경우에도
후속 닫힘·클릭이 이전 트리에 실행되지 않는다.

독립 7개 iiGUI 검사와 기존 회귀를 포함한 전체 86개, LVUI 27개, LVRS 12개가 통과했다.
iiGUI 계측 57개와 LVRS 합성 구현·갤러리 계측 7개도 통과했다. 마지막 계측은 Core 동적
라이브러리 전체 계측이나 시스템 프레임워크의 메모리 누수 보증이 아니다.

`build/native-popover-consumer`에 네 패키지를 새로 설치·재배치하고 Qt 검색을 비활성화한
소비자를 빌드했다. 이 소비자는 원본·설치 예제·생성 소스·컴파일러·Core 진입점 소스가 없는
실행에서도 4회 열기/닫기·모델 확정/복원·외부 클릭 전달·작업자 결과 42·크기 변경·실제 표시
프레임·정상 종료를 검사했다. 스코프 생성/해제는 1/1이며 모델이 Core 종료에서 해제됐다.
최종 바이너리에 Qt 링크·LVUI 파서 심볼·NSButton/NSTextField/NSPopover/NSPanel 클래스
참조가 없고, 설치 Core 진입점 컴파일과 다섯 번들 글꼴의 동일 해시도 확인했다.

실제 창에서도 팝오버의 가상 접근성 그룹과 자식, HEX 편집 `00E71F80`→Apply, 재개방 후
`FF0000`→실제 외부 마우스 누름을 확인했다. 색은 기준 초록·알파 50%로 복원되고 배경의
Outside action이 1로 증가했다. 세 번째 재개방의 `11223380`→Escape도 기준색을 복원하고
앵커로 포커스를 돌렸다. 네 번째 팝오버를 열린 상태로 남겼다. 작업자 결과 42와 프레임
`405 → 1065 → 3030 → 4305 → 4605`를 관측했으며 물리적 표시 FPS 측정은 아니다.

현재 실행 파일은 `build/native-popover-consumer/build/LVRSNativePopoverGallery.app`이다.
검증 집계는 `build/native-verification.json`, 직전 선택기 단계 기록은
`build/native-verification-before-popovers.json`이다. 이전 설치 소비자들은 직전 단계 기록으로
구분했다. 기본 설치본은 교체하지 않았다. 현재 149개 선언 중 부분 구현 39개·대기 110개·
전체 계약 완료 0개이다. Popover의 원본 Glass25·스프링 전환·동적 콘텐츠·전체 상속·반응형
정책과 다른 OS 위젯 어댑터가 남아 있다. 상세 계약은 [Popover](Popover.md)이다.

## ColorPickerButton 추가 검증 — 2026-10-23

원본의 별도 320×320 색상환 PNG와 22·28·36 크기, 기본 보라색, 현재색·알파 원,
기본/hover/pressed 배경, 포커스 테두리와 전체 비활성 투명도를 자체 C++ 버튼으로 연결했다.
ColorPicker의 큰 휠을 재사용하지 않았으며 원본 PNG 바이트와 디코딩 픽셀이 동일하다.
원본 파일 SHA-256은 `75e39c9111d53f25e87fb1bd44eb7048074721bcb22a2d337d4e11927c94c427`이다.

전용 iiGUI 검사 5개는 기본 크기·잘못된 옵션·포인터/키보드·입력 취소·모델 교체·해제,
실제 Metal 상태 픽셀과 동일 장면 캐시를 검사했다. 최초 픽셀 검사에서 루트 할당 폭에 따라
원이 늘어나는 문제를 발견했다. 직사각형 배경과 중앙 정사각형 콘텐츠의 계산을 분리하여
원과 링을 보존했고, 루트 전체 폭을 사용하는 픽셀 검사도 통과했다. 최초 검사는
`iiGUI/build/color-button-first-tests.xml`, 진단은 `color-button-rendering-diagnostic.xml`에 있다.
LVUI 소스 목록은 새 헤더·소스를 포함해 223개로 갱신했으며, 갱신 전 목록 검사의 실패는
`LVUI/build/color-button-before-inventory-refresh.xml`에 보존했다.

최종 전체 검사는 iiGUI 91개·LVUI 28개·LVRS 13개, 계측은 iiGUI 62개·LVRS 8개가 통과했다.
별도 `build/native-color-button-consumer`에서 네 패키지를 재설치·재배치하고 설치된 Core 진입점으로
앱을 빌드했다. 소스·생성 헤더·컴파일러가 없는 소비자 실행에서도 색상 버튼 36 크기·공유 모델,
4회 재개방·편집/적용/복원·외부 누름 전달·작업자 결과 42·크기 변경·표시 프레임·정상 종료와
백엔드 생성/해제 1/1을 검사했다. Qt 링크·LVUI 파서·OS 컨트롤 클래스 참조가 없으며,
공개 설치 헤더·다섯 번들 글꼴과 생성 헤더도 현재 소스/빌드와 비교했다.

실제 새 창에서는 기본 버튼 값 `#874AFF`, 편집 `00E71F80`의 버튼·피커 동시 갱신,
적용 후 초록 미리보기와 포커스 테두리, Space 재개방, `FF0000` 편집 중 빨강 미리보기를
확인했다. 실제 외부 마우스 누름으로 미리보기·모델이 초록/알파 50%로 복원되고
Outside action이 1로 증가했다. 세 번째 열기의 `11223380`→Escape도 버튼·모델을 복원했다.
작업자 결과 42, 프레임 `195 → 1545 → 1800 → 3510 → 4380`을 관측했다.
이전 검증 앱은 정상 종료 코드 0을 확인한 뒤 새 앱을 실행했다. 새 창은 편집기를 닫고
초록 버튼을 표시한 상태로 남겼다. 물리적 화면 표시 FPS 측정은 아니다.

현재 앱은 `build/native-color-button-consumer/build/LVRSNativePopoverGallery.app`이다.
직전 기록은 `build/native-verification-before-color-buttons.json`이며 현재 집계는
`build/native-verification.json`이다. 기본 설치본은 교체하지 않았다. 현재 선언 149개 중
부분 구현 40개·대기 109개·전체 계약 완료 0개이다. 버튼의 스프링·반동·전환,
전체 상속·반응형 입력과 다른 플랫폼 어댑터는 남아 있다. [상세 계약](ColorPickerButton.md)을
기준으로 전체 이식을 계속 진행한다.

## 열림 단위 콘텐츠 수명 추가 검증 — 2026-10-23

원본 Modal의 `contentComponent`는 열린 동안 생성되고 닫으면 해제된다. 이 계약을 위한
기반으로 iiGUI `Widget::set_popup_content`와 `popup_content`를 추가했다. 기존 정적
Popover 콘텐츠는 유지되며, C++ 생성 함수를 설정한 경우에만 재개방마다 별도 위젯 상태를
만든다. 외부 LVUI 백엔드는 한 번 유지하고 콘텐츠의 C++ 객체·모델은 열림 단위로 소유한다.
이는 Modal 자체의 시각 조합이나 LVUI 자식 콘텐츠 생성 구문을 완성한 것이 아니다.

독립 콘텐츠 검사 10개와 네이티브 검사 1개를 추가했다. 500회 재개방에서 활성 노드 수가
12개로 유지되고 이전 위젯·접근성 대상은 새 콘텐츠를 가리키지 않음을 검사했다. 전체 상태의
노드 4,096개·열린 팝업 8개·콘텐츠 깊이 8개 제한, 생성 실패와 예외의 롤백, 닫기 콜백의
재개방, 부모 숨김 중 자식 포커스 확정이 부모를 제거하는 경우도 검사했다.
닫힘 통지 전에 콘텐츠 소유권과 핸들을 해제한다. 실행 중인 C++ 콜백이 보유한 캡처는
그 호출이 반환한 뒤 해제될 수 있으며, 외부 공유 소유자와 작업은 명시적 수명 계약을 따른다.

실제 AppKit 검사에서는 26회 개방 중 입력 NSView의 수와 동일성을 유지하고, Cocoa 문자
입력과 IME 대상이 현재 자식 입력란에 도달함을 확인했다. 닫힌 세대의 가상 접근성 객체는
제거하며, 숨겨 둔 정적 위젯의 객체는 유지한다. 재개방 후 이전 AX 대상의 행동은 거부되고
반복 종료 후 캐시 크기도 일정하다. 중첩 메뉴가 콘텐츠 경계를 넘어 호스트 창 안에 배치되는지와
실제 Metal 색상 픽셀도 검사했다. NSControl·NSPopover·NSPanel은 추가하지 않았다.

최초 검사에서 중첩 메뉴를 콘텐츠 크기에 제한한 문제, 네이티브 포커스가 실제 자식 대신
외피를 선택한 문제, 해제한 세대의 AX 객체가 쌓이는 문제를 발견해 수정했다. 원인은 각각
`iiGUI/build/popup-content-expanded-tests.xml`, `popup-content-native-before-focus-fix.xml`,
`popup-content-before-ax-cache-fix.xml`에 남겼다. 수정 후 전체 iiGUI 102개·LVUI 28개·
LVRS 14개와 iiGUI 계측 73개·LVRS 계측 9개가 모두 통과했다.

새 `NativeActivationGallery` 설치 소비자는 네 패키지를 재배치하고 Qt 검색을 끈 상태로
빌드했다. 소스·생성 헤더·컴파일러·Core 진입점 소스가 없는 실행에서 콘텐츠 생성/해제 4/4,
외부 백엔드 1/1, 낡은 핸들 거부·모달 외부 클릭 차단·기준색 복원·작업자 결과 42·크기 변경·
실제 표시 프레임·정상 종료를 확인했다. 설치 공개 헤더·생성 헤더·다섯 글꼴의 해시가 맞으며
바이너리에 Qt 링크·LVUI 파서·OS 컨트롤 클래스 참조가 없다.

실제 창에서는 첫 `00E71F80`→Apply 후 생성/해제 1/1, 두 번째 `FF0000` 편집의 빨강 표시와
물리적 외부 누름 후 기준 초록 복원·2/2, 세 번째 `11223380`→Escape 후 초록 복원·3/3을
확인했다. 재개방 시 입력 식별자 세대가 `:1/`→`:3/`→`:5/`로 바뀌며 포커스가 원래 버튼에
돌아왔다. 작업자 결과 42와 표시 프레임 `270 → 1770 → 23070 → 24135`를 관측했다.
최종 창은 콘텐츠를 닫고 초록 미리보기를 표시한 상태이다. 물리적 표시 FPS 측정은 아니다.

프레임 질의 재확인에서는 기존 Core 프레임 검사 1개와 iiGUI 표시 호스트 검사 4개를 다시
빌드·실행했다. 결정적 60·120·144 Hz 전환과 100 Hz 고정 스텝 계약이 통과했고, 실제
디스플레이 표본은 목표 약 59.982 Hz·전달 약 54.9364 Hz였다. 별도 변경 억제 검사에서는
24개 콜백에 8개 표시 제출이었다. 목표 주기·전달 갱신률·표시 제출을 같은 수치로 취급하지 않는다.

현재 앱은 `build/native-activation-consumer/build/LVRSNativeActivationGallery.app`이다.
직전 집계는 `build/native-verification-before-activation.json`, 현재 집계는
`build/native-verification.json`이다. 선언은 149개, 부분 구현 40개·대기 109개·전체 계약 완료
0개로 유지한다. Modal의 중앙 배치·제목/아이콘/액션·원본 재질/모션·언어 수준 자식 생성은
남아 있으며 [콘텐츠 수명 계약](ActivationContent.md)에 경계를 명시했다. 기본 설치본은 교체하지 않았다.

## Modal 조합·AOT 자식 생성 추가 검증 — 2026-10-23

`lvrs::Modal`, `Widgets::modal`, LVUI `Modal`을 실제 네이티브 경로에 추가했다.
iiGUI에는 중앙 팝업 배치·폭 제약·수직 오프셋·원통 반경·배경색, 폭에 따른 스택 높이·
교차축 정렬·가변 폭 분배와 별도의 취소 이벤트를 추가했다. 기본 앵커 팝오버와 기존 스택의
기본 배치는 유지한다. 공통 계약은 iiGUI의 `docs/DialogLayout.md`에 있다.

LVRS는 원본 Modal의 아이콘·제목·설명/메시지 대체·최대 세 액션·최소/최대/선호 폭·
20단위 내부 패딩·중앙보다 16단위 위의 배치를 구성한다. 원본 토큰을 대조해 아이콘 테두리를
`#D0D7E2`의 18%로, 설명을 body 텍스트 역할로 맞췄다. 액션은 3차→2차→기본 순으로
오른쪽에 놓이며 높이 20이다. 액션은 자동으로 닫지 않는다. 닫기와 취소는 서로 다른 이벤트이다.

LVUI 자식 블록은 별도 `ModalContentFactory`로 컴파일하고 열릴 때 실행한다. 부모 백엔드는
기존 스코프를 빌리고 자식 ID·컨텍스트 메뉴 대상은 독립 범위를 사용한다. 초기 `open: true`는
마운트와 백엔드 구성 이후 실행한다. 범위를 넘는 대상 참조와 생성 중 중첩 초기 개방은
명시적으로 진단한다. C++에서도 콘텐츠 생성 함수를 전달하고 활성 콘텐츠 핸들을 얻을 수 있다.

전용 iiGUI 3개는 중앙 배치·폭 변화·줄바꿈 높이·일반 호스트 스택의 폭 분배/정렬·spacer
최소 길이·일반 닫기/취소의 구분을 검사했다. LVRS Modal 4개는 기본 기하·아이콘·설명 대체·
액션 순서/기본값/차단·30회 콘텐츠 생성/해제·크기 변경 시 수명 유지와 실제 Metal 픽셀을
검사했다. 컴파일러 검사 1개와 Core 기반 예제도 추가했다. 최초 미구현 API와 예약어 진단은
`iiGUI/build/modal-layout-red.log`, `LVUI/build/modal-red-tests.xml`에 보존했다.

최종 전체 iiGUI 105개·LVUI 29개·LVRS 19개와 계측 iiGUI 76개·LVRS 14개가 모두 통과했다.
새 `build/native-modal-consumer`는 네 패키지를 설치·재배치해 Qt 검색 없이 빌드했다.
소스·설치 예제·생성 헤더·컴파일러·Core 진입점 소스가 없는 실행에서 개방/닫기 4/4,
취소 2회·기본 액션 2회·부모 백엔드 1/1·새 자식 세대·크기 변경·작업자 결과 42·
표시 프레임·정상 종료를 확인했다. 설치 공개 헤더·생성 헤더·다섯 글꼴도 대조했다.
바이너리에 Qt 링크·LVUI 파서 심볼·OS 컨트롤 클래스 참조가 없다.

실제 설치 창에서도 중앙 프레임·아이콘·줄바꿈 설명·우측 액션을 확인했다. 첫 입력란에
`Edited in native Modal`을 넣은 뒤 Apply를 눌러도 열린 상태였다. Close dialog 후
`opened 1 / closed 1 / canceled 0 / primary 1`이었다. 두 번째 개방의 입력은 `Fresh editor`로
초기화되고 세대가 `:1/`에서 `:3/`으로 바뀌었다. Escape 후 `2/2/1/1`, 세 번째 개방의
실제 배경 버튼 위치 누름 후 `3/3/2/1`을 확인했다. 배경 액션 문구로 바뀌지 않았고 포커스는
열기 버튼에 돌아왔다. 작업자 결과 42와 프레임 `480 → 2490 → 3000 → 3225 → 3585`를
관측했다. 네 번째 개방은 세대 `:7/`과 초기 입력을 표시한 상태로 남겼다.

현재 앱은 `build/native-modal-consumer/build/LVRSNativeModalGallery.app`이다. 직전 원장은
`build/native-verification-before-modal.json`, 현재 원장은 `build/native-verification.json`이다.
149개 선언 중 부분 구현 41개·대기 108개·전체 계약 완료 0개이다. [Modal 계약](Modal.md)의
원본 스프링/퇴장·18단위 고정 줄 높이·속성 재바인딩·상속·overlay 부모·자식 내부 import와
타 OS 위젯 어댑터는 남아 있다. 헤더/액션의 숨김 내부 객체 수명도 원본과 모두 같지는 않다.
전체 목표는 진행 상태이며 기본 설치본·서명·공증은 이번 범위에서 갱신하지 않았다.


## Alert·AlertButton·Metal 유리 재질 추가 검증 — 2026-10-23

`lvrs::Alert`·`lvrs::AlertButton`, Widgets 진입점과 LVUI AOT를 실제 C++ 위젯 경로에 연결했다.
원본의 자동 액션 개수, 1/2개 가로·3개 세로 배치, 26/13 고정 줄 높이, 비활성 상태,
텍스트·이미지 별칭과 메서드→클릭 순서를 구현했다. 2줄 본문에서 500×379·500×479이며,
좁은 창에서는 최소 폭 아래로 줄어든다. 여섯 액션은 자동으로 닫지 않고 소유자가 결정한다.
원본처럼 닫혀 있는 동안 정적 위젯을 유지하고 100회 재개방에서 동일 핸들·노드 수를 검사했다.
이는 열림마다 콘텐츠를 생성하는 Modal의 수명과 다르다.

배경 콘텐츠만 캡처하는 실제 Metal Gaussian blur, RGBA16Float 선형 중간 텍스처,
반투명 tint·둥근 마스크·테두리를 구현했다. ROI와 최대 64 MiB 작업 텍스처를 사용하며,
카드 글자/버튼만 바뀌면 같은 배경 blur를 재사용한다. 렌더링 중 CPU 픽셀 읽기는 없다.
실제 검사는 흑백 경계 흐림·이전 overlay 제외·모서리 마스크·선형 tint·캐시 재사용·해제를
캡처 픽셀과 계수로 확인한다. 명시적인 Vulkan material 요청은 실패로 관측한다.
패널의 안쪽 테두리는 기존 64바이트 인스턴스에 추가했고 Metal/Vulkan 픽셀로 검사했다.
D3D11 테두리와 Windows/Linux 고정 줄 높이는 소스 반영 범위이다.

원본 SVG 두 개를 수정 없이 내장하고 C++에서 원본 바이트와 비교했다. 독립 AlertButton의
포인터·키보드 pressed→released→method→clicked와 비활성화 취소, 실행 중 메서드 교체와
부모 제거를 검사했다. 실제 앱과 C++ 테스트는 인자 있는 ButtonEvent 메서드와 인자 없는
메서드를 모두 실행하며 부모 backend는 한 번 생성·해제한다. C++23에서 지원하지 않는
이종 키 map::at를 사용한 초기 빌드는 실패했고 표준 find 조회로 수정했다. 최초 진단은 작업 도구 출력에 남으며, 수정 뒤 `LVUI/build/alerts-build.log`와
`alerts-final-build.log`의 최종 빌드는 경고 없이 통과했다.

원본과 비교하며 두 추가 차이를 먼저 실패 검사로 재현했다. 비정사각 아이콘이 늘어나던
동작은 Alert·Modal 모두 비율 유지·중앙 배치로 수정했다. 버튼 포커스의 사각 모서리는
원본 반경과 1.5단위 GPU 선으로 수정했다. 실패/성공 기록은
`build/native/build/alert-icon-fit-red-tests.xml`·`alert-icon-fit-tests.xml`,
`iiGUI/build/alert-focus-red-tests.xml`·`alert-focus-tests.xml`이다. 수정 뒤 전체 회귀와
ASan/UBSan을 다시 완료했다. 이번 최종 iiGUI 114개·LVUI 30개·LVRS 30개,
iiGUI 계측 85개·LVRS 계측 25개에서 실패·건너뜀은 0개이다.

최종 소비자는 `build/native-alert-consumer-final`이다. 네 패키지를 설치·재배치하여
소스 include 환경 없이 빌드했고 소스·생성 헤더·컴파일러·Core 진입점 소스를 일시적으로
제거한 상태에서 실행했다. 자체 위젯·Core 진입점·다섯 글꼴을 검사하고 Qt 링크,
LVUI 파서 심볼과 Cocoa 컨트롤 클래스가 없음을 확인했다. 원본 Alert SVG 라이선스도 설치한다.
Vulkan SPIR-V는 원본을 보존한 공식 glslang 16.0.0 소스로 재생성했고 검사/실제 stroke 픽셀을
통과했다. 해당 컴파일러는 빌드 도구이며 앱 런타임 의존성이 아니다.

최종 앱을 실제로 열고 두 액션에서 배경 클릭·Escape가 닫지 않는 정책과 Tab/Space를 확인했다.
기본 액션은 열린 상태를 유지했고 2차 액션 뒤 상태는 `1a2b`, 열림/닫힘 1/1이었다.
독립 버튼과 단일 액션을 실행한 뒤 명시적 Escape에서 dismissed 1·oneOpen 포커스 복귀를
관측했다. 세 액션의 원본 file-text 아이콘, 세로 배치와 destructive 글자를 확인하고 기본·2차·
재개방 3차를 실행했다. 상태는 `1a2b1a1a1a2b3c`, 열림/닫힘 4/4·dismissed 1이었다.
작업자 결과 42와 표시 프레임 `2175 → 2970 → 4035 → 4455 → 5310`을 관측했다.
이는 전달받은 Core 프레임 계수이며 물리적 표시 FPS 측정은 아니다. 다섯 번째 개방에서는
같은 `two.horizontal.1/2` ID를 확인하고 창을 열어 두었다. AX 상태·화면 네 개와 관측 JSON을
소비자 폴더에 보존한다. 크기 변경과 소스 없는 실행의 자동 검증은 별도 증거이다.

직전 Modal 원장은 `build/native-verification-before-alerts.json`에 보존한다. 현재 전체 원장은
`build/native-verification.json`이며 선언 149개 중 부분 구현 44개·대기 105개·완료 0개이다.
[Alert 계약](Alert.md)에 반응형/상속·임의 backdrop/overlay 부모·
이미지 URL과 일부 프레임 조합을 남긴다. 실제 GPU 흐림은 현재 Metal 전용이며 다른 백엔드는
명시적으로 거부한다. 전체 플랫폼 위젯 입력과 디스플레이 동기화도 미완료이다.
기본 설치본과 기존 사용자 앱은 교체하지 않았다. 전체 LVRS 이식 목표는 계속 진행 중이다.

## Alert 그림자·프레임 모션 단계

원본 그림자와 모션을 실제 Metal/Core 경로로 추가했다. 그림자 마스크는 R8이며 Gaussian
결과는 R16Float이다. 16 아래 offset·blur 32·검정 28%를 사용하고 모양이 바뀔 때만
다시 계산한다. 카드의 420ms OutBack·overshoot 1.45·150ms InCubic, 0.92→1 scale과
opacity를 합성하며 글자와 이미지는 기존 래스터를 재사용한다. 유리와 그림자는 변환하지
않는 형제이다. 도중 닫기/재개방, 즉시 입력 회수·포커스 복귀, 공유 reduced_motion과
speed, 뷰 해제·숨김과 유휴 프레임 소스 정지를 검사한다.

통합 검사에서 비변환 유리의 캡처 영역에 카드 변환이 잘못 적용되어 blur가 반복되는
오류를 발견했다. 효과 ROI에서 content_transform을 제외한 뒤 모션 동안 글자 래스터·
그림자 blur·배경 blur 횟수가 증가하지 않는 검사를 통과했다. 이전 모션 트리를 다른
뷰로 교체할 때 최종 프레임 구동 상태를 적용하고, 폭/높이 0인 팝업은 효과 제출을 생략한다.
명시적인 합성 한도는 64레이어이며 팝업 도메인은 열린 8개·열림/exit 합계 16개이다.

현재 단계의 결과는 다음과 같다. 실패·건너뜀은 모두 0이다.

- Core 전체 56개: `iiSDKCore/build/native-motion-full-tests.xml`.
- iiGUI 전체 134개: `iiGUI/build/alert-motion-full-tests.xml`.
- LVUI 전체 30개: `LVUI/build/alert-motion-full-tests.xml`.
- LVRS 네이티브 전체 33개: `build/native/build/alert-motion-full-tests.xml`.
- iiGUI ASan/UBSan 105개: `iiGUI/build/aot-sanitizer/build/alert-motion-sanitizer-tests.xml`.
- LVRS ASan/UBSan 28개: `build/native-sanitizer/build/alert-motion-sanitizer-tests.xml`.

`Motion.h/.cpp`는 원본 시간 토큰·네이티브 공유 모델·Alert의 실제 전환 정책을 제공한다.
LVUI `motion: vm.motion`은 스코프 소유 C++ 멤버로 컴파일한다. 일반 SpringBehavior와
모든 다른 컴포넌트의 모션, OS 모션 감소 설정 탐지, 추가 플랫폼 입력/RHI 효과는 아직 남아 있다.
현재 대응표는 부분 44개·대기 105개·완료 0개이다. 이전 Alert 원장은
`build/native-verification-before-alert-motion.json`에 보존한다.

새 설치 소비자 위치는 `build/native-alert-motion-consumer`이다.
`build/verify_alert_motion_consumer.py`가 네 패키지와 기존 MoltenVK 의존성을 같은
전용 prefix로 재배치하고, 예제 소스·생성 헤더·컴파일러·Core 진입점 소스를 실행 동안
분리한 뒤 복원한다. 결과는 해당 폴더의 `verification.json`과 `tests.xml`로 판정한다.
아카이브는 설치 시 달라질 수 있는 인덱스 대신 객체 멤버의 바이트로 비교한다.
새 예제의 자기 검사는 열림/닫힘 6회와 실행 중 모션 감소 설정, GPU 그림자/유리,
여섯 액션·독립 버튼·작업자 42·크기 변경·Core 정리를 포함한다.

## Modal 외곽·본문 수명과 프레임 전환 단계

원본 `Modal.qml`을 다시 대조하여 제목·아이콘·액션을 정적 위젯으로 옮겼다.
`contentLoader.active`는 닫힘 즉시 false이므로 본문만 먼저 해제하고, 외곽이 150ms
퇴장하는 동안 본문 없는 높이를 다시 계산한다. 420ms OutBack 열림·overshoot 1.45,
0.92→1 scale, opacity/dim과 실행 중 모션 감소·속도를 적용한다. 재개방은 같은 외곽
핸들과 현재 진행 값을 사용하면서 새 본문 세대를 만든다. `frameColor`의 alpha도
선형 색 공간에서 합성한다. 빈 헤더·액션 섹션과 본문만 있는 배치도 검사했다.

iiGUI의 `set_popup_content_at`은 정적 외곽 안의 빈 VStack을 본문 앵커로 지정한다.
본문 크기 변경이 외곽 배치를 갱신하고, 본문·액션 사이 Tab/Shift-Tab, 포인터 캡처,
중첩 메뉴 차단과 해제를 연결한다. 본문으로 포인터가 이동한 뒤 남던 외곽 hover와
캡처 상태를 실패 검사로 재현해 수정했다. 입력 비활성화가 명시적 레이블 색까지
바꾸던 차이, 비상호작용 본문의 불필요한 Tab 정지점, 최초 포커스가 외곽 입력란을
거쳐 본문으로 이동하던 차이도 수정했다. blur 콜백에서 뷰가 해제되면 이전 본문에
대기 중인 포커스를 전달하지 않는다. 본문 생성 실패와 닫힘 콜백 재개방도 검사한다.

현재 소스의 검사에서 실패·건너뜀은 모두 0이다.

- iiGUI 전체 148개: `iiGUI/build/modal-motion-full-tests.xml`.
- LVUI 전체 30개: `LVUI/build/modal-motion-full-tests.xml`.
- LVRS 네이티브 전체 40개: `build/native/build/modal-motion-full-tests.xml`.
- iiGUI ASan/UBSan 119개: `iiGUI/build/aot-sanitizer/build/modal-motion-sanitizer-tests.xml`.
- LVRS ASan/UBSan 35개: `build/native-sanitizer/build/modal-motion-sanitizer-tests.xml`.

Core 소스는 직전 단계와 같으며 기존 56개 전체 검사 증거를 재사용한다. Core 공유
라이브러리 자체는 위 계측 범위에 포함하지 않는다. 최신 예제는 실제 네이티브 프레임
진입부터 5초 완료 감시를 시작하고, CTest의 전체 60초 제한을 유지한다. 초기 전체 검사에서
진입 전 설치한 감시가 한 번 만료되었고 단독 재검사는 통과했다. 최종 전체 검사와
계측 검사는 수정한 예제로 완료했다. 감시 실패에는 단계·프레임·진행률·작업자·크기 변경
상태를 포함한다.

`build/verify_modal_motion_consumer.py`는 현재 네 패키지와 기존 MoltenVK를
`build/native-modal-motion-consumer/relocated`에 설치한다. 각 package_DIR을 명시하고
Qt 검색을 비활성화한다. 소스·생성 헤더·컴파일러·Core 진입점 소스가 없는 실행 결과는
소비자의 `verification.json`과 `tests.xml`로 확인한다. 정적 라이브러리는 설치 시 달라질 수
있는 아카이브 인덱스 대신 객체 멤버의 바이트를 비교한다.

같은 설치 소비자 앱의 실제 창에서 본문을 `Edited native body`로 편집하고 Apply가
닫지 않는 동작을 확인했다. 닫은 뒤 다시 열면 본문 세대가 `@iiscope:12:1`에서
`@iiscope:12:3`으로 바뀌며 `Fresh editor`로 초기화됐다. 제목·액션의 정적 ID는
유지했고, 실제 C++ 핸들 동일성은 자동 검사로 검증했다. Tab 뒤 Right로 슬라이더가
50에서 51로 바뀌었고, Escape는 취소를 한 번 기록한 뒤 열기 버튼으로 포커스를
복귀시켰다. 공유 설정의 모션 감소 켜짐과 2배속 상태도 실제 창에서 확인했다.
마지막에는 1배속·모션 감소 꺼짐으로 복원하고 새 본문을 연 상태로 두었다.
접근성 기록·스크린샷과 관측은 소비자 폴더의 `live-observations.json`에 보존한다.

타이머와 프레임의 관계를 재확인한 Core FrameLoop·Realtime 2개와 iiGUI 표시·계약·위젯
3개도 실패·건너뜀 없이 통과했다. 각 패키지 `build/frame-timer-question-tests.xml`에
기록하며, Core의 60·120·144 Hz 검사는 결정적 타임스탬프이고 iiGUI의 검사는 실제
macOS 표시 콜백이다. 이 재확인은 새 타이머 API 추가나 물리적 표시 FPS 측정이 아니다.

현재 원장은 `build/native-verification.json`, 이전 원장은
`build/native-verification-before-modal-motion.json`이다. 선언 149개 중 부분 44개·대기
105개·완료 0개를 유지한다. Modal의 전체 반응형·상속 속성, overlay 부모, 임의 아이콘 URL,
본문 내부의 별도 backend import, 모든 플랫폼 입력·표시와 일반 SpringBehavior는 남아 있다.

## Sheet·스크롤·이동 합성 단계

원본 Sheet의 데스크톱 중앙 배치와 모바일 하단 배치, fit/medium/large 높이,
명시적 안전 여백·헤더·닫기 버튼·grabber·본문 스크롤을 C++로 구현했다.
Modal과 달리 본문은 닫힌 동안에도 보관하며 21회 재개방에서 같은 객체·편집 값을
유지한다. 원본의 420ms OutBack/InCubic과 드래그 복귀는 Core unscaled 프레임을
사용한다. 이동 합성은 기존 Metal 전경을 재사용하고 subpixel 보간을 적용한다.

스크롤 뷰포트는 교차한 GPU scissor·휠의 중첩 전달·포커스 노출·크기 변경 제한을
적용한다. 별도 팝업이 조상 스크롤에 잘리거나 팝업 포커스가 원래 본문을 이동시키지
않도록 경계를 분리했다. 트리의 표시 순서와 할당 순서가 다를 때 클릭이 뒤의 버튼으로
전달되는 문제도 GPU 픽셀과 입력을 함께 검사해 재현·수정했다.

현재 최종 검사에서 실패·건너뜀은 모두 0이다.

| 검사 | 결과 | 기록 |
|---|---:|---|
| iiGUI 전체 | 155/155 | `iiGUI/build/sheet-full-tests.xml` |
| LVUI 전체 | 31/31 | `LVUI/build/sheet-full-tests.xml` |
| LVRS 전체 | 50/50 | `build/native/build/sheet-full-tests.xml` |
| iiGUI ASan/UBSan 전체 | 155/155 | `iiGUI/build/aot-sanitizer/build/sheet-sanitizer-final-tests.xml` |
| LVRS ASan/UBSan 전체 | 50/50 | `build/native-sanitizer/build/sheet-sanitizer-tests.xml` |
| 재배치한 Sheet 설치 소비자 | 1/1 | `build/native-sheet-consumer/tests.xml` |

Core는 변경하지 않았으며 직전 56개 전체 검사 증거를 재사용한다. 위 계측은
iiGUI·LVRS·해당 코덱/소비자 범위이며 별도로 설치한 Core 공유 라이브러리 전체나
시스템 코드·커널을 계측했다는 뜻은 아니다. leak detection은 비활성화했다.
첫 iiGUI 계측 전체에서는 기존 앱 수명 2개가 15초 제한에 걸렸다. 같은 바이너리와
같은 제한에서 각각 3회씩 재검사한 뒤 전체 155개도 다시 통과했다. 최초 기록은
`sheet-sanitizer-first-tests.xml`, 반복 기록은 `sheet-application-retests.xml`과
동명의 로그에 보존한다. 원인을 확정하거나 시작 지연이 해결되었다고 주장하지 않는다.

소비자는 Core·LVUI·iiGUI·LVRS와 기존 MoltenVK를 별도 경로에 설치한다. Qt 검색을
끄고 설치된 Core 진입점을 실제 컴파일한다. 실행할 때 소비자/설치 예제 소스·생성
헤더·컴파일러·Core 진입점 소스를 사용할 수 없게 했다가 복구했다. Qt·LVUI 파서와
NSButton/NSTextField/NSPanel/NSPopover/NSScrollView/NSScroller/NSClipView 클래스의
링크가 없음을 확인했다. 정적 아카이브의 객체 멤버는 iiGUI 21개·LVUI 3개·LVRS 13개가
원본 빌드와 일치한다. 최종 바이너리는 `build/native-sheet-consumer` 아래의 앱이다.

실제 창에서도 데스크톱 본문을 편집하고 Apply가 닫지 않는 동작, 헤더를 고정한 스크롤,
재개방 시 같은 ID·편집 값과 스크롤 초기화를 확인했다. Escape 후 열기 버튼으로
포커스가 돌아왔고, 모바일 grabber를 실제 포인터로 길게 드래그하면 닫혔다. 짧은
드래그는 모션 감소 상태에서 열린 채 복귀했다. 2배속·모션 감소 켜짐을 확인한 뒤
1배속·모션 감소 꺼짐으로 복원했다. 모바일 편집 값도 보존했으며 마지막에는 모바일
Sheet를 열어 두었다. 접근성·이미지와 관측은 `live-observations.json`에 보존한다.
이는 Mac의 모바일 프레젠테이션 실행이며 iOS/Android 장치 실행·물리적 표시 FPS의
측정은 아니다. 기본 `~/.local` 설치와 기존 실행 앱은 교체하지 않았다.

해당 Sheet 검증 당시 대응표는 149개 선언 중 부분 45개·대기 104개·전체 계약 완료 0개였다.
Sheet의 전체 상속/반응형·전환 완료 신호 타이밍·자동 OS 안전 영역 관찰·터치 관성과
모든 플랫폼 어댑터가 남아 있으며 전체 LVRS 이식 목표는 계속 진행 상태이다.


## Card·Avatar 구현 단계

Card의 여덟 유형, Avatar, 이미지 원본 영역·마스크·그라데이션·전경 텍스처 영역 및
순수 paint provider를 연결한다. LVUI AOT와 반응형 이미지/행/커스텀 팩토리, 네이티브
갤러리를 추가한다. 자세한 범위와 미완성 계약은 Media.md와 parity 장부를 따른다.
모든 앱·테스트·self-test 실행은 전체 목표 최종 일괄 단계로 보류한다. 중간 빌드와
비공개 설치 소비자·정적 바이너리 검사는 실행 검증과 분리해 build/native-media-evidence에
기록한다. 기존 Qt 구현과 전역 설치본은 별도 유지한다.


## 네이티브 머티리얼 단계

MaterialSurface·PanelMaterial·WindowMaterial과 여덟 악센트 그라데이션, 임의 윤곽,
공유 마스크 그림자, 흐림 전 투명 입력, 안쪽 하이라이트와 Core 색상 전환을 연결한다.
일반 위젯 paint provider와 뷰별 공유 Core 프레임을 사용한다. 앱·검사·self-test 실행은
전체 목표의 최종 일괄 단계로 보류한다. 컴파일·설치 소비자·셰이더·패키지 정적 증거는
build/native-materials-evidence에 기록한다. 기능 범위와 남은 계약은 Materials.md 및
대응표를 따른다. Windows HLSL 전처리/셰이더 프런트엔드 확인은 Windows 네이티브 빌드와 다르다.

## 텍스트 문서 기반 단계

TextDocumentModel·TextMarkup과 창 없는 iiGUI TextIndex를 추가한다. 파일 오프셋·청크·
문자 경계·범위 수정·이력·iiFileProvider 원자적 저장을 연결한다. LVUI 문서 모델 갤러리는
현재 줄만 조회하는 유지 스코프를 시연하며 TextEditor/CodeEditor 화면의 대체물이 아니다.
앱·검사·self-test는 전체 목표의 최종 일괄 단계로 보류한다. 소스/계측 컴파일과 설치 소비자
정적 증거는 build/native-text-document-evidence에 기록한다. 기능 범위와 남은 계약은
TextDocuments.md 및 대응표를 따른다.

## 자체 CodeEditor 기반 단계 기록

PlainText/NoWrap CodeEditor, iiGUI TextLine의 네이티브 단락 배치·가로 crop,
요청 범위를 제한한 TextInputProvider, 독립 선택·IME 투영과 Core 소유 caret 작업을 연결했다.
원본 CodeEditor.qml과 현재 Theme의 수치를 대조했으며 기능 범위와 남은 시각·입력 계약은
CodeEditor.md 및 대응표를 따른다. RichText TextEditor의 대체 구현으로 간주하지 않는다.

2026-10-24 macOS arm64에서 일반/ASan+UBSan 구성의 컴파일·링크·작업 폴더 내 설치를
완료했다. 재배치 패키지로 소비자와 일곱 독립 갤러리를 빌드·설치했다. 현재 공개 헤더
134개와 범위 내 소스 객체 178개, Core 진입점, 정적 아카이브, AOT 출력·리소스·문서의
일치를 정적으로 확인했다. 검사한 macOS 소비자에서 Qt·LVUI 런타임 파서와 Cocoa 편집
컨트롤의 직접 클래스 참조는 발견되지 않았다. 이는 실제 창·입력기·픽셀 실행 검증과 다르다.

기록은 build/native-code-editor-evidence/final-build-audit.json 및
runtime-batch-pending.json에 있다. 여러 빌드 구성을 포함한 등록 항목 1,818개는 모두
미실행이다. 이번 단계에서 앱·테스트·self-test를 실행하지 않았으며 사용자의 요청에 따라
전체 이식 목표의 최종 일괄 단계로 보류했다. ASan/UBSan으로 컴파일했다는 사실은 런타임
메모리 안전성의 통과 증거가 아니다. Core·LVUI·provider의 번들 SQLite는 계측하지 않았다.
Windows/Linux 새 폰트·입력 소스는 실제 대상에서 빌드/실행하지 않았다.

최신 대응표는 149개 선언 중 부분 79개·대기 70개·전체 계약 완료 0개이다.
기존 전역 설치와 실행 중인 앱은 교체하지 않았으며 전체 LVRS 이식 목표는 진행 상태이다.

## 공유 편집 상태·네이티브 글리프 색상 확장

정확한 UTF-16 범위·transaction·정규화한 삽입 길이·undo/redo 방향을
TextDocumentEdit로 통지한다. 모델은 실패한 수신자 이후의 살아 있는 뷰에도 이미 반영된
변경을 전달하고 오류를 반환한다. CodeEditor는 외부 편집에 따라 독립 선택을 재배치하며
편집을 시작한 뷰의 역방향 선택까지 제한된 메타데이터로 undo/redo한다. 빈 undo와 뷰 탐색은
문서 이력이나 커서를 갱신하지 않는다. 두 CodeEditor가 한 모델을 공유하는 AOT 예제를 확장했다.

iiGUI TextRasterCache는 네이티브 RGB·컬러 글꼴 픽셀을 유지하고 LRU 예산을 적용한다.
투명도만 바뀌면 이미지를 재사용하고 본문/선택 RGB가 다르면 각각의 네이티브 결과를 캐시한다.
TextLine은 일반 문자와 컬러 글리프의 투명도를 래스터화 후 동일하게 적용한다. Windows
TextLine의 Direct2D는 컬러 글꼴 플래그를 사용하도록 소스를 갱신했으나 실제 Windows 빌드·실행은 미확인이다.

이 단계 기록은 build/native-text-state-evidence/와 build/native-text-state-consumer/에 둔다.
2026-10-24 macOS arm64 일반·ASan/UBSan 컴파일·링크·작업용 설치와 재배치 패키지만을
사용한 소비자·일곱 독립 갤러리의 빌드·설치를 완료했다. Qt·LVUI 런타임 파서·Cocoa 편집
컨트롤의 직접 클래스 참조, 공개 헤더·아카이브·AOT 생성 결과·리소스·문서를 정적으로
감사한다. 최신 감사 기록은 final-build-audit.json, 실행 대기 목록은 runtime-batch-pending.json이다.
새 구성을 포함한 1,932개 등록 항목은 모두 미실행이다. 신규 범위/알림 15개·편집기 9개·글리프 캐시 12개·
투명도 2개의 검사 소스를 추가했다. 이 단계에서도 앱·테스트·self-test는 실행하지 않았으며
전체 LVRS 목표의 마지막 일괄 단계까지 보류한다. 새 API·픽셀·입력·메모리 안전성의 실행
통과를 주장하지 않는다. 전역 SDK 설치와 현재 실행 중인 앱은 이 단계에서 변경하지 않는다.

## 네이티브 Tooltip 추가 단계

원본 말풍선 배치·꼬리 경로, PanelMaterial·유지 콘텐츠·스크롤, 대상 hover/press/focus 관찰과
Core 소유 지연·타임아웃·닫힘 latch를 연결했다. iiGUI는 독립 상호작용 구독과 명시적 팝업 프레임·
입력 영역·기원 캡처 보존·모달 배경 차단을 제공한다. LVUI는 같은 객체의 바인딩과 앞/뒤 targetId
연결을 생성한다. NativeTooltipGallery는 두 종류의 말풍선과 scoped backend·비동기 결과를 구성한다.

이 단계의 컴파일·설치 소비자·정적 감사 기록은 build/native-tooltip-evidence/와
build/native-tooltip-consumer/에 둔다. 모든 앱·테스트·self-test 실행은 전체 목표의 마지막
일괄 단계까지 계속 보류한다. 원본의 자동 OS 안전 영역·물리 화면 교차, 완전한 모션/상속 API·
픽셀과 플랫폼 실행은 아직 완료하지 않았으며 Tooltip은 partial로 기록한다.

2026-10-24 macOS arm64에서 일반·ASan/UBSan 컴파일·링크·작업용 설치와 재배치 패키지만을
사용한 소비자·여덟 독립 갤러리의 빌드·설치를 완료했다. 공개 헤더 139개와 범위 내 소스 객체
202개, 정적 아카이브·Core 진입점·AOT 출력·리소스·문서를 현재 소스와 정적으로 대조했다.
검사한 소비자에서 Qt·LVUI 런타임 파서·Cocoa 컨트롤의 직접 클래스 참조는 발견되지 않았다.
최신 기록은 final-build-audit.json, 실행 대기 목록은 runtime-batch-pending.json이다.

새 검사는 iiGUI 상호작용/입력 17개, Tooltip 배치/시간 변환 15개, 컨트롤러/유지 콘텐츠 19개,
LVUI AOT 1개와 갤러리 self-test이다. 여러 구성을 포함한 2,090개 등록 항목은 모두 미실행이다.
이 단계의 앱·테스트·self-test 실행 횟수는 0회이며 전체 목표의 최종 일괄 단계까지 보류했다.
ASan/UBSan 컴파일과 정적 감사는 런타임 메모리 안전성·입력·타이밍·픽셀 통과 증거가 아니다.
Core·LVUI와 provider 번들 SQLite는 계측하지 않았다. Windows/Linux 실제 대상의 빌드·실행은
이 단계에서 수행하지 않았다. 전역 설치와 현재 실행 앱은 변경하지 않았다.

대응표는 149개 선언 중 부분 80개·대기 69개·전체 계약 완료 0개이다. Tooltip의 상세 미구현
경계는 Tooltip.md와 catalog/native/parity.json에 유지하며 전체 LVRS 이식 목표는 진행 상태이다.
