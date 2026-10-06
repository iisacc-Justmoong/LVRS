<a id="repository-guidelines"></a>

# 저장소 지침

<a id="project-structure--module-organization"></a>

## 프로젝트 구조 및 모듈 구성
- `src/main.cpp`는 Qt 애플리케이션을 부팅하고 QML 모듈 진입점을 로드합니다.
- `src/backend/`에는 C++ 싱글턴 및 QML에 노출된 도우미(예: `Backend`, `Platform`, `RenderMonitor`)가 포함되어 있습니다.
- `src/qml/`에는 UI 소스가 포함되어 있습니다. `src/qml/Main.qml`는 앱 창 루트입니다.
- `src/qml/components/`는 우려 사항으로 분할됩니다.
  - `buttons/` (e.g., `AbstractButton.qml`, `LabelButton.qml`)
  - `layout/` (e.g., `VStack.qml`, `HStack.qml`)
  - `navigation/` (e.g., `PageRouter.qml`, `Link.qml`)
  - `surfaces/` (e.g., `AppCard.qml`)
- `CMakeLists.txt`에는 소스 및 QML 파일을 어셈블하기 위한 디렉터리별 `CMakeLists.txt`가 포함되어 있습니다.
- `build/`는 로컬 빌드 출력 폴더입니다(생성됨, 직접 편집하지 않음).

<a id="build-test-and-development-commands"></a>

## 빌드, 테스트 및 개발 명령
- 구성: `cmake -S . -B build`는 `./build`에서 빌드 파일을 생성합니다.
- 빌드: `cmake --build build`는 C++ 대상과 QML 모듈을 컴파일합니다.
- 실행: `./build/LVRS`가 Qt 퀵 UI를 실행합니다.

_아직 자동화된 테스트가 설정되지 않았습니다._

<a id="coding-style--naming-conventions"></a>

## 코딩 스타일 및 명명 규칙
- 들여쓰기: C++ 및 QML에 대한 4 공백(기존 파일과 일치)
- QML 구성 요소: `PascalCase` 파일 이름(예: `AppHeader.qml`).
- 속성 및 ID: `camelCase`(예: `headerTitle`, `contentWrap`).
- 가능한 경우 UI 색상과 간격을 구성 요소 내에서 중앙 집중화하십시오.
- Qt 퀵 컨트롤 2 유형(`ApplicationWindow`, `ToolBar`, `Drawer`)을 선호합니다.
- QML 구성 요소에는 각 파일 끝에 짧은 API 사용법 설명이 포함되어 있습니다.

<a id="navigation--routing"></a>

## 탐색 및 경로
- `PageRouter`(QML 싱글턴)는 Svelte와 유사한 경로를 사용하여 StackView 기반 라우팅을 제공합니다.
  - 정적: `/reports`
  - 매개변수: `/runs/[id]`
  - 나머지: `/logs/[...path]`
- `Link` 구성 요소는 HTML `<a>`를 모방합니다. `href` 및 `router`를 설정하고 하위 콘텐츠를 래핑합니다.
- `ApplicationWindow`는 적응형 내비게이션을 직접 구동하며 `navItems`에 `path`가 포함되고 `pageRouter`가 설정된 경우 경로를 지정할 수 있습니다.

<a id="backend-notes"></a>

## 백엔드 노트
- `RenderMonitor`(QML 싱글턴)는 `QQuickWindow`에 연결되어 FPS 및 프레임 타이밍을 보고합니다.

<a id="testing-guidelines"></a>

## 테스트 지침
- 테스트 프레임워크가 구성되지 않았습니다. 테스트를 추가하는 경우 다음을 문서화하세요.
  - 프레임워크 선택(예: Qt 테스트)
  - 실행 방법(`ctest` 등).
  - 명명 규칙(예: `test_*.cpp`)

<a id="commit--pull-request-guidelines"></a>

## 커밋 및 풀 요청 지침
- 이 저장소는 현재 Git 저장소가 아닙니다. 커밋 기록이 없습니다.
- Git를 초기화하는 경우 명확한 작업 지향 커밋 메시지(예: "Refine adaptive layout behavior")를 사용하세요.
- PR의 경우 다음을 포함합니다.
  - UI/동작 변경 사항에 대한 간략한 요약입니다.
  - UI 업데이트에 대한 스크린샷 또는 녹음입니다.
  - 모든 빌드 또는 플랫폼 참고 사항(Qt 버전, OS)

<a id="configuration-notes"></a>

## 구성 참고사항
- `QtQuick` 및 `QtQuickControls2`와 함께 Qt 6.5+가 필요합니다.
- 생성된 빌드 아티팩트를 소스 제어 외부에 유지하세요(예: `build/`).
