# RouteMatcher

위치: `src/backend/navigation/routematcher.h` / `src/backend/navigation/routematcher.cpp`

`RouteMatcher`는 `PageRouter` 경로 정규화/일치 핫 경로를 C++로 이동하는 QML 싱글턴입니다.

<a id="purpose"></a>

## 목적

- 일관된 규칙을 사용한 프로세스 경로 정규화(`normalizePath`).
- C++에서 동적 세그먼트(`[id]`)와 나머지 세그먼트(`[...path]`)를 일치시킵니다.
- QML JavaScript 루프에서 반복되는 문자열 분할/조인 오버헤드를 줄입니다.

## API

- `normalizePath(path): string`
  - 빈 입력을 `/`로 정규화
  - 최고의 `/` 보장
  - 루트(`/`)를 제외하고 후행 `/`를 제거합니다.
- `match(path, routePath): map`
  - 반환:
    - `matched: bool`
    - `params: map`
  - 예:
    - `("/runs/42", "/runs/[id]") -> matched=true, params.id="42"`
    - `("/logs/a/b", "/logs/[...path]") -> matched=true, params.path="a/b"`

<a id="notes"></a>

## 메모

- `PageRouter.qml`는 `RouteMatcher`를 선호하며 사용할 수 없는 경우 JavaScript 대체 경로 경로를 유지합니다.
- 일치 결과는 `PageRouter` 경로 해결 캐시에 저장되어 반복 해결 비용을 더욱 절감합니다.
