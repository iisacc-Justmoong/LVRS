# ButtonMethodRegistry

위치: `src/qml/components/control/buttons/ButtonMethodRegistry.qml`

`ButtonMethodRegistry`는 버튼 제품군에서 사용되는 공유 내부 메서드 주입 호스트입니다.

<a id="purpose"></a>

## 목적

- 버튼 모양 컨트롤 전체에서 `method` 및 `methods` 주입 API를 정규화합니다.
- 클릭 및 수동 메서드 디스패치를 위한 일관된 이벤트 페이로드를 구축합니다.
- 각 버튼을 명령 프레임워크에 연결하지 않고도 JavaScript 기능과 명령 개체를 지원합니다.

## API

- `owner`: API를 노출하는 구성 요소입니다.
- `defaultTrigger`: 명시적인 트리거가 제공되지 않을 때 사용되는 트리거 이름입니다.
- `method`: 단일 주입 호출 가능.
- `methods`: 콜러블 또는 명령 개체의 배열입니다.
- `hasInjectedMethods`(읽기 전용).
- `collectMethods()`
- `createEvent(triggerName)`
- `invokeMethod(candidate, eventData)`
- `invokeMethods(eventData)`

<a id="callable-contract"></a>

## 호출 가능한 계약

허용되는 항목은 다음과 같습니다.

- JavaScript 기능: `function(eventData) { ... }`
- `invoke(eventData)`를 사용한 명령 개체
- `trigger(eventData)`를 사용한 명령/작업 개체

`method`는 `methods`의 항목보다 먼저 실행됩니다.

<a id="usage"></a>

## 사용법

```qml
ButtonMethodRegistry {
    owner: control
    defaultTrigger: "clicked"
    method: function(eventData) {
        commandBus.dispatch(eventData.trigger)
    }
}
```
