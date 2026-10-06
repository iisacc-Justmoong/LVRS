# RenderMonitor

위치: `src/backend/runtime/renderingmonitor.h` / `src/backend/runtime/renderingmonitor.cpp`

`RenderMonitor`는 `QQuickWindow`에 대한 프레임 타이밍 메트릭(`fps`, 프레임 시간, 백분위수 프레임 통계, 드롭 카운터)을 제공합니다.

<a id="purpose"></a>

## 목적

- `QQuickWindow::frameSwapped`를 통해 렌더링 케이던스를 관찰하세요.
- 런타임 프레임 성능 메트릭을 QML에 노출합니다.
- 창 수명주기와 관계없이 시작/중지/재설정 제어를 허용합니다.
- 단일 스냅샷 스키마에서 P0 기준 지표(`avg`, `p95`, `p99`, 삭제된 프레임)를 제공합니다.

<a id="properties"></a>

## 속성

- `active: bool`
- `fps: double`
- `lastFrameMs: double`
- `avgFrameMs: double`
- `p95FrameMs: double`
- `p99FrameMs: double`
- `droppedFrameCount: uint64`
- `droppedFrameThresholdMs: double`
- `frameSampleCapacity: int`
- `recentSampleCount: int`
- `frameCount: uint64`

<a id="methods"></a>

## 방법

- `attachWindow(window)`
- `start()`
- `stop()`
- `reset()`
- `performanceSnapshot()`

<a id="signals"></a>

## 신호

- `activeChanged()`
- `statsChanged()`
- `droppedFrameThresholdMsChanged()`
- `frameSampleCapacityChanged()`

<a id="how-it-works"></a>

## 동작 원리

- `attachWindow(window)`는 기존 대상의 연결을 끊고 새 `QQuickWindow`에 바인딩합니다.
- 모든 `frameSwapped`에서 모니터는 이전 프레임에서 경과된 밀리초를 계산합니다.
- 경과 시간이 양수이면 `fps`는 `1000 / lastFrameMs`로 파생됩니다.
- 롤링 프레임 샘플을 유지하고 해당 창에서 `avg/p95/p99`를 계산합니다.
- 프레임 시간이 `droppedFrameThresholdMs`를 초과하면 `droppedFrameCount`가 증가합니다.
- 창 파괴는 대상을 자동으로 분리하고 모니터를 비활성화합니다.

<a id="snapshot-schema-performancesnapshot"></a>

### 스냅샷 스키마(`performanceSnapshot`)

반환된 맵 키:

- `schema` (`lvrs.performance.v1`)
- `component` (`RenderMonitor`)
- `epochMs`
- `active`
- `fps`
- `lastFrameMs`
- `avgFrameMs`
- `p95FrameMs`
- `p99FrameMs`
- `frameCount`
- `droppedFrameCount`
- `droppedFrameThresholdMs`
- `recentSampleCount`
- `frameSampleCapacity`

<a id="usage-example"></a>

## 사용예

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RenderMonitor.attachWindow(window)
    LV.RenderMonitor.start()
}

LV.Label {
    text: "FPS: " + LV.RenderMonitor.fps.toFixed(1)
    style: body
}
```

<a id="operational-usage-pattern"></a>

## 운영 사용 패턴

권장 수명 주기:

1. 루트 창 생성 후 창 연결,
2. 대상 시나리오에 대해 `frameSampleCapacity` 및 `droppedFrameThresholdMs`를 설정하고,
3. 성능에 민감한 화면에 들어갈 때 모니터를 시작합니다.
4. 측정 실행 전에 메트릭을 재설정하고,
5. 불필요한 신호 변동을 줄이기 위해 화면을 떠날 때 모니터를 중지합니다.

<a id="caveats"></a>

## 주의사항

- `fps`는 종단 간 앱 대기 시간이 아닌 프레임 스왑 흐름을 반영합니다.
- 재설정 후 첫 번째 프레임은 타이밍 기준을 초기화하는 데 사용됩니다.
- 백분위수 측정항목은 전체 프로세스 수명이 아닌 롤링 기간에서 계산됩니다.

## FAQ

Q. 시작 시 FPS가 0를 읽는 이유는 무엇입니까?   A. 첫 번째 프레임 교체 후 메트릭이 초기화됩니다. 프레임 케이던스가 설정되기 전에는 값이 0으로 유지됩니다.
