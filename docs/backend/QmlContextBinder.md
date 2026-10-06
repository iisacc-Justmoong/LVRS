# QmlContextBinder

위치: `src/backend/runtime/qmlcontextbinder.h` / `src/backend/runtime/qmlcontextbinder.cpp`

`QmlContextBinder` 는 선포된  C++ 객체 노출 계획을  `QQmlApplicationEngine` 에 적용합니다. 이는  2   부트스트랩 작업으로,  하위 소비 측 앱이 종종 수동으로 반복하는 작업입니다:

- C++ 객체를 QML 컨텍스트 속성으로 설정하고,
- C++ ViewModels를 `LV.ViewModels`에 등록하고 선택적으로 이를 컨텍스트 속성으로 노출합니다.

## API

- `lvrs::applyQmlContextBindPlan(engine, plan) -> QmlContextBindResult`

계획 구조:

- `QmlContextObjectBinding`
- `QmlViewModelBinding`
- `QmlContextBindPlan`
- `QmlContextBindResult`

<a id="context-object-binding"></a>

## 컨텍스트 객체 바인딩

`QmlContextObjectBinding` 필드:

- `contextName`
- `object`
- `required`(기본값 `true`)

필수 null 개체는 오류로 보고됩니다. 선택적 null 개체는 건너뜁니다.

<a id="viewmodel-binding"></a>

## ViewModel 바인딩

`QmlViewModelBinding` 필드:

- `key`
- `object`
- `contextName`
- `displayName`
- `metadata`
- `viewId`
- `writable`
- `required`(기본값 `true`)

개체가 `ViewModel`에서 파생된 경우 바인더는 `key`가 생략된 경우 개체의 `key`를 사용하고 등록 전에 `displayName` / `metadata`를 적용합니다.

`viewId`가 설정된 경우 바인더는 다음도 호출합니다.

```cpp
ViewModels.bindView(viewId, key, writable)
```

이는 소유권 정책을 QML 시작 코드 전체에 분산시키는 대신 C++ 부트스트랩 계획에 유지합니다.

<a id="result"></a>

## 결과

`QmlContextBindResult` 보고서:

- `ok`
- `errors`
- `contextNames`
- `viewModelKeys`
- `errorMessage()`

<a id="usage"></a>

## 사용법

```cpp
lvrs::QmlContextBindPlan plan;

lvrs::QmlContextObjectBinding services;
services.contextName = QStringLiteral("workspaceServices");
services.object = workspaceServices;
plan.contextObjects.append(services);

lvrs::QmlViewModelBinding libraryVm;
libraryVm.key = QStringLiteral("Library");
libraryVm.object = libraryViewModel;
libraryVm.contextName = QStringLiteral("libraryViewModel");
libraryVm.displayName = QStringLiteral("Library");
libraryVm.viewId = QStringLiteral("LibraryView");
libraryVm.writable = true;
plan.viewModels.append(libraryVm);

const lvrs::QmlContextBindResult result =
    lvrs::applyQmlContextBindPlan(engine, plan);
if (!result.ok)
    qWarning().noquote() << result.errorMessage();
```

<a id="responsibility-boundary"></a>

## 책임 경계

LVRS 소유:

- 검증,
- 컨텍스트 속성 할당,
- `ViewModels` 싱글턴 등록,
- 선택적 뷰 바인딩 및 소유권 주장,
- 오류 집계.

앱이 소유하는 것:

- 객체 구성,
- 도메인 키,
- 도메인별 소유권 정책,
- 데이터 로딩 및 돌연변이 의미론.
