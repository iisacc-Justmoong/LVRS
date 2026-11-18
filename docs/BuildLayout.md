# Build and resource layout

Windows SDK의 `min`·`max` 매크로가 켜진 상태에서도 계층 모델의 인덱스
상수를 선언할 수 있도록 표준 극한값 호출을 보호한다. 네이티브 테스트 빌드에는
해당 공개 헤더의 컴파일 전용 검사가 포함된다. 이번 변경의 Windows 빌드와
설치는 수행하지 않았다.

The supported CMake build directory is the repository's `build/`. The default
native and legacy configurations compile the module without examples. The
root `build.sh` adds tests by default; `--without-tests` disables them, and
`--with-examples` explicitly opts into example application targets. `--clean`
uses the configured build system's clean target and does not recursively erase
unrecognized files.

Test fixtures, temporary package installations and consumer sources belong to
the project `resources/` directory. The native package tests reserve isolated
short-lived directories in `resources/.t`, with consumer compiler output
confined to uniquely owned directories inside `build/`. They clean these
directories after use and leave the regular install manifest intact. Existing
user data and reference materials recovered from older builds are retained
under ignored resource directories; they are not inputs to the default build.

`LVRS.native_presentation_package_build` validates a relocated installed package
through compilation without starting a GUI application. The separate
`LVRS.native_presentation_package` additionally runs the installed consumer.
Installation into the user's SDK and application execution are separate actions.

레거시 계층 코덱과 텍스트 마크업의 기존 사례는 CTest 실행 파일로
유지하며 해당 타깃은 Qt Test를 링크하지 않는다. 다른 레거시 GUI 검사는
아직 Qt Test를 사용한다.

레거시 디버그 QML과 Windows 내보내기 검사도 CTest 실행기로 유지하며
Qt Test를 링크하지 않는다. Windows 내보내기 검사의 신호는 연결 성공과
전달 횟수를 직접 확인한다. 나머지 레거시 GUI 테스트는 별도 이전 대상이다.

레거시 예제의 여섯 QML 로딩 사례도 CTest 실행기로 유지하며 테스트
소스 경로에서 원래 예제 파일을 찾는다. 이벤트 대기·시간 초과와 지연 삭제는
별도 CTest 지원 검사로 검증하고 해당 타깃에서 Qt Test를 링크하지 않는다.

권한 순서 실행의 두 사례도 CTest 실행기로 유지한다. 기록 변경 신호는
연결 성공과 발생 횟수를 확인하며 Qt Test 신호 감시기에 의존하지 않는다.

앱 상태 다섯 사례와 병렬 부트스트랩·포그라운드 서비스·글꼴 정책의
각 두 사례도 CTest 실행기로 이전한다. 기존 창 생성과 글꼴 검사는 그대로
유지하므로 GUI 실행 검증은 별도 요청이 필요하다.

계층 상호작용 다섯 사례, 레이아웃 다섯 사례와 런타임 이벤트 세 사례도
CTest 실행기로 유지한다. 레이아웃과 이벤트 검사는 기존 QML·창 동작을
유지하며 GUI 실행 검증은 별도 요청이 필요하다.

QML 타입 등록의 두 사례도 CTest 실행기로 유지하며 테스트에 필요한
생성 가능·생성 불가 객체의 Qt 메타오브젝트는 계속 생성한다.

앱 진입 아홉 사례, 이벤트 리스너 여섯 사례와 페이지 라우터 12개
사례도 CTest 실행기로 유지한다. 앱 진입 시험이 만드는 임시 QML 모듈은
리소스 디렉터리에 둔다. GUI 사례는 별도 실행 요청이 필요하다.

레거시 성능 게이트와 장시간 부하 검사는 QtTest 신호 감시 대신 형식이 지정된 연결로 완료 횟수를 기록하며, 기존 시간 제한과 성능 기준을 유지한다. 부하 검사용 임시 파일은 resources에 생성한다. 두 검사는 GUI 초기화 및 실행 정책을 따로 따른다.

