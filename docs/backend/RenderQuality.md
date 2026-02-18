# RenderQuality

Location: `backend/runtime/renderquality.h` / `backend/runtime/renderquality.cpp`

`RenderQuality` is the low-level rendering-quality policy singleton for LVRS.  
Its job is to keep vector-first quality guarantees while bounding latency cost for full-scene supersampling.

## 1. Scope and Design Contract

`RenderQuality` owns:

- fixed quality policy constants (vector-first, text-vector-first, HiDPI, HiRes @3x),
- per-window scene-supersampling activation decision,
- default surface format / text-rendering global bootstrap policy,
- layer texture-size resolution for QML consumers.

`RenderQuality` does **not** own:

- icon/svg source resolution (`SvgManager` owns this),
- UI tree composition (QML owns this),
- platform backend selection (`vulkanbootstrap` owns this).

## 2. Property Contract (Detailed)

| Property | Type | Mutability | Effective Behavior | Edge Cases |
|---|---|---|---|---|
| `vectorFirst` | `bool` | constant | Always `true`. Backend policy value, not user-tunable at runtime. | None. |
| `textVectorFirst` | `bool` | constant | Always `true`. Native text rendering is enforced as policy. | None. |
| `hiDpiEnabled` | `bool` | constant | Always `true`. Global env policy is configured when defaults are applied. | If app is already created before policy call, Qt may warn about timing. |
| `hiResScale` | `real` | constant | Always `3.0`. Public mirror of forced supersample scale. | None. |
| `effectiveSupersampleScaleValue` | `real` | constant-read | Returns `3.0` when enabled; returns `1.0` only if supersampling is effectively disabled. | Disabled path is API-compatible fallback behavior. |
| `supersamplingEnabled` | `bool` | constant | Always `true` in current policy build. | Compile-time policy only. |
| `antialiasingEnabled` | `bool` | constant | Always `true`; MSAA floor >= 2 is enforced. | Requests below 2 are clamped. |
| `sceneSupersampling` | `bool` | read/write | Gate flag for full-scene layer supersampling decision. | Setting `false` forces `sceneSupersamplingActive=false`. |
| `sceneSupersamplingActive` | `bool` | read-only | Computed from current bound window size + policy budget + scale. | `false` when no bound window, invalid size, or over budget. |
| `sceneSupersamplePixelBudget` | `int` | constant | Default `6,000,000` pixels (cost model uses `w*h*scale^2`). | Constant in current implementation. |
| `enabled` | `bool` | read/write (policy-clamped) | API accepts writes, but policy normalizes to enabled=true (quality-on default). | Write `false` is ignored by policy in current build. |
| `supersampleScale` | `real` | read/write (policy-clamped) | API accepts writes, but scale is normalized to fixed `3.0`. | Any value is clamped/forced to policy. |
| `minimumSupersampleScale` | `real` | constant | `3.0` | None. |
| `maximumSupersampleScale` | `real` | constant | `3.0` | None. |
| `msaaSamples` | `int` | read/write | Clamped to `[2,16]` when antialiasing is enabled. | Negative/zero values become `2`. |
| `nativeTextRendering` | `bool` | read/write (policy-clamped) | Enforced `true`; API remains for compatibility. | Write `false` is ignored by policy. |
| `framesInFlight` | `int` | read/write | RHI frames-in-flight hint used during global defaults bootstrap. | Clamped `[1,3]`. |
| `partialUpdateEnabled` | `bool` | read/write | Enables dirty-region/partial-update environment hints. | Effective when set before app bootstrap. |
| `batchRenderingEnabled` | `bool` | read/write | Enables batching/atlas environment hints. | Effective when set before app bootstrap. |
| `inactiveRenderDowngradeEnabled` | `bool` | read/write | Enables hidden/minimized window render-cost downgrade. | Runtime-toggle supported. |
| `inactiveMsaaSamples` | `int` | read/write | Target MSAA sample count while power-save mode is active. | Clamped `[0,16]`. |
| `powerSaveActive` | `bool` | read-only | True when bound window is hidden/minimized and downgrade policy is active. | Cleared on unbind/destroy. |

