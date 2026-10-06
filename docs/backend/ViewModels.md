# ViewModels

위치:

- `src/backend/state/viewmodel.h` / `src/backend/state/viewmodel.cpp`
- `src/backend/state/statemodel.h` / `src/backend/state/statemodel.cpp`
- `src/backend/state/viewmodelregistry.h` / `src/backend/state/viewmodelregistry.cpp`

`ViewModel`는 전용 ViewModel 개체에 대한 C++ 기본 유형입니다. `StateModel`는 QML에서 구성 요소 상태를 마이그레이션하는 동안 사용되는 구체적인 키-값 상태 모델입니다. `ViewModels`는 싱글턴 레지스트리이며 MVVM 모델 개체에 대한 소유권 게이트입니다.

<a id="purpose"></a>

## 목적

- C++ ViewModels에 일관된 진단 표면을 제공합니다.
- 아직 도메인별 하위 클래스가 필요하지 않은 구성 요소에 대해 재사용 가능한 C++ 상태 개체를 제공합니다.
- 키로 모델 객체를 등록합니다.
- 뷰 ID를 키에 바인딩합니다.
- 단일 작성자 소유권 의미 체계를 적용합니다.
- 개발자 도구가 도메인 클래스를 몰라도 검사할 수 있는 설명자를 노출합니다.

<a id="c-viewmodel-base"></a>

## C++ ViewModel 베이스

앱은 안정적인 MVVM 계약이 필요할 때 `ViewModel`에서 파생되는 C++ ViewModel 클래스를 선호해야 합니다. 도메인별 상태 및 명령은 여전히 ​​앱 하위 클래스에 있습니다.

기본 속성:

- `key: string`
- `displayName: string`
- `busy: bool`
- `error: string`
- `hasError: bool`
- `metadata: map`

기본 방법:

- `clearError()`
- `snapshot()`

기본 유형은 생성할 수 없는 C++ 유형으로 QML에 등록됩니다. QML는 부트스트랩 앱에 등록된 구체적인 인스턴스를 사용합니다. 기본 유형을 인스턴스화해서는 안됩니다.

## StateModel

`StateModel`는 `ViewModel`에서 파생되며 다음을 추가합니다.

- `values`
- `stateKeys`
- `revision`
- `empty`
- `value(...)`, `setValue(...)`, `applyPatch(...)`, `removeValue(...)`, `clearValues()`
- `valueOr(...)`
- `stateSnapshot()`

형식화된 도메인 ViewModel를 설계하기 전에 임시 QML 구성 요소 상태를 C++로 이동할 때 선호되는 첫 번째 단계입니다.

<a id="properties"></a>

## 속성

- `keys: stringList`
- `views: stringList`
- `bindings: map`
- `owners: map`
- `descriptors: map`
- `lastError: string`

<a id="registration-apis"></a>

## 등록 API

- `set(key, object)`
- `registerViewModel(object, fallbackKey = "")`
- `get(key)`
- `remove(key)`
- `clear()`

`registerViewModel()`는 ViewModel 개체에 대한 기본 C++ 부트스트랩 진입점입니다. 먼저 명시적인 대체 경로에서 키를 확인한 다음 `ViewModel::key`에서 키를 확인하고 개체를 저장하고 설명자 관찰을 시작합니다.

<a id="binding-and-ownership-apis"></a>

## 바인딩 및 소유권 API

- `bindView(viewId, key, writable = false)`
- `unbindView(viewId)`
- `getForView(viewId)`
- `keyForView(viewId)`
- `claimOwnership(viewId, key)`
- `releaseOwnership(viewId, key?)`
- `canWrite(viewId, key?)`
- `ownerOf(key)`

<a id="property-access-apis"></a>

## 자산 액세스 API

- `updateProperty(viewId, property, value)`
- `updatePropertyByKey(viewId, key, property, value)`
- `readProperty(viewId, property)`

<a id="descriptor-apis"></a>

## 설명자 API

- `descriptor(key)`
- `descriptors`

설명 맵에는 다음이 포함됩니다.

- `key`
- `className`
- `owner`
- `views`
- `viewModel`
- `ViewModel`에서 파생된 개체의 경우 `viewModelKey`, `displayName`, `busy`, `error`, `hasError`, `metadata`
- `StateModel`에서 파생된 개체의 경우 `stateModel`, `values`, `stateKeys`, `revision`, `empty`

`descriptorsChanged`는 등록된 ViewModel 진단 변경, 키가 추가/제거될 때 또는 보기 바인딩/소유권이 변경될 때 내보내집니다.

<a id="write-guard-semantics"></a>

## 가드 의미론 작성

다음과 같은 경우 쓰기 업데이트가 실패합니다.

- 보기 ID가 비어 있습니다.
- 뷰에는 바인딩이 없습니다.
- 뷰는 대상 키의 소유자가 아닙니다.
- 키/객체/속성이 잘못되었습니다.

실패하면 API는 `false`를 반환하고 `lastError`를 업데이트합니다.

<a id="usage-example"></a>

## 사용예

```cpp
class DashboardViewModel : public ViewModel
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    // 도메인 상태와 명령은 생략하였다.
};

auto *vm = new DashboardViewModel(&engine);
vm->setKey(QStringLiteral("Dashboard"));
vm->setDisplayName(QStringLiteral("Dashboard"));
vm->setMetadata({{QStringLiteral("domain"), QStringLiteral("dashboard")}});

auto *registry = engine.singletonInstance<ViewModelRegistry *>(QStringLiteral("LVRS"),
                                                               QStringLiteral("ViewModels"));
registry->registerViewModel(vm);
registry->bindView(QStringLiteral("DashboardPage"), QStringLiteral("Dashboard"), true);
```

QML는 레지스트리를 통해 객체를 사용해야 합니다.

```qml
import LVRS 1.0 as LV

property var vm: LV.ViewModels.getForView("DashboardPage")
```

<a id="internal-behavior-notes"></a>

## 내부 행동 참고 사항

- 토큰은 트림 정규화됩니다.
- 키가 사라지면 오래된 바인딩/소유자가 정리됩니다.
- 레지스트리에 부모로 지정된 객체는 키 참조가 모두 사라지면 자동으로 처분될 수 있습니다.
- 개체가 더 이상 레지스트리 키에 의해 참조되지 않으면 설명자 관찰의 연결이 끊어집니다.

<a id="extended-example-explicit-ownership-transfer"></a>

## 확장된 예: 명시적인 소유권 이전

```qml
import LVRS 1.0 as LV

function transferOwner(fromView, toView, key) {
    LV.ViewModels.releaseOwnership(fromView, key)
    if (!LV.ViewModels.claimOwnership(toView, key))
        console.warn(LV.ViewModels.lastError)
}
```

<a id="review-checklist"></a>

## 체크리스트 검토

- 모든 쓰기 가능한 뷰에는 명시적인 소유권 주장이 있어야 합니다.
- 읽기 전용 뷰는 `writable=false`와 바인딩되어야 합니다.
- `lastError`는 쓰기 실패에 대한 개발자 도구에 표시되어야 합니다.

## FAQ

Q. 2 뷰가 동시에 하나의 키에 쓸 수 있나요? A. 아니요. 소유권 모델은 계약상 단일 작성자입니다.