레거시 카탈로그의 세 사례는 CTest 실행기로 유지한다. 창 노출과 캡처 지연은 이벤트 루프로 처리하고, 캡처 디렉터리가 없을 때는 건너뛰기를 기록한다. 실행기에서 전체 선택 사례가 건너뛰어지면 CTest 코드 77을 반환한다. GUI 실행은 별도 정책을 따른다.

시각 회귀의 이미지 비교와 기준 장면 검사는 CTest로 선택한다. 렌더 완료 신호와 캡처·기준 이미지 갱신 동작을 유지하며, 차이 이미지는 `resources/ctest-visual-*`에 작성하고 검사가 실패할 때 보존한다. GUI 실행 결과는 별도로 검증해야 한다.

레거시 SVG 관리자의 아홉 사례는 CTest 실행기로 유지한다. 여섯 변경 신호의 발생 횟수, 아이콘 자산 수·해시·렌더 및 부동소수점 비교 조건을 보존한다. 임시 SVG는 resources의 격리 디렉터리에 두며 GUI 테스트 실행은 프로젝트의 별도 유예 정책을 따른다.

Windows QML 링크 검사는 기존 QGuiApplication과 LVRS 타입 생성 확인을 그대로 사용하며 불필요한 QtTest 링크를 제거한다. 실제 Windows 타깃 빌드와 실행은 해당 플랫폼 키트에서 검증해야 한다.

레거시 텍스트 문서 모델의 네 사례는 CTest 실행기로 유지한다. 완료·모델 변경 신호와 행 삽입·삭제 통지의 균형을 기록하고, 행 인덱스·역할·지속 인덱스 및 파일 저장·취소 동작을 검사한다. 각 임시 작업공간은 resources 아래에 생성한다. 이 검사의 실행 결과는 별도 빌드와 CTest 결과를 확인해야 한다.

macOS 레거시 윈도 블러의 다섯 사례는 QtTest 없이 CTest로 실행한다. Cocoa가 아닌 환경에서는 CTest 건너뛰기 코드 77을 반환한다. 클릭은 Qt 마우스 이벤트로 전달하고, 창 노출과 전체 화면 진입·해제 알림을 계속 확인한다. 실제 Cocoa 창 동작은 해당 플랫폼에서 검증해야 한다.

레거시 코드 편집기의 다섯 사례는 QtTest 없이 CTest로 구성한다. 키 누름·해제, 조합 입력, 더블 클릭 및 드래그 선택은 Qt 입력 이벤트로 전달하고 기존 텍스트·제출·선택 결과를 검사한다. 더블 클릭은 첫 클릭과 두 번째 누름·더블 클릭·해제를 순서대로 전달한다. 각 사례를 개별 CTest로도 선택할 수 있으며 실제 GUI 실행 결과는 별도로 확인해야 한다.

레거시 아바타 검사와 설치 소비자 검사는 QtTest 대신 CTest 실행기를 사용한다. 이미지 캡처 완료 신호를 직접 기록하고, 가로·세로 이미지와 네 크기 및 여섯 이미지 형식의 데이터 행을 각각 유지한다. 임시 이미지 파일은 resources의 격리된 디렉터리에 생성한다. 실제 GUI와 설치 소비자 실행은 별도 검증해야 한다.

레거시 텍스트 편집기의 일곱 사례는 CTest 실행기로 유지한다. 동적 문서 변경 신호의 텍스트·리비전과 파일 읽기·동기화 완료 신호의 경로를 직접 기록하고, 키 입력과 파일 동기화 동작을 검사한다. 임시 문서는 resources의 개별 작업공간에 생성한다. 일곱 사례는 개별 CTest로 선택할 수 있으며 GUI 실행 결과는 별도 검증해야 한다.

레거시 렌더 품질의 세 사례는 QtTest 없이 CTest로 구성한다. 열 가지 품질 변경 신호의 연결과 발생 횟수, 전역 렌더 설정과 GPU 정책을 검사하고 테스트가 바꾼 전역 상태를 복원한다. 파이프라인 캐시와 이미지 파일은 resources의 개별 작업공간에 둔다. 실제 타깃 실행 결과는 빌드와 CTest 기록을 확인해야 한다.

