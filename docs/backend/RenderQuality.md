# RenderQuality

위치: `src/backend/runtime/renderquality.h` / `src/backend/runtime/renderquality.cpp`

`RenderQuality`는 LVRS 런타임 렌더링 품질, GPU 비용, 전력/성능 균형 정책을 담당하는 QML 싱글턴입니다.

<a id="1-responsibility-scope"></a>

## 1. 책임범위

`RenderQuality`는 다음을 직접 관리합니다.

- 장면 슈퍼샘플링 활성화/비활성화 결정
- 장면 슈퍼샘플링 스케일 창 픽셀 예산에 따른 클램핑(가능한 경우 텍스트 보정 유지)
- MSAA, 전송 중인 프레임, 부분 업데이트 및 배치 렌더러에 대한 기본 정책
- 창이 비활성화된 경우 다운그레이드(절전) 정책 렌더링
- PSO(파이프라인 상태 개체) 캐시 정책
- 압축된 텍스처 후보 선택 및 밉맵 사용 정책
- DRS(동적 해상도 스케일링) 컨트롤러
- 명시적인 하위 소비 측 재정의를 위한 선택적 장치 계층 사전 설정(낮음/균형/높음/울트라)

<a id="2-p3-implementation-mapping"></a>

## 2. P3 구현 매핑

| P3 ID |구현 위치|행동|
|---|---|---|
| `P3-01` | `applyGraphicsConfiguration`, `configureGlobalDefaults` |자동 파이프라인 캐시 적용 + `QQuickGraphicsConfiguration`에 파일 로드/저장 및 전역 `QSG_RHI_PIPELINE_CACHE_LOAD/SAVE` 기본값 설정|
| `P3-02` |`resolveTextureSource`, QML `Image.mipmap` 바인딩|동일한 경로에서 압축된 텍스처(`ktx2/ktx/dds`) 자동 선택 및 밉맵 정책 통합 레이어/아이콘용|
| `P3-03` | `applyGraphicsConfiguration` |2D 패스 상태 전이 오버헤드를 줄이고  `depthBufferFor2D` 와 디버그/타임스탬프 비활성화 정책을 적용합니다.|
| `P3-04` |`sampleFrameTime`, `frameSwapped` 연결|히스테리시스 기반 DRS 스케일 업/다운 제어|
| `P3-05` | `detectDeviceTierForSystem`, `applyDeviceTierPreset` |CPU-스레드 기반 계층 추정 + 로우/밸런스/하이 프리셋 애플리케이션|

<a id="3-core-property-contracts"></a>

## 3. 핵심 속성 계약

<a id="31-gpu-cachepipeline"></a>

### 3.1 GPU 캐시/파이프라인

- `psoCacheEnabled`
- `psoCacheLoadEnabled`
- `psoCacheSaveEnabled`
- `psoCacheFile`
- `depthBufferFor2D`

`applyWindow()` 가 네이티브 창이 생성/표시되기 전에 호출되면, 위의 값들이 `QQuickWindow::graphicsConfiguration()` 에 적용됩니다. iOS 에서, 네이티브 창이 존재하면 LVRS 는 현재 Metal 표면 구성을 안정적으로 유지하고, 원래는 스왑체인 변경을 강요할 수 있는 후기 그래픽 구성 재작성을 건너뛰면서, `4x` MSAA 및 `2.0x+` 동적 오버샘플링에 대한 iOS 품질 하한을 보존합니다.

<a id="32-texture-policy"></a>

### 3.2 텍스처 정책

- `mipmapEnabled`
- `textureCompressionEnabled`
- `compressedTextureExtensions`(기본값: `ktx2`, `ktx`, `dds`)
- `resolveTextureSource(source)`

`resolveTextureSource()`는 압축된 확장 파일이 로컬 파일/qrc 경로에 실제로 존재하는 경우에만 후보를 대체합니다.

### 3.3 DRS

- `dynamicResolutionEnabled`
- `dynamicResolutionScale`
- `dynamicResolutionMinScale`
- `dynamicResolutionMaxScale`
- `dynamicResolutionStep`
- `dynamicResolutionTargetFrameMs`
- `dynamicResolutionHysteresisMs`

`effectiveSupersampleScaleValue`는 DRS가 활성화되고 `NOTIFY` 신호를 사용하여 QML 바인딩을 업데이트할 때 동적으로 변경됩니다.

<a id="34-scene-supersampling-budget-behavior"></a>

### 3.4 장면 슈퍼샘플링 예산 동작

