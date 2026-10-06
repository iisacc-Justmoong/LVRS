<a id="changelog"></a>

# 변경 내역

<a id="2026-02-15--2026-02-28-sun-sat"></a>

## 2026-02-15 ~ 2026-02-28 (일~토)

요약: 2026-02-15 에서 2026-02-28까지 총 61 커밋이 누적되었습니다. 주요 주제는 설치 흐름을 Rust CLI 로의 이주, 다 플랫폼 부트스트랩 동작의 정제, QML 컴포넌트의 호환성 리팩토링, 라우팅/계층 구조 탐색 성능 개선 및 P4 품질 자동화의 확장입니다. 변경 사항은 `rust-cli/`, `install.sh`, `cmake/`, `qml/components/`, `tests/`, 및 `docs/` 에 집중되었습니다.

범위 개요:
- Install/부트스트랩: `install.sh`는 Rust CLI 래퍼(`lvrs install`)로 변환되었으며 `bootstrap`/`doctor`/`platform` 명령이 추가되었습니다.
- 다중 플랫폼 정책: WASM를 포함한 부트스트랩 대상 및 내보내기/실행 흐름이 강화되었으며, Qt/SDK/NDK/emsdk에 대한 자동 힌트 주입이 개선되었습니다.
- QML/Components: 입력/확인/버튼/표면/탐색 구성 요소에 호환성 리팩토링이 적용되었으며 `hasChildItems`를 기반으로 계층 구조 갈매기형 표시가 수정되었습니다.
- 라우팅/상태: `RouteResolver`가 도입되었으며 `HierarchyList` 상태 관리는 더 깔끔한 경로 확인/캐시 동작 및 트리 상호 작용을 위해 최적화되었습니다.
- 품질/관찰 가능성: P4 품질 자동화 스크립트(성능/시각적/담금/소독제) 및 관련 tests/baseline 문서가 확장되었습니다.
- 플랫폼 호환성: Apple AGL 경고 경로 및 아이콘 집합 구조 주변을 반복적으로 정리하여 build/runtime 소음을 줄였습니다.

주요 커밋 그룹(대표 커밋):
- 2026-02-15 플랫폼 부트스트랩 확장: `9c31546`, `d9a189e`, `cfc6aeb`, `fefebfa`, `ed431a0`, `4646fa2`, `85fefb1`, `e93699c`.
- 2026-02-16 대형 UI/docs 정리: `f81f159`, `74526ca`, `db79cec`, `39f0fff`, `b6a7d43`, `eb8edb2`.
- 2026-02-18 품질 자동화 및 런타임 성능 관찰 가능성: `4bb8fe5`, `eb4b1bd`, `8dada65`, `39b43fe`, `f1bd142`.
- 2026-02-19 라우팅/계층 구조 성능 개선: `a77d21b`, `b8f846a`, `628e2b1`, `ebfc355`.
- 2026-02-21 아이콘 세트/플랫폼 경고 정리: `cf892bb`, `b18271e`, `3f73532`.
- 2026-02-28 전체 Rust CLI 채택 및 호환성 리팩터링: `868a0a7`, `e355f24`, `5823419`, `8516dd3`, `dbd539d`, `953e78b`.

주요 파일 터치포인트:
- `rust-cli/src/*`, `rust-cli/README.md`: 신규 및 확장 설치/부트스트랩/진단 CLI.
- `install.sh`: 셸 기반 설치 프로그램에서 CLI 래퍼 진입점으로 이동되었습니다.
- `CMakeLists.txt`, `cmake/LVRSHelpers.cmake`, `cmake/LVRSConfig.cmake.in`: 강화된 QML 모듈 메타데이터 호환성, 프레임워크 부트스트랩 대상 및 플랫폼 힌트/레지스트리 전파.
- `qml/components/control/*`, `qml/components/navigation/*`, `qml/components/surfaces/*`, `qml/ApplicationWindow.qml`: 호환성 리팩토링 및 쉐브론/입력 동작 조정.
- `tests/ci/*`, `tests/tst_*`: P4 품질 게이트 및 회귀/성능 범위가 확장되었습니다.
- `docs/*`: build/architecture/component 문서를 강화하고 API 설명을 개선했습니다.

확인 참고사항:
- 이 섹션은 `git log --since=2026-02-15` 및 커밋당 변경된 파일 목록(`--name-only`)에서 집계 및 요약되었습니다.
- 현재 설치 동작은 `install.sh`와 `rust-cli/src/commands/{install,bootstrap}.rs` 사이에서 교차 점검되었습니다.

<a id="2026-02-14-sat"></a>

## 2026-02-14 (토)

요약: 2026-02-14에 23 개의 커밋이 생성되었습니다. QML 앱 구성 자동화, 렌더링 백엔드 정책 강화, 런타임 이벤트 콘솔 전환, 대형 아이콘 세트 추가, UI 테마/톤 명명 정리 및 새로운 컴포넌트 소개(계층 구조 포함) 는 하루 동안 집중적으로 수행되었습니다. 주요 변경 사항은 `CMakeLists.txt` , `cmake/` , `backend/runtime/` , `qml/` , `docs/` , 및 `resources/iconset/` 에 걸쳐 분산되었습니다.

범위 개요:
- 빌드/설치: QML 앱 구성 기능 추가(`lvrs_configure_qml_app`, `lvrs_add_qml_app`), 설치 스크립트 확장 및 정적 플러그인/모듈 감지 개선.
- 그래픽 백엔드: Vulkan 시행/런타임 유효성 검사, macOS/iOS Metal 시행 정책 및 Vulkan 부트스트랩 유틸리티가 추가되었습니다.
- 런타임 이벤트/콘솔: RuntimeEvents 기능 확장, 입력 상태 추적 개선, 런타임 이벤트 데몬 콘솔로 전환.
- UI/QML: `Main.qml`를 재구성하고 계층 구성 요소를 추가하고 세련된 버튼/메뉴/경고 스타일을 지정하고 테마 색상 이름을 전체적으로 정리했습니다.
- 리소스: 큰 아이콘 세트를 추가하고 기존 아이콘의 이름을 변경했습니다.
- 문서/예제/테스트: 광범위한 문서 강화 및 예제/테스트 업데이트.

