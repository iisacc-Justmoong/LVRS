# MVVM

LVRS MVVM는 C++ ViewModel 클래스, 재사용 가능한 구성 요소 상태를 위한 `StateModel`, `ViewModels`(`ViewModelRegistry`) 싱글턴 및 경로/보기를 중심으로 합니다. `PageRouter`에서 내보낸 바인딩 메타데이터입니다.

<a id="purpose"></a>

## 목적

- 논리 키로 뷰 모델 인스턴스를 등록합니다.
- 재사용 가능한 구성요소 상태를 C++ 상태 객체로 이동합니다.
- 구체적인 뷰 ID(`viewId`)를 모델 키에 바인딩합니다.
- 한 번에 하나의 뷰만 하나의 모델 키를 변경할 수 있도록 쓰기 소유권을 명시적으로 제어합니다.
- C++ 부트스트랩 코드에서 ViewModel 수명, 진단 및 등록 정책을 유지합니다.

<a id="viewmodel-shape"></a>

## ViewModel 모양

전용 앱 ViewModels는 `ViewModel`(`src/backend/state/viewmodel.h`)에서 파생되어야 하며 도메인별 Q_PROPERTY 상태 및 호출 가능한 명령을 추가해야 합니다.

기본 클래스는 다음을 제공합니다.

- `key`
- `displayName`
- `busy`
- `error`
- `hasError`
- `metadata`
- `snapshot()`

기본 유형은 QML에서 생성할 수 없습니다. QML는 `ViewModels`를 통해 앱에서 생성된 인스턴스를 받습니다.

`StateModel`(`src/backend/state/statemodel.h`)는 `ViewModel`에서 파생되며 `revision` 및 키별 변경 신호가 포함된 키-값 상태 맵(`values`)을 제공합니다. 전용 유형의 ViewModel가 존재하기 전에 QML에서 일반 구성요소 상태를 마이그레이션할 때 이를 사용하십시오.

<a id="core-api-viewmodels"></a>

## 코어 API (`ViewModels`)

위치: `src/backend/state/viewmodelregistry.h` / `src/backend/state/viewmodelregistry.cpp`

등록 수명주기:

- `set(key, object)`
- `registerViewModel(object, fallbackKey?)`
- `get(key)`
- `remove(key)`
- `clear()`

바인딩 및 소유권:

- `bindView(viewId, key, writable)`
- `unbindView(viewId)`
- `claimOwnership(viewId, key)`
- `releaseOwnership(viewId, key?)`
- `canWrite(viewId, key?)`
- `ownerOf(key)`

데이터 액세스 도우미:

- `updateProperty(viewId, property, value)`
- `updatePropertyByKey(viewId, key, property, value)`
- `readProperty(viewId, property)`

상태/진단:

- `keys`, `views`, `bindings`, `owners`, `descriptors`, `lastError`

`set()`는 수동 또는 호환 배선에 계속 사용할 수 있습니다. 새로운 C++ 부트스트랩 코드는 `registerViewModel()` 또는 `QmlContextBinder`를 선호해야 합니다.

<a id="binding-from-router-metadata"></a>

## 라우터 메타데이터에서 바인딩

`PageRouter`는 경로 메타데이터에 다음이 포함된 경우 자동으로 뷰를 바인딩할 수 있습니다.

- `viewModelKey` 또는 `modelKey`
- `viewId`(선택 사항, 기본값은 경로 경로 또는 생성된 ID)
- `writable` 또는 `modelWritable`

이를 통해 페이지별 상용구 없이 경로 수준 소유권 정책을 사용할 수 있습니다.

<a id="write-ownership-rule"></a>

## 쓰기 소유권 규칙

두 조건이 모두 true인 경우에만 쓰기 액세스가 허용됩니다.

1. 뷰는 모델 키에 바인딩됩니다.
2. 뷰는 해당 키의 현재 소유자입니다.

소유권이 없거나 충돌하는 경우 API가 `false`를 반환하고 `lastError`를 설정하도록 작성하세요.

<a id="c-bootstrap-pattern"></a>

## C++ 부트스트랩 패턴

```cpp
auto *sessionVm = new SessionViewModel(&engine);
sessionVm->setKey(QStringLiteral("Session"));
sessionVm->setDisplayName(QStringLiteral("Session"));

lvrs::QmlContextBindPlan plan;

lvrs::QmlViewModelBinding sessionBinding;
sessionBinding.key = QStringLiteral("Session");
sessionBinding.object = sessionVm;
sessionBinding.contextName = QStringLiteral("sessionViewModel");
sessionBinding.viewId = QStringLiteral("SessionPage");
sessionBinding.writable = true;
plan.viewModels.append(sessionBinding);

const lvrs::QmlContextBindResult result = lvrs::applyQmlContextBindPlan(engine, plan);
if (!result.ok)
    qWarning().noquote() << result.errorMessage();
```

그런 다음 QML는 등록된 객체를 사용합니다.

```qml
import LVRS 1.0 as LV

property var vm: LV.ViewModels.getForView("SessionPage")
```

<a id="how-it-works"></a>

## 동작 원리

- 키와 보기 ID는 트림 기반 토큰 정규화를 통해 정규화됩니다.
- 키가 제거되면 소유권 맵(`key -> viewId`)이 정리됩니다.
- `ViewModels` 에 의해 부모로 연결된 고아 모델 객체는 키 참조가 더 이상 남아있지 않을 때 자동으로 처분됩니다.
- 바인딩 업데이트는 신호 기반(`viewsChanged`, `ownershipChanged`)이므로 QML 관찰자가 반응할 수 있습니다.
- 설명자 업데이트는 신호 기반(`descriptorsChanged`)이므로 도구를 통해 C++ ViewModel 상태 및 현재 소유권을 검사할 수 있습니다.

<a id="failure-modes-to-handle"></a>

## 처리해야 할 실패 모드

- `viewId` 또는 키가 비어 있습니다.
- 알 수 없는 키에 바인딩 중입니다.
- 다른 뷰가 키를 소유하고 있는 동안 쓰기 가능한 바인딩을 시도했습니다.
- 속성이 바인딩되지 않은 뷰 또는 알 수 없는 속성 이름에 기록됩니다.

<a id="advanced-scenario-read-only-detail--writable-editor"></a>

## 고급 시나리오: 읽기 전용 세부 정보 + 쓰기 가능한 편집기

일반적인 패턴은 2 뷰가 동일한 모델 키에 바인딩되되 서로 다른 권한을 갖는 것입니다.

- 상세 페이지: `bindView("Detail", "RunVM", false)`
- 편집기 페이지: `bindView("Editor", "RunVM", true)`

이를 통해 읽기 전용 구성 요소는 하나의 편집기 표면만 모델 속성을 변경할 수 있는 동안 상태를 표시할 수 있습니다.

<a id="ownership-handover-pattern"></a>

## 소유권 이양 패턴

편집 가능한 표면을 전환하는 경우:

1. `releaseOwnership(previousViewId, key)`
2. `claimOwnership(nextViewId, key)`
3. 다음 보기에서만 쓰기 업데이트를 실행합니다.

이 명시적 핸드오버는 자동 쓰기 거부를 방지하고 소유권 감사 가능성을 유지합니다.

<a id="debugging-write-failures"></a>

## 쓰기 실패 디버깅

`updateProperty*`가 `false`를 반환하면 순서대로 검사합니다.

- `ViewModels.lastError`
- `ViewModels.bindings`
- `ViewModels.owners`
- QObject에 대상 속성이 존재하는지 여부

대부분의 런타임 오류는 소유권 누락이나 속성 이름의 오타로 인해 발생합니다.
