# QmlTypeRegistrar

Location: `backend/runtime/qmltyperegistrar.h` / `backend/runtime/qmltyperegistrar.cpp`

`QmlTypeRegistrar` registers app-owned QML types from a declared C++ manifest.
It is intended for downstream apps that have long repeated blocks of `qmlRegisterType(...)`,
`qmlRegisterUncreatableType(...)`, or custom singleton registration calls.

## Purpose

- Keep type registration order explicit.
- Aggregate registration diagnostics in one report.
- Make repeated app bootstrap code smaller.
- Keep app domain types in the app; LVRS only owns the registration harness.

## API

- `lvrs::qmlCreatableType<T>(uri, major, minor, qmlName, diagnosticName?, required?)`
- `lvrs::qmlUncreatableType<T>(uri, major, minor, qmlName, reason, diagnosticName?, required?)`
- `lvrs::qmlCustomTypeRegistration(uri, major, minor, qmlName, kind, callback, diagnosticName?, required?)`
- `lvrs::registerQmlTypes(manifest) -> QmlTypeRegistrationReport`

Core structs:

- `QmlTypeRegistration`
- `QmlTypeRegistrationResult`
- `QmlTypeRegistrationReport`

## Registration Kinds

- `Creatable`: wraps `qmlRegisterType<T>()`.
- `Uncreatable`: wraps `qmlRegisterUncreatableType<T>()`.
- `Singleton`: reserved diagnostic kind for app-provided singleton callbacks.
- `Custom`: app-provided callback.

Use `qmlCustomTypeRegistration()` for registrations that need app-specific construction,
for example `qmlRegisterSingletonType(...)`, singleton instance registration, or platform-gated types.

## Result

`QmlTypeRegistrationReport` contains:

- `ok`
- `results`
- `errors`
- `errorMessage()`
- `diagnostics()`

Each result includes:

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

Required failures set `report.ok=false`. Optional failures are marked as `skipped=true` and do not fail the report.
Duplicate `(uri, version, qmlName)` entries are diagnosed before the second callback is invoked.

## Usage

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

Custom singleton example:

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

## Responsibility Boundary

LVRS owns:

- manifest validation,
- duplicate detection,
- registration callback execution,
- diagnostics and error aggregation.

The app owns:

- type classes,
- module URI and version policy,
- singleton construction semantics,
- platform/domain gating.
