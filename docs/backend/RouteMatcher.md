# RouteMatcher

Location: `backend/navigation/routematcher.h` / `backend/navigation/routematcher.cpp`

`RouteMatcher` is a QML singleton that moves `PageRouter` path normalization/matching hot paths into C++.

## Purpose

- Process path normalization (`normalizePath`) with consistent rules.
- Match dynamic segments (`[id]`) and rest segments (`[...path]`) in C++.
- Reduce repeated string split/join overhead in QML JavaScript loops.

## API

- `normalizePath(path): string`
  - normalize empty input to `/`
  - ensure leading `/`
  - remove trailing `/` except for root (`/`)
- `match(path, routePath): map`
  - returns:
    - `matched: bool`
    - `params: map`
  - examples:
    - `("/runs/42", "/runs/[id]") -> matched=true, params.id="42"`
    - `("/logs/a/b", "/logs/[...path]") -> matched=true, params.path="a/b"`

## Notes

- `PageRouter.qml` prefers `RouteMatcher` and keeps a JavaScript fallback path when unavailable.
- Match results are stored in the `PageRouter` route-resolve cache to further reduce repeated resolution cost.
