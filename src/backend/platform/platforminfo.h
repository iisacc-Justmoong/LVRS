#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

#if defined(linux)
#undef linux
#endif

class PlatformInfo : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Platform)
    QML_SINGLETON

    Q_PROPERTY(QString os READ os CONSTANT)
    Q_PROPERTY(QString canonicalOs READ canonicalOs CONSTANT)
    Q_PROPERTY(QString arch READ arch CONSTANT)
    Q_PROPERTY(QString graphicsBackend READ graphicsBackend CONSTANT)

    Q_PROPERTY(bool mobile READ mobile CONSTANT)
    Q_PROPERTY(bool desktop READ desktop CONSTANT)

    Q_PROPERTY(bool android READ android CONSTANT)
    Q_PROPERTY(bool ios READ ios CONSTANT)
    Q_PROPERTY(bool macos READ macos CONSTANT)
    Q_PROPERTY(bool windows READ windows CONSTANT)
    Q_PROPERTY(bool linux READ linux CONSTANT)
    Q_PROPERTY(bool wasm READ wasm CONSTANT)

    Q_PROPERTY(bool metalSupported READ metalSupported CONSTANT)
    Q_PROPERTY(bool vulkanSupported READ vulkanSupported CONSTANT)

    Q_PROPERTY(QStringList runtimeTargets READ runtimeTargets CONSTANT)
    Q_PROPERTY(QStringList desktopTargets READ desktopTargets CONSTANT)
    Q_PROPERTY(QStringList mobileTargets READ mobileTargets CONSTANT)
    Q_PROPERTY(QVariantList runtimeProfiles READ runtimeProfiles CONSTANT)

public:
    explicit PlatformInfo(QObject *parent = nullptr);

    QString os() const;
    QString canonicalOs() const;
    QString arch() const;
    QString graphicsBackend() const;

    bool mobile() const;
    bool desktop() const;

    bool android() const;
    bool ios() const;
    bool macos() const;
    bool windows() const;
    bool linux() const;
    bool wasm() const;

    bool metalSupported() const;
    bool vulkanSupported() const;

    QStringList runtimeTargets() const;
    QStringList desktopTargets() const;
    QStringList mobileTargets() const;
    QVariantList runtimeProfiles() const;

    Q_INVOKABLE QString normalizeTarget(const QString &target) const;
    Q_INVOKABLE bool isKnownTarget(const QString &target) const;
    Q_INVOKABLE bool targetMatchesCurrent(const QString &target) const;
    Q_INVOKABLE bool targetIsMobile(const QString &target) const;
    Q_INVOKABLE bool targetIsDesktop(const QString &target) const;
    Q_INVOKABLE bool supportsTargetGeneration(const QString &target) const;
    Q_INVOKABLE bool backendFeatureReadyFor(const QString &target) const;
    Q_INVOKABLE QString graphicsBackendFor(const QString &target = QString()) const;
    Q_INVOKABLE QVariantMap runtimeProfile(const QString &target = QString()) const;
};
