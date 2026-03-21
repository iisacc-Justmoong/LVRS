#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

class FontPolicy : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FontPolicy)
    QML_SINGLETON

    Q_PROPERTY(QString preferredFamily READ preferredFamily CONSTANT)
    Q_PROPERTY(QString effectiveFamily READ effectiveFamily NOTIFY effectiveFamilyChanged)
    Q_PROPERTY(bool pretendardAvailable READ pretendardAvailable NOTIFY effectiveFamilyChanged)
    Q_PROPERTY(QString lastWarning READ lastWarning NOTIFY lastWarningChanged)

public:
    explicit FontPolicy(QObject *parent = nullptr);

    QString preferredFamily() const;
    QString effectiveFamily() const;
    bool pretendardAvailable() const;
    QString lastWarning() const;

    Q_INVOKABLE bool enforceApplicationFallback();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString resolveFamily(const QString &requestedFamily) const;
    Q_INVOKABLE int weightForTextSize(int pixelSize, int fallbackWeight) const;
    Q_INVOKABLE QString styleNameForTextSize(int pixelSize, const QString &fallbackStyleName) const;
    Q_INVOKABLE bool isThemeTextStyleCompliant(int pixelSize, int weight, const QString &styleName) const;

    static void loadBundledFonts();
    static void installPretendardFallbacks();
    static bool enforcePretendardFallback();

signals:
    void effectiveFamilyChanged();
    void lastWarningChanged();

private:
    static bool tokenForPixelSize(int pixelSize, int *weight, QString *styleName);
    static QString styleNameForWeight(int weight);

    bool familyAvailable(const QString &family) const;
    QString detectPretendardFamily() const;
    QString detectEffectiveFamily() const;
    void setLastWarning(const QString &message);

    QString m_preferredFamily = QStringLiteral("Pretendard");
    QString m_effectiveFamily = QStringLiteral("Pretendard");
    bool m_pretendardAvailable = false;
    QString m_lastWarning;
};