## 3. Method Contract (Detailed)

### `effectiveSupersampleScale(): real`

- Returns the effective scale used by policy, not the raw user request.
- In current policy: normally `3.0`.

### `shouldUseSceneSupersampling(width, height): bool`

Decision formula:

1. `enabled` must be true.
2. `sceneSupersampling` must be true.
3. `width > 0 && height > 0`.
4. `effectiveScale > 1.0`.
5. `width * height * scale^2 <= sceneSupersamplePixelBudget`.

Examples with `scale=3.0`, budget `6,000,000`:

- `640x360`: cost = `640*360*9 = 2,073,600` -> active candidate = true
- `1280x720`: cost = `8,294,400` -> false (over budget)
- `1480x980`: cost = `13,053,600` -> false

### `resolveLayerTextureSize(width, height, sceneSupersamplingActive = true): size`

Rules:

- Base dimensions are clamped to at least `1x1`.
- If `sceneSupersamplingActive=false`, returns base size.
- If active and scale>1, returns rounded scaled size.

Examples (scale=3):

- `(640,360,true)` -> `(1920,1080)`
- `(640,360,false)` -> `(640,360)`
- `(0,0,true)` -> `(3,3)` because base becomes `(1,1)` then scaled.

### `bindWindow(window): void`

- Accepts `QObject*`; only `QQuickWindow*` is valid.
- On valid window:
  - stores pointer,
  - subscribes to width/height change,
  - subscribes to destroyed signal,
  - recomputes `sceneSupersamplingActive`.
- On invalid/null:
  - clears existing binding,
  - forces `sceneSupersamplingActive=false`.

### `unbindWindow(): void`

- Disconnects all bound-window signal links and clears active flag.

### `applyWindow(window): void`

Per-window quality application:

1. Validates window type.
2. Binds window (for live active-flag recompute).
3. Applies format constraints:
   - `samples >= msaaSamples` (clamped by policy),
   - keeps stronger existing format values.
4. Enables persistent graphics/scene graph.
5. Forces native text rendering.
6. Recomputes `sceneSupersamplingActive`.

### `applyGlobalDefaults(): void`

- Calls static default configuration with current runtime msaa setting.

### `configureGlobalDefaults(msaaSamples = 4, nativeTextRendering = true, framesInFlight = 2, partialUpdateEnabled = true, batchRenderingEnabled = true)` (static)

Global process-level defaults:

- HiDPI env hints (`QT_ENABLE_HIGHDPI_SCALING`, rounding policy),
- antialiasing method hint (`QSG_ANTIALIASING_METHOD=msaa`),
- frames-in-flight hint (`QSG_RHI_FRAMES_IN_FLIGHT`),
- partial update hints (`QSG_PARTIAL_UPDATE`, `QSG_NO_FULL_REDRAW`) when enabled,
- batch/atlas hints (`QSG_BATCH_RENDERER`, `QSG_ATLAS_WIDTH`, `QSG_ATLAS_HEIGHT`) when enabled,
- render-loop / software-render fallback defaults when unset,
- default `QSurfaceFormat` sample/depth/stencil floors,
- native text render type enforcement.

`nativeTextRendering` parameter is currently API-compatible but policy-forced to native rendering.

## 4. State Transition Model

### Scene supersampling active flag

`sceneSupersamplingActive` transitions on:

- `bindWindow`,
- bound window width/height changes,
- `enabled`/`sceneSupersampling`/`supersampleScale` policy updates,
- `unbindWindow`,
- bound window destruction.

### Window lifecycle scenarios

