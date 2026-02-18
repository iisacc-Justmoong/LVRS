# ApplicationWindow

Location: `qml/ApplicationWindow.qml`

`ApplicationWindow` is the LVRS root shell for adaptive layout, global interaction routing, and runtime/render policy wiring.

## 1. Scope and Role

`ApplicationWindow` owns:

- platform/size-class signals and adaptive layout state,
- scaffold navigation composition (rail/drawer/bottom modes),
- render-quality layer bridge (`RenderQuality`),
- global pressed/context events via root listeners,
- optional runtime and backend hook bootstrapping,
- optional native window styling hooks.

`ApplicationWindow` does **not** own:

- backend policy constants (RenderQuality/RuntimeEvents own those),
- route parser internals (`PageRouter` owns route matching),
- data persistence.

## 2. Startup Sequence (Operational Contract)

On `Component.onCompleted`, shell performs:

1. `FontPolicy.enforceApplicationFallback()`,
2. optional device-tier preset apply (`RenderQuality.applyDeviceTierPreset(...)`),
3. `RenderQuality.applyWindow(windowRoot)`,
4. `SvgManager.ensureMinimumScale(effectiveSupersampleScale)`,
5. runtime daemon start/attach by default (`autoAttachRuntimeEvents`, default: `globalEventListenersEnabled`),
6. optional backend mirror hook (`autoHookBackendUserEvents`, default `false`),
7. native window style application and deferred refresh.

Order matters because:

- render-layer sizing depends on active render-quality policy,
- runtime hook path expects daemon availability and attached window context.

## 3. Core Property Groups

### 3.1 Platform and size-class

| Property | Meaning |
|---|---|
| `platform` | OS token from `Qt.platform.os`. |
| `isMobilePlatform`, `isDesktopPlatform` | Platform group flags. |
| `widthClass`, `heightClass` | Compact/medium/expanded classification. |
| `isCompact`, `isExpanded` | Combined class helpers. |

### 3.2 Layout and scaffold

| Property | Meaning |
|---|---|
| `navItems` | Source nav model for scaffold delegates. |
| `navIndex`, `navigationEnabled` | Current navigation selection and enable-state. |
| `scaffoldLayoutMode` | `auto/mobile/desktop` preference hint. |
| `adaptiveLayoutProfile` | Resolved profile (`mobile-compact`, `desktop-wide`, etc.). |
| `adaptiveNavigationMode` | Resolved navigation mode (`rail`, `drawer`, `bottom`, `none`). |

### 3.3 Render-quality bridge

| Property | Meaning |
|---|---|
| `effectiveSupersampleScale` | Mirrors backend effective policy value. |
| `sceneSupersamplingActive` | Backend-computed activation flag for scene layer supersampling. |
| `inactiveRenderDowngradeEnabled` | Hidden/minimized 윈도우 렌더 비용 강등 정책 토글. |
| `inactiveRenderMsaaSamples` | 강등 모드에서 사용할 MSAA 샘플 목표값. |
| `autoApplyDeviceTierPreset` | 시작 시 디바이스 등급 프리셋 자동 적용 여부. |
| `forcedDeviceTierPreset` | 프리셋 강제값(`-1`은 자동 탐지). |
| `pageRouterRetainInactivePages` | 내부 `PageRouter`가 유지할 비활성 페이지 깊이. |
| `pageRouterCacheCapacity` | 내부 `PageRouter` route-resolve 캐시 용량. |
| supersample host `layer.textureSize` | Resolved through `RenderQuality.resolveLayerTextureSize(...)`. |
| supersample host `layer.mipmap` | `RenderQuality.mipmapEnabled` 정책을 직접 반영. |

### 3.4 Runtime/global event bridge

| Property/Signal | Meaning |
|---|---|
| `globalEventListenersEnabled` | Master switch for global listener components. |
| `autoAttachRuntimeEvents` | Auto-start/attach runtime daemon (default: follows `globalEventListenersEnabled`). |
| `autoHookBackendUserEvents` | Auto-hook backend mirror cache (default `false`, opt-in). |
| `globalPressedEvent(eventData)` | Root global press relay. |
| `globalContextEvent(eventData)` | Root global context-request relay. |
| `lastGlobalPressedEventData`, `lastGlobalContextEventData` | Last relayed payload snapshots. |

## 4. Adaptive Layout State Behavior

Internal adaptive host enforces guarded transitions for:

