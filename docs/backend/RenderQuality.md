# RenderQuality

Location: `backend/runtime/renderquality.h` / `backend/runtime/renderquality.cpp`

`RenderQuality` is a QML singleton responsible for LVRS rendering quality, GPU cost, and power/performance balance policy.

## 1. Responsibility Scope

`RenderQuality` directly manages the following:

- scene supersampling enable/disable decisions
- scene supersampling scale clamping by window pixel budget (keep text compensation when possible)
- baseline policy for MSAA, frames-in-flight, partial update, and batch renderer
- render downgrade (power-save) policy when a window is inactive
- PSO (pipeline state object) cache policy
- compressed texture candidate selection and mipmap usage policy
- DRS (dynamic resolution scaling) controller
- device-tier presets (low/balanced/high)

## 2. P3 Implementation Mapping

| P3 ID | Implementation Location | Behavior |
|---|---|---|
| `P3-01` | `applyGraphicsConfiguration`, `configureGlobalDefaults` | apply automatic pipeline cache + load/save files on `QQuickGraphicsConfiguration`, and set global `QSG_RHI_PIPELINE_CACHE_LOAD/SAVE` defaults |
| `P3-02` | `resolveTextureSource`, QML `Image.mipmap` binding | auto-select compressed textures (`ktx2/ktx/dds`) at the same path and unify mipmap policy for layers/icons |
| `P3-03` | `applyGraphicsConfiguration` | reduce 2D pass state-transition overhead via `depthBufferFor2D` and debug/timestamp disable policy |
| `P3-04` | `sampleFrameTime`, `frameSwapped` hookup | hysteresis-based DRS scale up/down control |
| `P3-05` | `detectDeviceTierForSystem`, `applyDeviceTierPreset` | CPU-thread-based tier estimation + low/balanced/high preset application |

## 3. Core Property Contracts

### 3.1 GPU cache/pipeline

- `psoCacheEnabled`
- `psoCacheLoadEnabled`
- `psoCacheSaveEnabled`
- `psoCacheFile`
- `depthBufferFor2D`

When `applyWindow()` is called, the values above are immediately applied to `QQuickWindow::graphicsConfiguration()`.

### 3.2 Texture policy

- `mipmapEnabled`
- `textureCompressionEnabled`
- `compressedTextureExtensions` (default: `ktx2`, `ktx`, `dds`)
- `resolveTextureSource(source)`

`resolveTextureSource()` replaces candidates only when compressed extension files actually exist on local file/qrc paths.

### 3.3 DRS

- `dynamicResolutionEnabled`
- `dynamicResolutionScale`
- `dynamicResolutionMinScale`
- `dynamicResolutionMaxScale`
- `dynamicResolutionStep`
- `dynamicResolutionTargetFrameMs`
- `dynamicResolutionHysteresisMs`

`effectiveSupersampleScaleValue` changes dynamically when DRS is enabled and uses a `NOTIFY` signal to update QML bindings.

### 3.4 Scene supersampling budget behavior

- Requested supersample scale starts from `effectiveSupersampleScaleValue`.
- Runtime computes a budget-fit scale per window size instead of binary full-on/full-off in normal ranges.
- If the budget cannot keep at least `1.0x`, scene supersampling is disabled for that size.
- Framework keeps a default text-compensation floor (`1.2x`) whenever the pixel budget allows it.

### 3.5 Device presets

- `detectedDeviceTier` (constant)
- `activeDeviceTier`
- `applyDeviceTierPreset(tier = -1)`

If `tier=-1`, the automatically detected device tier is applied.

## 4. DRS Behavior Rules

1. Sample frame interval (ms) at `frameSwapped` timing.
2. Scale down when frames above `target + hysteresis` accumulate.
3. Scale up when enough frames below `target - hysteresis` accumulate.
4. Scale changes are constrained to `[dynamicResolutionMinScale, dynamicResolutionMaxScale]`.
5. Stop DRS sampling while the window is in power-save state.

## 5. Preset Standards

| Preset | Intent | Default Summary |
|---|---|---|
| `LowTier` | prioritize low-end stability | `MSAA=2`, `framesInFlight=1`, `DRS on`, `mipmap off` |
| `BalancedTier` | default balanced mode | `MSAA=4`, `framesInFlight=2`, `DRS on`, `mipmap on` |
| `HighTier` | prioritize image quality | `MSAA=12`, `framesInFlight=3`, `DRS off`, `mipmap on`, `textureCompression off` |
| `UltraTier` | maximum visual fidelity baseline | `MSAA=16`, `framesInFlight=3`, `DRS off`, `mipmap on`, `depthBufferFor2D on`, `textureCompression off`, `inactive downgrade off` |

## 6. QML Integration Points

- `qml/ApplicationWindow.qml`
- `qml/Window.qml`
- major icon/image components (`IconButton`, `IconMenuButton`, `LabelMenuButton`, `MenuItem`, `HierarchyItem`, `ListToolbar`)
- snapshot-driven control icons (`Stepper`, `InputField` search icon) via supersampled `Image.sourceSize`
- `Canvas`-based control icons (`CheckBox`, `ToggleSwitch`) via supersampled `canvasSize` with per-axis ceil rounding

Applied behavior:

- layer: `layer.mipmap: RenderQuality.mipmapEnabled`
- image source: `source: RenderQuality.resolveTextureSource(...)`
- startup preset apply: `RenderQuality.applyDeviceTierPreset(...)`
- shell defaults keep startup policy in auto-detect mode (`forcedDeviceTierPreset: -1`) in `ApplicationWindow` and `Window`

## 7. Verification Commands

- `cmake --build build-codex --target LVRSTests_render_quality`
- `./build-codex/tests/LVRSTests_render_quality -txt`

P3 regression validation test: `render_quality_gpu_policy_pso_texture_and_drs_contract()`

## 8. Related Documents

- `docs/components/app/ApplicationWindow.md`
- `docs/architecture/rendering-backend.md`
- `docs/quality-automation-p4.md`
