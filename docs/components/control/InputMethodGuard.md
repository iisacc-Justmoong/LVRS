# InputMethodGuard

위치: `src/qml/components/control/util/InputMethodGuard.qml`

`InputMethodGuard`는 IME 로캘/가시성/초점 조건이 변경될 때 텍스트 구성 상태를 보호합니다.

<a id="purpose"></a>

## 목적

- IME 전환 전반에 걸쳐 사전 편집/구성 잔여물을 방지합니다.
- 민감한 텍스트 컨트롤에 대해 명시적인 작성 커밋 지점을 제공합니다.

## API

- `target`(텍스트 입력형 객체)
- `guardEnabled`(기본값 `true`)
- `commitOnLocaleChanged`(기본값 `false`)
- `commitOnVisibilityLost`(기본값 `true`)
- `commitOnFocusLost`(기본값 `true`)
- `logCommitEvents`(기본값 `false`)

<a id="commit-conditions"></a>

## 커밋 조건

`target.inputMethodComposing == true`인 경우 가드는 구성된 트리거에 대해 구성을 커밋합니다.

- 로케일 변경
- IME 가시성 상실
- 초점을 잃었다

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.InputMethodGuard {
    target: editor
    guardEnabled: enabled && !readOnly
}
```

<a id="how-it-works"></a>

## 동작 원리

- `Qt.inputMethod` 및 대상 초점 신호를 수신합니다.
- 가드가 활성화되고 대상이 구성 중일 때만 `Qt.inputMethod.commit()`를 실행합니다.
- 선택적 디버그 출력은 `Debug.log`를 통해 커밋 이유를 기록합니다.

<a id="deployment-note"></a>

## 배포 노트

대상 사용자 기반이 구성 중에 IME 로케일을 적극적으로 전환하지 않는 한 기본적으로 로케일 변경 커밋을 비활성화된 상태로 유지합니다. 실제 손상 보고가 더 엄격한 커밋 타이밍을 정당화하는 경우에만 활성화하십시오.

## FAQ

Q. 로케일 변경 커밋이 기본적으로 비활성화되어 있는 이유는 무엇입니까?   A. 로케일 변경은 포커스/가시성 전환보다 빈도가 낮으며 일부 IME 워크플로에서는 적극적인 커밋이 방해가 될 수 있습니다.

Q. 읽기 전용 필드에 가드를 부착해야 합니까?   A. 아니요. 읽기 전용 컨트롤에 대해 `guardEnabled`를 false로 유지하세요.

<a id="validation-checklist"></a>

## 검증 체크리스트

- 활성화되면 포커스 손실 시 텍스트 작성이 커밋되는지 확인하고,
- 일반 입력 경로에 중복된 커밋 부작용이 없는지 확인하고,
- 편집 불가능한 컨트롤에 대해 가드가 비활성화되어 있는지 확인하십시오.

<a id="shared-motion"></a>

## 공유 모션

가드는 LVRS 입력이 포커스 링을 제공하는 동안 입력 방법 상태를 조정합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