| Scenario | Expected Result |
|---|---|
| bind small window (`640x360`) | `sceneSupersamplingActive=true` |
| resize to large (`1480x980`) | flips to `false` |
| minimize/hide bound window with downgrade enabled | `powerSaveActive=true`, persistent graphics/scenegraph disabled |
| restore bound window visibility | `powerSaveActive=false`, persistent graphics/scenegraph re-enabled |
| unbind | `false` |
| destroy bound window | `false` + connections released |
| applyWindow(null) | no-op |

## 5. QML Integration Patterns

### Recommended

```qml
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root
    readonly property bool sceneSsaa: LV.RenderQuality.sceneSupersamplingActive

    Component.onCompleted: {
        LV.RenderQuality.applyWindow(root)
    }

    Item {
        anchors.fill: parent
        layer.enabled: root.sceneSsaa
        layer.textureSize: LV.RenderQuality.resolveLayerTextureSize(width, height, layer.enabled)
    }
}
```

### Avoid

- Re-implementing texture-size math directly in QML repeatedly.
- Assuming raw `supersampleScale` write is honored as-is in policy-fixed builds.
- Enabling full-scene supersampling blindly for large windows without budget gating.

## 6. Failure and Troubleshooting Matrix

| Symptom | Likely Cause | Verification | Action |
|---|---|---|---|
| No visible quality change | `applyWindow()` not called on active window | inspect startup sequence logs | call `applyWindow(root)` at `Component.onCompleted` |
| `sceneSupersamplingActive` always false | window too large for budget | evaluate `w*h*scale^2` | keep policy, accept fallback to non-scene-SSAA |
| Text looks rasterized | custom text render type override elsewhere | inspect `QQuickWindow::textRenderType()` | remove conflicting override, keep native rendering |
| MSAA lower than expected | format already fixed before app default set | inspect startup order | configure defaults before window creation |
| HiDPI warning at runtime | policy called after `QGuiApplication` creation | check logs | move global default call earlier in bootstrap |

## 7. Compatibility Notes

- API keeps setter surfaces (`enabled`, `supersampleScale`, `nativeTextRendering`) for compatibility with existing QML, even when policy is fixed.
- Consumers should bind to `effectiveSupersampleScaleValue` and `sceneSupersamplingActive` instead of assuming mutable scale.
- If future build variants relax policy, the same APIs remain valid.

## 8. Codex-Oriented Playbook

This section is written for automated engineering agents (Codex) operating on LVRS.

### 8.1 Safe default sequence for code generation

1. Use `effectiveSupersampleScaleValue` as read-only source of truth.
2. Call `applyWindow(window)` once at window/component startup.
3. Bind `layer.enabled` to `sceneSupersamplingActive`.
4. Derive `layer.textureSize` exclusively via `resolveLayerTextureSize(...)`.
5. Avoid writing policy-clamped setters unless preserving external API shape.

### 8.2 “Do” contract for Codex patches

- Prefer backend-owned policy calls over inline QML math.
- Preserve `bindWindow`/`unbindWindow` lifecycle correctness if window ownership changes.
- Keep compatibility-facing properties intact even if policy is fixed.

### 8.3 “Do not” contract for Codex patches

- Do not add duplicate supersampling formulas in multiple QML components.
- Do not bypass scene budget checks by forcing `layer.enabled=true`.
- Do not reintroduce non-native text rendering for convenience.

### 8.4 Quick verification script pattern

After patching:

1. resize-test small and large windows to validate active-flag transitions,
2. inspect that `sceneSupersamplingActive` flips deterministically,
3. confirm no QML-side custom texture-size arithmetic remains in touched files.

## 9. Validation Checklist

- `applyWindow()` called for each active `QQuickWindow`.
- `sceneSupersamplingActive` flips correctly across resize thresholds.
- `layer.textureSize` is resolved by `resolveLayerTextureSize()`.
- `QQuickWindow::textRenderType()` remains native.
- Default surface format has expected sample/depth/stencil floors.

## 10. Related APIs

- `SvgManager`: vector icon source enforcement.
- `RuntimeEvents`: runtime capture cost controls (indirect latency impact).
- `ApplicationWindow`: standard QML integration shell using this policy.