커밋별 추적(각 커밋의 diff 통계에서 요약됨):
- d1d20717: QML 앱 구성 자동화(`lvrs_add_qml_app`) 및 설치 스크립트 확장. CMake 도우미/정적 대상 템플릿을 추가하고 문서를 업데이트하고 런타임 서비스 테스트를 개선했습니다.
- e16cccd: 원격 마스터 브랜치에서 병합합니다. LICENSE 변경 사항을 포함합니다.
- 0d2060d: `lvrs_configure_qml_app` 도입, QML 예제 프로젝트 설정 재구성, LVRSConfig 확장 및 docs/examples. 정리
- bedbbb3: LICENSE 업데이트.
- 22a13b0: 플랫폼별 렌더링 백엔드 선택 정책을 강화하고, Vulkan 검증을 확장하고, macOS/iOS Metal 시행을 문서화했습니다.
- 4465254: 이전 `Main.qml` 기반 카탈로그를 제거하고 VisualCatalog로 마이그레이션하고 AppBootstrap/AppEntry를 추가하고 디버그 로거를 확장하고 많은 tests/docs.를 업데이트했습니다.
- f9100c0: 향상된 성능/관리성을 위해 이벤트 모니터 데이터 구조를 `ListModel`로 변환했습니다.
- 06478ff: `LVRS`를 `LVRSCore`로 리팩터링하고 이벤트 모니터 기능을 추가했으며 구성 요소 통합을 강화했습니다.
- e8d405d: 많은 새로운 QML 컨트롤을 추가하고, RuntimeEvents/백엔드를 확장하고, 이벤트 파이프라인/렌더링 정책 문서를 추가했습니다.
- db3b178: RuntimeEvents 입력 상태 추적을 더욱 상세하게 만들고 EventListener 통합을 확장했으며 테스트를 강화했습니다.
- 0cd35a3: Design System Console을 런타임 Event Daemon Console로 교체하고 실시간 모니터링/필터링/요약 기능을 추가했습니다.
- dd3f32a: 계층 구조 구성 요소 및 버튼 스타일 일관성 업데이트가 추가되었으며 광범위한 RuntimeEvents/Alert/ContextMenu 변경 사항이 추가되었습니다.
- 4d72b86: 초기 계층 추가 및 관련 구성요소/문서 업데이트.
- 954bfd8: TextEditor/ContextMenu/Alert 및 아이콘 색상 표준화에 대한 스타일 개선.
- 058d6fa: Vulkan 시행 옵션 및 Qt Vulkan 기능 감지 논리가 추가되었습니다.
- 208c807: Vulkan 부트스트랩 유틸리티를 도입하고 기본/예제에서 중복된 논리를 제거했습니다.
- 7e12fc4: 기본/예제에 Vulkan 백엔드를 적용하고 Apple GL 링크 처리를 변경했습니다.
- 2ea2160: `UIF`에서 `LV`로 전체 네임스페이스 마이그레이션.
- 8cb923f: `Main.qml`/`Theme.qml`의 강조 색상 이름을 더 명확한 이름으로 리팩터링했습니다.
- 92f8615: `Accent` 톤의 이름을 `Primary`로 변경하고 관련 docs/examples/QML를 광범위하게 업데이트했습니다.
- 1825f4b: `Theme.qml`에서 `accent`를 `primary`로 이름을 바꾸고 팔레트 정의를 확장했습니다.
- e706ad1: 아이콘 세트 추가 및 이름 변경.
- 315dd82: 대형 아이콘 리소스 추가.

주요 파일 터치포인트:
- `CMakeLists.txt`: Vulkan 시행/검증 옵션, 통합 QML 앱 구성 기능 및 런타임 플랫폼 옵션 정리.
- `cmake/LVRSHelpers.cmake`, `cmake/LVRSConfig.cmake.in`, `cmake/LVRSTargetsStatic.cmake.in`, `cmake/LVRSAppEntryPoint.cpp.in`: QML 앱 자동화 및 정적 플러그인 처리 지원.
- `backend/runtime/`: `vulkanbootstrap`를 도입하고 `appbootstrap`/`appentry`를 추가하고 `runtimeevents`를 확장했습니다.
- `qml/Main.qml`: 런타임 이벤트 콘솔로 전환되었으며 모니터링 UI가 크게 재구성되었습니다.
- `qml/components/navigation/Hierarchy*.qml`: 계층 트리 탐색 구성이 추가되었습니다.
- `qml/components/control/*`, `qml/components/surfaces/*`: 버튼/경고/입력 구성 요소에 대한 스타일 및 동작이 개선되었습니다.
- `resources/iconset/`: 대규모 새 아이콘 추가 및 기존 아이콘 이름 변경.
- `docs/`: 광범위한 build/backend/component 문서 업데이트 및 새 문서.
- `install.sh`: 확장된 설치 워크플로 옵션 및 스냅샷 지원.

확인 참고사항:
- 커밋 로그 및 diff 통계를 사용하여 작업을 추적했으며 커밋당 변경된 파일과 변경 크기를 검토했습니다.
- 자동화된 테스트 프레임워크가 전역적으로 구성되지 않기 때문에 필요한 경우 수동 build/run 검증이 필요할 수 있습니다(`cmake -S . -B build`, `cmake --build build`, `./build/LVRS`).
