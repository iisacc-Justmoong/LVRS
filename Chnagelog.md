# Chnagelog

## 2026-02-14 (토)

요약이다. 어제(2026-02-14) 하루 동안 총 23개의 커밋이 있었고, QML 앱 구성 자동화, 렌더링 백엔드 정책 강화, 런타임 이벤트 콘솔 전환, 대규모 아이콘셋 추가, UI 테마/톤 명명 정리, 신규 컴포넌트(Hierarchy 등) 도입이 한날에 집중적으로 반영되었다. 주요 변경은 `CMakeLists.txt`, `cmake/`, `backend/runtime/`, `qml/`, `docs/`, `resources/iconset/`에 분산되어 있다.

변경 범위 개요이다.
- 빌드/설치: QML 앱 구성 함수(`lvrs_configure_qml_app`, `lvrs_add_qml_app`) 추가와 설치 스크립트 확장, 정적 플러그인 처리/모듈 탐지 보강.
- 그래픽스 백엔드: Vulkan 강제 및 런타임 검증, macOS/iOS Metal 강제 정책 추가, Vulkan bootstrap 유틸리티 도입.
- 런타임 이벤트/콘솔: RuntimeEvents 기능 확대, 입력 상태 추적 강화, Runtime Event Daemon Console로 전환.
- UI/QML: Main.qml 구조 재편, Hierarchy 컴포넌트 추가, 버튼/메뉴/알림 스타일 정비, 테마 색상 네이밍 전면 정리.
- 리소스: 대규모 아이콘셋 추가 및 기존 아이콘 리네임.
- 문서/예제/테스트: 문서 대규모 보강 및 예제/테스트 업데이트.

커밋별 변경 추적이다. (각 커밋의 diff 통계를 확인하여 요약함)
- d1d20717: QML 앱 구성 자동화(`lvrs_add_qml_app`)와 설치 스크립트 확장. CMake 도우미/정적 타깃 템플릿 추가, 문서 갱신, 런타임 서비스 테스트 보강.
- e16cccd: 원격 master 병합 커밋. LICENSE 변경 포함.
- 0d2060d: `lvrs_configure_qml_app` 도입, QML 예제 프로젝트 구성 정비, LVRSConfig 확장, 문서/예제 정리.
- bedbbb3: LICENSE 업데이트.
- 22a13b0: 플랫폼별 렌더링 백엔드 선택 정책 강화, Vulkan 검증 확대, macOS/iOS Metal 강제 정책 문서 반영.
- 4465254: 기존 `Main.qml` 기반 카탈로그 제거 및 VisualCatalog로 이관, AppBootstrap/AppEntry 추가, 디버그 로거 확장, 다수 테스트/문서 갱신.
- f9100c0: 이벤트 모니터 데이터 구조를 `ListModel`로 전환하여 성능/관리성 개선.
- 06478ff: `LVRS`를 `LVRSCore`로 리팩터링, 이벤트 모니터 기능 추가, 컴포넌트 연계 강화.
- e8d405d: 새로운 QML 컨트롤 다수 추가, RuntimeEvents/Backend 확장, 이벤트 파이프라인 및 렌더링 정책 문서 추가.
- db3b178: RuntimeEvents 입력 상태 추적을 상세화하고 EventListener 연동 확대, 테스트 보강.
- 0cd35a3: Design System Console를 Runtime Event Daemon Console로 교체, 실시간 모니터링/필터링/요약 기능 추가.
- dd3f32a: Hierarchy 컴포넌트 추가 및 버튼 스타일 일관화, RuntimeEvents/Alert/ContextMenu 다수 수정.
- 4d72b86: Hierarchy 초기 추가 및 관련 컴포넌트/문서 업데이트.
- 954bfd8: TextEditor/ContextMenu/Alert 등 UI 스타일 정비와 아이콘 색상 표준화.
- 058d6fa: Vulkan 지원 강제 옵션과 Qt Vulkan 기능 탐지 로직 추가.
- 208c807: Vulkan bootstrap 유틸리티 도입, 메인/예제에서 중복 로직 제거.
- 7e12fc4: 메인과 예제에서 Vulkan 백엔드 강제, Apple GL 링크 처리 변경.
- 2ea2160: 네임스페이스 `UIF` -> `LV` 전면 전환.
- 8cb923f: `Main.qml`/`Theme.qml`의 accent 색상 명칭을 사람이 이해하기 쉬운 이름으로 리팩터링.
- 92f8615: `Accent` 톤을 `Primary`로 변경하고 관련 문서/예제/QML 전반 갱신.
- 1825f4b: `Theme.qml`에서 `accent` -> `primary` 명칭 변경 및 팔레트 확장.
- e706ad1: 아이콘셋 추가 및 리네임.
- 315dd82: 대규모 아이콘 리소스 추가.

핵심 파일 변동 포인트이다.
- `CMakeLists.txt`: Vulkan 강제/검증 옵션, QML 앱 구성 함수 통합, 플랫폼 런타임 옵션 정비.
- `cmake/LVRSHelpers.cmake`, `cmake/LVRSConfig.cmake.in`, `cmake/LVRSTargetsStatic.cmake.in`, `cmake/LVRSAppEntryPoint.cpp.in`: QML 앱 자동화와 정적 플러그인 처리 지원.
- `backend/runtime/`: `vulkanbootstrap` 도입, `appbootstrap`/`appentry` 추가, `runtimeevents` 확장.
- `qml/Main.qml`: Runtime Event Console로 전환 및 모니터링 UI 대규모 재구성.
- `qml/components/navigation/Hierarchy*.qml`: Hierarchy 트리 네비게이션 구성 추가.
- `qml/components/control/*`, `qml/components/surfaces/*`: 버튼/알림/입력 컴포넌트 스타일 및 동작 정비.
- `resources/iconset/`: 신규 아이콘 대량 추가 및 기존 아이콘 리네임.
- `docs/`: 빌드/백엔드/컴포넌트 문서 대규모 수정 및 신규 문서 추가.
- `install.sh`: 설치 워크플로우 옵션 확장 및 스냅샷 지원.

검증 메모이다.
- 오늘 작업은 커밋 로그와 diff 통계를 기준으로 추적되었고, 개별 커밋의 변경 파일과 규모를 확인했다.
- 자동 테스트 프레임워크가 없으므로, 필요 시 수동 빌드(`cmake -S . -B build`, `cmake --build build`)와 실행(`./build/LVRS`)으로 정상 동작을 확인하는 단계가 후속으로 필요하다.