레거시 기본 색상 검사는 QtTest 없이 CTest로 구성한다. 여섯 색상·루트 유형 데이터 행, 바인딩 순환 경고 감지, 이미지 캡처 완료 신호와 렌더된 픽셀 검사를 유지한다. 실행기에는 경고 감지 자체를 확인하는 사례도 포함한다. GUI와 캡처 결과는 별도 CTest 실행으로 검증해야 한다.

레거시 백엔드 I/O의 여덟 사례는 QtTest 없이 CTest로 구성한다. 비동기 완료 신호의 요청 ID·작업·대상·성공 여부·결과·오류·소요 시간을 기록하고 병합·대기열 제한·취소·캐시 검사와 성능 기준을 유지한다. 임시 파일은 resources의 개별 작업공간에 두고 백엔드의 스레드 풀이 종료된 뒤 작업공간을 정리한다. 실제 실행 결과는 타깃 빌드와 CTest 기록을 확인해야 한다.

레거시 시트의 여섯 사례는 QtTest 없이 CTest로 구성한다. 캡처 완료와 닫힘·파괴 신호, 마우스 드래그·키 입력, 레이아웃과 캡처 픽셀을 그대로 검사한다. 부동소수점 기하 값은 허용 오차를 보존해 비교하며 실제 GUI 실행 결과는 CTest로 확인해야 한다.

레거시 머티리얼의 열두 사례는 QtTest 없이 CTest로 구성한다. 이미지 캡처 완료와 팝업 열기·닫기 신호를 기록하고 배경·노이즈·블러 픽셀 검사를 보존한다. 소프트웨어 렌더러가 지원하지 않는 블러 사례는 건너뛰기 코드 77로 보고한다. 실제 렌더 결과는 해당 플랫폼의 CTest에서 확인해야 한다.

레거시 탭 검사는 QtTest 없이 CTest로 구성한다. 필수 아이콘의 세 데이터 행, 활성화 인덱스, 키보드·터치 입력과 접근성·배치 검사를 유지한다. 터치 이벤트 주입은 QtGui의 테스트 보조 API를 사용하며 QtTest 라이브러리는 링크하지 않는다. 선택적 화면 캡처가 설정되지 않은 사례는 건너뛰기 코드 77로 보고한다.

레거시 모바일 내비게이션의 일곱 사례는 QtTest 없이 CTest로 구성한다. 활성화 인덱스·검색 신호, 포커스와 키보드·마우스·터치 입력, 모션 시간 제한을 유지한다. 선택적 갤러리 캡처가 없으면 건너뛰기 코드 77을 반환한다. 실제 GUI 동작은 CTest 결과로 확인해야 한다.

레거시 토글 스위치 검사는 QtTest 없이 CTest로 구성한다. 꺼짐→켜짐과 켜짐→꺼짐 데이터 행, 전환 신호, 키·마우스·터치와 취소·드래그 동작, 리바운드 시간 기준을 유지한다. 각 사례는 독립된 QML 인스턴스를 사용하고 선택적 캡처의 행 이름을 보존한다.

레거시 카드 검사는 QtTest 없이 CTest로 구성한다. 데스크톱·모바일의 카드 유형, 크기, 상세 수준, 상태 조합 74개와 마우스·키보드 및 중첩 동작, 이미지 완료 신호와 픽셀, 갤러리 검사를 보존한다. 각 데이터 행은 고유한 CTest 이름으로 실행한다.

레거시 슬라이더 검사는 QtTest 없이 CTest로 구성한다. 두 플랫폼, 일곱 유형, 네 크기와 네 상태의 224개 데이터 행, 값 변경 신호, 키·마우스·터치, 범위·스냅과 갤러리 이미지 완료 및 픽셀 검사를 유지한다. 데이터 행은 고유한 CTest 이름으로 선택할 수 있다.