- layout profile transitions (`mobile-compact`, `mobile-wide`, `desktop-compact`, `desktop-wide`),
- navigation mode transitions (`rail`, `drawer`, `bottom`, `none`).

Guard logic:

- disallows abrupt incompatible transitions in one step,
- emits transition rejection metadata,
- schedules follow-up convergence (`Qt.callLater`) when intermediate state is needed.

This prevents unstable oscillation during rapid resize/device-class changes.

## 5. Render Path Cases

| Case | `sceneSupersamplingActive` | Expected layer behavior |
|---|---|---|
| Small window under pixel budget | true | supersample layer active; texture size scaled by backend policy |
| Large window over budget | false | layer remains unscaled base size |
| RenderQuality disabled by build variant | false/1x | graceful fallback to base texture sizing |
| Window not yet applied | false until apply | call `RenderQuality.applyWindow` |

## 6. Runtime/Event Cases

| Case | Condition | Expected behavior |
|---|---|---|
| Runtime auto attach enabled (default) | `globalEventListenersEnabled=true` and `autoAttachRuntimeEvents=true` | daemon starts and attaches to this window |
| Backend auto hook enabled (opt-in) | `autoHookBackendUserEvents=true` | `Backend.hookUserEvents()` attempted and logged on failure |
| Global listener disabled | `globalEventListenersEnabled=false` | global pressed/context relays stop |
| Context event with no coordinates | malformed payload | context menu helpers should no-op safely |

## 7. Recommended Integration Patterns

### 7.1 Production shell baseline

```qml
import LVRS 1.0 as LV

LV.ApplicationWindow {
    visible: true
    width: 1280
    height: 800

    globalEventListenersEnabled: true
}
```

### 7.2 Navigation with internal page stack

```qml
import LVRS 1.0 as LV

LV.ApplicationWindow {
    useInternalPageStack: true
    pageRoutes: [
        { path: "/", component: homePage },
        { path: "/reports", component: reportsPage }
    ]
}
```

## 8. Common Pitfalls

- Enabling route definitions but disabling internal page stack unintentionally.
- Replacing render-layer sizing math in-page instead of using backend resolver.
- Expecting global context signals while root listeners are disabled.
- Assuming runtime daemon is always attached even after manually setting `autoAttachRuntimeEvents=false`.
- Assuming backend mirrored cache is always active without enabling `autoHookBackendUserEvents`.

## 9. Troubleshooting Matrix

| Symptom | Likely Cause | Verification | Action |
|---|---|---|---|
| adaptive mode flickers on resize | conflicting manual state writes | inspect layout profile transitions | let adaptive host own profile transitions |
| context menu not appearing | missing global context payload or listener disabled | inspect `lastGlobalContextEventData` | enable listeners, verify payload coordinates |
| render quality no effect | window policy not applied | check startup logs/order | ensure `RenderQuality.applyWindow` runs at completion |
| backend hook fails | runtime unavailable or hook error | inspect `Backend.lastError` | ensure runtime singleton loaded and attached |

## 10. Codex-Oriented Playbook

### 10.1 Safe Codex patch boundaries

When editing `ApplicationWindow.qml`, prefer:

1. keep adaptive state machine semantics intact,
2. keep render-quality math in backend API calls,
3. keep runtime hook wiring behind existing feature flags,
4. keep signal payload contracts stable.

### 10.2 Codex anti-patterns

- Do not inline new supersample formulas in multiple delegates.
- Do not directly mutate internal adaptive host private transition flags unless required.
- Do not hard-wire backend hooks to always-on if feature flags exist.

### 10.3 Codex regression checklist

After patching shell behavior:

1. verify adaptive layout still converges after fast resize,
2. verify global pressed/context signals still emit,
3. verify supersample host binds to backend-resolved texture size,
4. verify runtime auto-attach and backend auto-hook flags still work independently.

## 11. Validation Checklist

- Startup sequence ordering remains unchanged for policy-sensitive operations.
- Adaptive transitions remain deterministic under compact/wide boundary changes.
- Render layer toggles by backend flag, not duplicated local math.
- Global event relays and runtime flags behave independently as documented.

## 12. Related APIs

- `RenderQuality` (`docs/backend/RenderQuality.md`)
- `RuntimeEvents` (`docs/backend/RuntimeEvents.md`)
- `Backend` (`docs/backend/Backend.md`)
- `PageRouter` component docs
