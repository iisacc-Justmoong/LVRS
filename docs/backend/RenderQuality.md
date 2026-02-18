# RenderQuality

Location: `backend/runtime/renderquality.h` / `backend/runtime/renderquality.cpp`

`RenderQuality`는 LVRS의 렌더 품질·GPU 비용·전력/성능 균형 정책을 담당하는 QML 싱글턴이다.

## 1. 책임 범위

`RenderQuality`가 직접 관리하는 항목은 다음과 같다.

- 장면 supersampling 활성/비활성 결정
- MSAA, frames-in-flight, partial update, batch renderer 기본 정책
- 윈도우 비활성 시 렌더 강등(power-save) 정책
- PSO(pipeline state object) 캐시 적용 정책
- 텍스처 압축 후보 선택 및 밉맵 사용 정책
- DRS(dynamic resolution scaling) 제어기
- 디바이스 등급별 프리셋(low/balanced/high)

## 2. P3 구현 매핑

| P3 ID | 구현 지점 | 동작 |
|---|---|---|
| `P3-01` | `applyGraphicsConfiguration`, `configureGlobalDefaults` | `QQuickGraphicsConfiguration`의 automatic pipeline cache + load/save 파일 적용, 전역 `QSG_RHI_PIPELINE_CACHE_LOAD/SAVE` 기본값 설정 |
| `P3-02` | `resolveTextureSource`, QML `Image.mipmap` 바인딩 | 동일 경로의 압축 텍스처(`ktx2/ktx/dds`) 자동 선택, 레이어/아이콘 밉맵 정책 통합 |
| `P3-03` | `applyGraphicsConfiguration` | `depthBufferFor2D`, debug/timestamp 비활성 정책으로 2D 패스 상태전환 부담 축소 |
| `P3-04` | `sampleFrameTime`, `frameSwapped` 연결 | 히스테리시스 기반 DRS scale up/down 제어 |
| `P3-05` | `detectDeviceTierForSystem`, `applyDeviceTierPreset` | CPU thread 기반 등급 추정 + low/balanced/high 프리셋 적용 |

## 3. 핵심 프로퍼티 계약

### 3.1 GPU 캐시/파이프라인

- `psoCacheEnabled`
- `psoCacheLoadEnabled`
- `psoCacheSaveEnabled`
- `psoCacheFile`
- `depthBufferFor2D`

`applyWindow()`가 호출되면 위 값이 `QQuickWindow::graphicsConfiguration()`에 즉시 반영된다.

### 3.2 텍스처 정책

- `mipmapEnabled`
- `textureCompressionEnabled`
- `compressedTextureExtensions` (기본: `ktx2`, `ktx`, `dds`)
- `resolveTextureSource(source)`

`resolveTextureSource()`는 로컬 파일/qrc 경로에서 압축 확장자 파일이 실제로 존재할 때만 후보를 치환한다.

### 3.3 DRS

- `dynamicResolutionEnabled`
- `dynamicResolutionScale`
- `dynamicResolutionMinScale`
- `dynamicResolutionMaxScale`
- `dynamicResolutionStep`
- `dynamicResolutionTargetFrameMs`
- `dynamicResolutionHysteresisMs`

`effectiveSupersampleScaleValue`는 DRS 활성 시 동적으로 변하며, QML 바인딩 업데이트를 위해 `NOTIFY` 신호를 사용한다.

### 3.4 디바이스 프리셋

- `detectedDeviceTier` (상수)
- `activeDeviceTier`
- `applyDeviceTierPreset(tier = -1)`

`tier=-1`이면 자동 탐지 등급을 적용한다.

## 4. DRS 동작 규칙

1. `frameSwapped` 시점 프레임 간격(ms)을 샘플링한다.
2. `target + hysteresis` 초과 프레임이 누적되면 scale down 한다.
3. `target - hysteresis` 미만 프레임이 충분히 누적되면 scale up 한다.
4. scale은 `[dynamicResolutionMinScale, dynamicResolutionMaxScale]` 내에서만 변경된다.
5. 윈도우가 power-save 상태면 DRS 샘플링을 중단한다.

## 5. 프리셋 표준

| 프리셋 | 의도 | 기본값 요약 |
|---|---|---|
| `LowTier` | 저사양 안정성 우선 | `MSAA=2`, `framesInFlight=1`, `DRS on`, `mipmap off` |
| `BalancedTier` | 기본 균형 모드 | `MSAA=4`, `framesInFlight=2`, `DRS on`, `mipmap on` |
| `HighTier` | 화질 우선 | `MSAA=8`, `framesInFlight=3`, `DRS off`, `mipmap on` |

## 6. QML 적용 포인트

- `qml/ApplicationWindow.qml`
- `qml/Window.qml`
- 주요 아이콘 컴포넌트(`IconButton`, `IconMenuButton`, `LabelMenuButton`, `MenuItem`, `HierarchyItem`, `ListToolbar`)

적용 내용:

- 레이어 `layer.mipmap: RenderQuality.mipmapEnabled`
- 이미지 `source: RenderQuality.resolveTextureSource(...)`
- 시작 시 `RenderQuality.applyDeviceTierPreset(...)`

## 7. 검증 명령

- `cmake --build build-codex --target LVRSTests_render_quality`
- `./build-codex/tests/LVRSTests_render_quality -txt`

P3 회귀 검증 테스트: `render_quality_gpu_policy_pso_texture_and_drs_contract()`

## 8. 관련 문서

- `docs/components/app/ApplicationWindow.md`
- `docs/architecture/rendering-backend.md`
- `docs/quality-automation-p4.md`