레거시 플랫폼 통합의 여덟 사례는 QtTest 없이 CTest로 구성한다. 플랫폼 프로필과 모바일 테마, 안전 영역과 창 수명, 유효한 크기 조절 경계 및 Cocoa 수동 크기 조절 입력을 유지하고 플랫폼이 맞지 않는 사례는 건너뛰기 코드 77로 보고한다.

레거시 색상 선택기 검사는 QtTest 없이 CTest로 구성한다. 여섯 높이 데이터 행, 색상 변경·수락의 값과 취소·스포이드 신호, 마우스·키·터치, 입력 검증, 모달 수명 및 이미지 캡처 완료를 보존한다.

레거시 목록 합성 검사는 QtTest 없이 CTest로 구성한다. 17개 유형의 데스크톱·모바일 데이터 행 34개, 액션·편집 신호 인자, 마우스·문자·키 입력, 모델 편집 및 이미지 캡처 완료와 픽셀 검사를 유지한다.

슬라이더 갤러리의 강조색 픽셀 검사는 고정 RGB 경계 대신 해당 슬라이더의 테마 강조색을 기준으로 비교한다. 강조색과 밝은 픽셀의 기존 최소 개수는 유지하며, 색상 비교 자체의 성공·실패 경계도 별도 CTest로 검사한다.

레거시 인스턴스 상태 검사는 QtTest 없이 CTest로 구성한다. 24개 입력 수명 행과 선택 메뉴의 네 데이터 행, 클릭 신호·포커스·취소·모션 및 독립 상태 검사를 보존한다. 선택적 네이티브 캡처가 없으면 해당 사례는 건너뛴다.

레거시 툴팁 검사는 QtTest 없이 CTest로 구성한다. 아홉 배치·꼬리 행과 네 호버·클릭 행, 창·콘텐츠 수명과 지연, 터치·입력, 캡처 완료 신호 및 QML 바인딩 루프 경고 기록을 보존한다.

레거시 내비게이션·상태 모델의 17개 사례는 QtTest 없이 CTest로 구성한다. 히스토리·스택·레지스트리·상태 변경 신호를 기록하고 값 변경의 키, 현재 값 및 이전 값을 보존하며 모델과 QML 컨텍스트 계약을 검사한다.

인스턴스 상태 취소 검사는 포인터가 창 바깥에 나간 뒤 클릭 지점으로 돌아오는 이동 이벤트를 먼저 전달해, 클릭 후 호버 복귀를 실제 입력 순서로 확인한다.

레거시 런타임 서비스의 14개 사례는 QtTest 없이 CTest로 구성한다. 런타임 입력·유휴·컨텍스트 메뉴, 터치·드래그·스크롤의 이벤트 인자, 렌더링 모니터와 로거 신호 및 기존 시간 제한을 보존한다.

레거시 Figma 정합성 검사는 QtTest 없이 CTest로 구성한다. 저장된 테마·텍스트·기하 자료의 모든 데이터 행을 행 이름과 함께 실행하고 버튼·메뉴 신호 인자, 입력, 픽셀 검증 및 렌더러 조건부 건너뛰기를 보존한다.

레거시 모션 검사는 QtTest 없이 CTest로 구성한다. 버튼 종류, 입력 방식, 톤, 행 상태의 94개 데이터 행과 클릭·취소 신호, 포인터·키보드·터치 입력, 리바운드·전환 시간 조건을 유지한다.

레거시 API 가져오기 검사는 QtTest 없이 CTest로 구성한다. 기존 82개 단일 사례와 12개 데이터 행, 페인트·프레임 완료 및 활성화·버튼 신호 인자, 입력·스냅샷·플랫폼별 건너뛰기와 Windows 전용 실행 선택을 유지한다.

레거시 LVRS 테스트 구성은 Qt Test 패키지를 찾거나 테스트 타깃에 링크하지 않는다. QtGui가 제공하는 터치 주입 보조 API는 기존 입력 사례를 위해 유지한다.
