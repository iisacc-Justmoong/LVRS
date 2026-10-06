# Windows 검증

Qt 6.8.3의 MinGW 키트와 Ninja를 사용하며 생성물은 `build/`에 둔다. DLL 소비자와 QML 가져오기, SVG 렌더링, 텍스트 편집 및 성능 테스트를 실제 실행한다.

누락된 `makefile`, `makefileApp`, `makefileToolWindow` 아이콘은 원본 Figma의 Iconset 페이지에서 다시 내보냈다. 세 원본 SVG의 해시는 기존 manifest의 `preResponsiveSha256`와 일치한다. 고정 크기를 반응형 루트 속성으로 변경한 현재 파일의 해시를 검증 목록에 반영하였다. 원본 노드는 `207:34377`, `207:34374`, `207:34152`이다. Makefile 패턴 때문에 다시 누락되지 않도록 Git 추적 예외를 추가하였다.

Qt 리소스는 Windows에서도 대소문자를 구분한다. 원본 파일을 사용하는 `javascript.svg`와 `nodestest.svg`의 리소스 별칭을 명시하여 manifest와 QML의 동일한 경로를 보장한다.

Windows UI 테스트는 `QT_QPA_PLATFORM=windows`로 native font engine을 사용한다. 같은 번들 Pretendard가 offscreen 대체 font engine에서 다른 폭을 보이는 현상은 실제 Windows 플러그인에서 버튼·분할 버튼·메뉴·목록 기하 계약을 모두 검증하여 구분하였다. 문서 계약은 한국어 설명서와 유지되는 API 식별자를 검사한다.

makefileApp의 원본 도형은 X축 중심이 8.5이므로 외곽 그룹에 translate(0.5 0)을 적용하여 18px 뷰포트 가운데에 배치한다. 크기별 실제 래스터 경계 검증을 유지한다.

Windows 픽셀 회귀 fixture 중 도형 골든 이미지와 ToggleSwitch shadow 캡처는 Qt 테스트 옵션 QT_ENABLE_HIGHDPI_SCALING=0으로 1x 기준을 사용한다. ToggleSwitch의 픽셀 계약은 LVRSTests_import_toggle_pixel에서 별도로 실행하며 native import 검사는 나머지 슬롯을 모두 실행한다. 별도의 toggle_switch 기능 테스트는 실제 모니터 배율로 계속 실행한다. 제품 실행의 모니터 DPI는 변경하지 않는다. 텍스트 기하에는 실제 Windows 배율을 사용하며 table 및 list 캡처는 논리 크기로 정규화하고 상태 색상은 애니메이션 완료 후 확인한다.

Windows의 네이티브 창 이동/크기 변경은 OS modal loop에 진입하므로 합성 입력 계약은 별도 offscreen 테스트로 검증하고 나머지 GUI 검사는 Windows QPA로 수행한다. 참고: https://doc.qt.io/qt-6.8/highdpi.html

성능 및 soak fixture의 1–2 ms 합성 작업은 Windows timer 해상도를 1 ms로 요청한 상태에서 측정한다. 테스트 종료 시 timeEndPeriod로 요청을 해제하며 기존 지연 상한은 유지한다. 기본 시스템 timer 간격에 의해 합성 sleep이 반올림되는 비용과 실제 큐 지연을 구분하기 위한 테스트 조건이다. 참고: https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod

ToggleSwitch shader shadow 픽셀 검증은 Basic 스타일 및 Windows D3D11 RHI로 수행한다. Qt Quick software 백엔드에서는 MultiEffect의 GPU shader가 실행되지 않으므로 해당 캡처의 백엔드를 명시한다.

Soak fixture는 각 동기 파일 쓰기·읽기·dispatch 반복 사이에 event loop를 처리한다. Windows의 300회 파일 쓰기 동안 완료 callback을 의도치 않게 보류하던 측정 문제를 제거하며, 작업 수·파일 내용·큐 배출·오류·backpressure 및 500 ms p99 상한은 그대로 검사한다.

HierarchyItem의 interaction 상태는 행의 effectiveHoverState 및 effectivePressedState에 직접 연결한다. UX 상태와 실제 배경 렌더링이 같은 입력을 사용함을 hover·선택 전환 및 instance interaction 회귀로 검증한다.

네이티브 hover fixture는 자신의 창 활성화와 실제 포인터 위치를 합성 이벤트와 일치시키고 종료 시 이전 포인터 위치를 복원한다. OS의 후속 leave 이벤트가 테스트의 합성 hover를 즉시 취소하는 상황을 구분한다.

HierarchyItem의 native fixture 좌표는 창 노출과 초기 layout이 완료된 뒤 계산한다. 초기 visible 상태만으로 좌표를 계산하던 DPI/layout 경쟁을 제거하였다.

Hold 제스처의 단발 timer는 Qt::PreciseTimer를 사용한다. 기본 CoarseTimer가 임계 시간보다 최대 5% 먼저 만료되어 60 ms hold가 조기에 시작될 수 있는 문제를 수정하였다. 실제 emitted duration이 설정된 holdThresholdMs 이상인지 검사하며 기존 상한과 입력 계약을 유지한다. Qt timer의 조기 만료 계약은 https://doc.qt.io/qt-6.8/qtimer.html#accuracy-and-timer-resolution 에서 확인하였다.