- 요청된 슈퍼샘플 스케일은 `effectiveSupersampleScaleValue`부터 시작됩니다.
- 런타임는 정상 범위에서 바이너리 완전 켜짐/완전 꺼짐 대신 창 크기당 예산 적합 규모를 계산합니다.
- 예산이 최소 `1.0x`를 유지할 수 없는 경우 해당 크기에 대해 장면 슈퍼샘플링이 비활성화됩니다.
- 프레임워크는 픽셀 예산이 허용할 때마다 기본 텍스트 보상 하한선(`1.2x`)을 유지합니다.

<a id="35-device-presets"></a>

### 3.5 장치 사전 설정

- `detectedDeviceTier`(상수)
- `activeDeviceTier`
- `applyDeviceTierPreset(tier = -1)`

`tier=-1`인 경우 자동으로 감지된 장치 계층이 적용됩니다. Stock LVRS 쉘은 더 이상 시작 시 이를 자동 적용하지 않습니다. 장치 계층 사전 설정은 이제 런타임 직접 품질 경로 위에 명시적인 옵트인 재정의입니다.

<a id="4-drs-behavior-rules"></a>

## 4. DRS 행동 규칙

1. `frameSwapped` 타이밍의 샘플 프레임 간격(ms)입니다.
2. `target + hysteresis` 이상의 프레임이 누적되면 축소됩니다.
3. `target - hysteresis` 미만의 프레임이 충분히 누적되면 확장하세요.
4. 축척 변경은 `[dynamicResolutionMinScale, dynamicResolutionMaxScale]`로 제한됩니다.
5. 창이 절전 상태인 동안 DRS 샘플링을 중지합니다.

<a id="5-preset-standards"></a>

## 5. 사전 설정된 표준

|사전 설정|의도|기본 요약|
|---|---|---|
| `LowTier` |로우엔드 안정성을 우선시합니다.| `MSAA=2`, `framesInFlight=1`, `DRS on`, `mipmap off` |
| `BalancedTier` |기본 밸런스 모드| `MSAA=4`, `framesInFlight=2`, `DRS on`, `mipmap on` |
| `HighTier` |이미지 품질 우선| `MSAA=12`, `framesInFlight=3`, `DRS off`, `mipmap on`, `textureCompression off` |
| `UltraTier` |최대 시각적 충실도 기준| `MSAA=16`, `framesInFlight=3`, `DRS off`, `mipmap on`, `depthBufferFor2D on`, `textureCompression off`, `inactive downgrade off` |

<a id="6-qml-integration-points"></a>

## 6. QML 통합 포인트

- `src/qml/ApplicationWindow.qml`
- `src/qml/Window.qml`
- 주요 아이콘/이미지 구성 요소(`IconButton`, `IconMenuButton`, `LabelMenuButton`, `MenuItem`, `HierarchyItem`, `ListToolbar`)
- 슈퍼샘플링된 `Image.sourceSize`를 통한 스냅샷 기반 제어 아이콘(`Stepper`, `InputField` 검색 아이콘)
- 축당 천장 반올림 기능이 있는 슈퍼샘플링된 `canvasSize`를 통한 `Canvas` 기반 제어 아이콘(`CheckBox`) `ToggleSwitch`는 래스터 캔버스 대신 장면 그래프 벡터 손잡이를 사용합니다.
- `SvgManager.icon(...)`는 호출자가 `logicalSize`를 생략할 때 `18 x 18` 논리적 기본값을 사용합니다. 래스터 출력은 정사각형으로 유지되고 SVG 콘텐츠는 종횡비를 유지합니다.

적용된 동작:

- 층: `layer.mipmap: RenderQuality.mipmapEnabled`
- 이미지 출처: `source: RenderQuality.resolveTextureSource(...)`
- 시작 창 연결: `RenderQuality.applyWindow(...)`
- 셸 기본값은 `ApplicationWindow` 및 `Window`에서 `autoApplyDeviceTierPreset: false` 및 `forcedDeviceTierPreset: -1`를 유지하므로 런타임 직접 슈퍼샘플링/MSAA 기본값은 하위 소비 측 앱이 명시적으로 장치 계층 사전 설정을 선택하지 않는 한 활성 상태를 유지합니다.

<a id="7-verification-commands"></a>

## 7. 확인 명령

- `cmake --build build-codex --target LVRSTests_render_quality`
- `./build-codex/tests/LVRSTests_render_quality -txt`

P3 회귀 검증 테스트: `render_quality_gpu_policy_pso_texture_and_drs_contract()`

<a id="8-related-documents"></a>

## 8. 관련 문서

- `docs/components/app/ApplicationWindow.md`
- `docs/architecture/rendering-backend.md`
- `docs/quality-automation-p4.md`
