# QmlTypeRegistrar

위치: `src/backend/runtime/qmltyperegistrar.h` / `src/backend/runtime/qmltyperegistrar.cpp`

`QmlTypeRegistrar`는 선언된 C++ 매니페스트에서 앱 소유 QML 유형을 등록합니다. 이는 `qmlRegisterType(...)`, `qmlRegisterUncreatableType(...)` 또는 사용자 정의 싱글턴 등록 호출이 오랫동안 반복되는 하위 소비 측 앱을 위한 것입니다.

<a id="purpose"></a>

## 목적

- 유형 등록 순서를 명시적으로 유지하세요.
- 하나의 보고서에 등록 진단을 집계합니다.
- 반복되는 앱 부트스트랩 코드를 더 작게 만듭니다.
- 앱에서 앱 도메인 유형을 유지합니다. LVRS는 등록 하네스만 소유합니다.

## API

- `lvrs::qmlCreatableType<T>(uri, major, minor, qmlName, diagnosticName?, required?)`
- `lvrs::qmlUncreatableType<T>(uri, major, minor, qmlName, reason, diagnosticName?, required?)`
- `lvrs::qmlCustomTypeRegistration(uri, major, minor, qmlName, kind, callback, diagnosticName?, required?)`
- `lvrs::registerQmlTypes(manifest) -> QmlTypeRegistrationReport`

핵심 구조체:

- `QmlTypeRegistration`
- `QmlTypeRegistrationResult`
- `QmlTypeRegistrationReport`

<a id="registration-kinds"></a>

## 등록 종류

- `Creatable`: `qmlRegisterType<T>()`를 래핑합니다.
- `Uncreatable`: `qmlRegisterUncreatableType<T>()`를 래핑합니다.
- `Singleton`: 앱에서 제공하는 싱글턴 콜백을 위해 예약된 진단 종류입니다.
- `Custom`: 앱 제공 콜백.

앱별 구성이 필요한 등록(예: `qmlRegisterSingletonType(...)`, 싱글턴 인스턴스 등록 또는 플랫폼 기반 유형)에는 `qmlCustomTypeRegistration()`를 사용하세요.

<a id="result"></a>

## 결과

`QmlTypeRegistrationReport`에는 다음이 포함됩니다.

- `ok`
- `results`
- `errors`
- `errorMessage()`
- `diagnostics()`

각 결과에는 다음이 포함됩니다.

- `uri`
- `majorVersion`
- `minorVersion`
- `qmlName`
- `qualifiedName`
- `diagnosticName`
- `kind`
- `typeId`
- `ok`
- `skipped`
- `error`

필수 오류는 `report.ok=false`를 설정합니다. 선택적 실패는 `skipped=true`로 표시되며 보고서에 실패하지 않습니다. 두 번째 콜백이 호출되기 전에 중복된 `(uri, version, qmlName)` 항목이 진단됩니다.

<a id="usage"></a>

## 사용법

```cpp
const QList<lvrs::QmlTypeRegistration> manifest = {
    lvrs::qmlCreatableType<WorkspaceDocument>(
        QStringLiteral("WhatSon.Internal"),
        1,
        0,
        QStringLiteral("WorkspaceDocument"),
        QStringLiteral("WorkspaceDocument")),

    lvrs::qmlUncreatableType<WorkspaceCommand>(
        QStringLiteral("WhatSon.Internal"),
        1,
        0,
        QStringLiteral("WorkspaceCommand"),
        QStringLiteral("WorkspaceCommand is constructed by C++"),
        QStringLiteral("WorkspaceCommand"))
};

const lvrs::QmlTypeRegistrationReport report = lvrs::registerQmlTypes(manifest);
if (!report.ok)
    qWarning().noquote() << report.errorMessage();
```

사용자 정의 싱글턴 예:

```cpp
auto singletonRegistration = lvrs::qmlCustomTypeRegistration(
    QStringLiteral("WhatSon.Internal"),
    1,
    0,
    QStringLiteral("WorkspaceServices"),
    lvrs::QmlTypeRegistrationKind::Singleton,
    []() {
        return qmlRegisterSingletonType<WorkspaceServices>(
            "WhatSon.Internal",
            1,
            0,
            "WorkspaceServices",
            [](QQmlEngine *, QJSEngine *) -> QObject * {
                return WorkspaceServices::instance();
            });
    },
    QStringLiteral("WorkspaceServices"));
```

<a id="responsibility-boundary"></a>

## 책임 경계

LVRS 소유:

- 매니페스트 검증,
- 중복 감지,
- 등록 콜백 실행,
- 진단 및 오류 집계.

앱이 소유하는 것:

- 유형 수업,
- 모듈 URI 및 버전 정책,
- 싱글턴 구성 의미론,
- 플랫폼/도메인 게이팅.
