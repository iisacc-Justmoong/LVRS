# LVRS 네이티브 구현 작업 지침

이 저장소에는 기존 Qt/QML 구현과 LVUI·iiGUI 기반 C++23 구현이 함께 있다.
네이티브 구현의 기준은 기존 소스 전체이며 부분 이식을 완료로 표시하지 않는다.

- 네이티브 공개 API와 구현은 `src/LVRS/`에 같은 이름의 헤더·소스 쌍으로 둔다.
- LVUI는 AOT 생성기이며 최종 앱의 수명과 실행 파일 진입점은 iiSDKCore가 소유한다.
- iiGUI는 자체 C++ 위젯을 그린다. OS 위젯이나 Qt 컨트롤로 네이티브 구성 요소를 대체하지 않는다.
- 문서·테스트를 함께 갱신한다. README.md를 제외한 docs/ 내부 문서는 한국어로 작성한다.
- 빌드 출력 디렉터리의 이름은 `build/`로 한다. 작업용 설치 패키지도 Workspace 안에 둔다.
- 현재 작업의 사용자 지시에 따라 앱·테스트·self-test 실행은 전체 작업의 최종 일괄 단계에 모은다.
  중간 컴파일·링크·코드 생성·정적 검사는 실행 통과 증거와 구분한다.
- 새 독립 앱 예제는 `example/` 아래에 두며 앱 소스는 그 예제의 `App/`에 둔다.
  기존 `examples/`와 레거시 예제는 별도 요청 없이 이동하거나 삭제하지 않는다.
- `catalog/native/parity.json`과 `docs/native/Parity.md`는 원본 선언·지원 범위·남은 계약을 기록한다.
  원본과 상속 API, 시각 상태, 플랫폼과 실행 검증을 모두 충족하기 전에는 complete로 표시하지 않는다.

## 보존한 AGENTS.md 프로젝트 지침

# LVRS 저장소 작업 지침

현재 작업 지침은 [AGENT.md](AGENT.md)에 정리한다. 사용자에게서 직접 받은 최신 지침을
우선 적용한다. 네이티브 LVRS는 LVUI AOT·iiGUI 자체 C++ 위젯·iiSDKCore 앱 런타임으로
구축하며, 기존 Qt/QML 소스는 전체 동작의 비교 기준으로 보존한다.

기존 Qt 중심 안내에 있던 “자동 테스트 없음”, “Git 저장소 아님”, “Qt 위젯 우선”은 현재
네이티브 구현에 적용하지 않는다. 빌드·문서·테스트·설치 소비자의 정적 검증과 실제 실행
증거를 구분한다. 현재 사용자 요청에 따라 실행 검증은 전체 작업의 마지막에 일괄 수행한다.

<!-- workspace-docs:agent:start -->
## 문서 구조와 에이전트 탐색

- 프로젝트 루트의 `README.md`는 사람용 영문 진입 문서이며 `AGENT.md`는 에이전트 진입점이다.
- 하위 인간용 Markdown과 구조화된 agent JSON을 함께 유지한다. Markdown이 원본이며 JSON은 원문 해시와 줄 범위를 가진 파생 문서이다.
- 먼저 [docs/agent-index.json](docs/agent-index.json)에서 관련 문서와 JSON 투영을 찾는다. 원문 SHA-256이 다르면 현재 Markdown을 읽고 투영을 갱신한다.
- JSON의 규칙 후보는 검색용 어휘 일치 결과이다. 과거 기록·예제·실패 상태를 현재 지시나 검증 완료로 자동 해석하지 않는다.
- 기존 프로젝트별 규칙, 동결된 증거, 라이선스, 외부 원본, 사용자 데이터와 다른 작업의 변경을 보존한다.
- 이 문서 재편 지침은 과거 작업의 기능 구현·배포를 자동 재개하라는 요청이 아니다. 현재 사용자의 작업 범위를 우선한다.
- 문서를 수정하면 JSON 쌍·색인을 함께 갱신하고 검증한다. 기존 소스 변경에 대한 빌드·테스트 규칙은 계속 따른다.

Workspace에서 이 프로젝트의 문서를 갱신하는 명령은 다음과 같다.

```sh
python3 ../../Engineering/WorkspaceDocumentation/App/workspace_docs.py --workspace ../.. sync --project SDK/LVRS
```
<!-- workspace-docs:agent:end -->
