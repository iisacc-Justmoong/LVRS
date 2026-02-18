# RouteMatcher

Location: `backend/navigation/routematcher.h` / `backend/navigation/routematcher.cpp`

`RouteMatcher`는 `PageRouter`의 경로 정규화/매칭 핫패스를 C++로 이관하기 위한 QML 싱글턴이다.

## Purpose

- 경로 정규화(`normalizePath`)를 일관된 규칙으로 처리한다.
- 동적 세그먼트(`[id]`)와 rest 세그먼트(`[...path]`) 매칭을 C++로 수행한다.
- QML JS 루프에서 반복되는 문자열 분해/병합 비용을 줄인다.

## API

- `normalizePath(path): string`
  - 입력이 비어 있으면 `/`로 정규화한다.
  - 선행 `/`를 보장한다.
  - 루트(`/`)를 제외한 후행 `/`를 제거한다.
- `match(path, routePath): map`
  - 반환:
    - `matched: bool`
    - `params: map`
  - 예:
    - `("/runs/42", "/runs/[id]") -> matched=true, params.id="42"`
    - `("/logs/a/b", "/logs/[...path]") -> matched=true, params.path="a/b"`

## Notes

- `PageRouter.qml`은 우선적으로 `RouteMatcher`를 사용하고, 비가용 시 JS fallback 경로를 유지한다.
- 매칭 결과는 `PageRouter`의 route-resolve 캐시에 저장되어 반복 탐색 비용을 추가로 줄인다.

