#pragma once

#include <QColor>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace AccentFixture {
inline QJsonArray colors()
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/accent-palette.json"));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object()["colors"].toArray();
}

inline QColor color(const QString &property)
{
    for (const auto &entry : colors()) {
        const auto row = entry.toObject();
        if (row["property"].toString() == property) {
            const auto rgba = row["rgba"].toObject();
            return QColor::fromRgbF(rgba["r"].toDouble(), rgba["g"].toDouble(),
                                    rgba["b"].toDouble(), rgba["a"].toDouble());
        }
    }
    return {};
}

inline QColor blueWithAlpha(qreal alpha)
{
    QColor value = color(QStringLiteral("accentBlue"));
    value.setAlphaF(alpha);
    return value;
}

inline QColor background(const QString &name)
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-accent-roles.json"));
    if (!file.open(QIODevice::ReadOnly)) return {};
    for (const auto &entry : QJsonDocument::fromJson(file.readAll()).object()["backgroundVariables"].toArray()) {
        const auto row = entry.toObject();
        if (row["name"].toString() != name) continue;
        const auto rgba = row["value"].toObject();
        return QColor::fromRgbF(rgba["r"].toDouble(), rgba["g"].toDouble(),
                                rgba["b"].toDouble(), rgba["a"].toDouble());
    }
    return {};
}

inline QColor role(const QString &colorName, const QString &roleName)
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-accent-roles.json"));
    if (!file.open(QIODevice::ReadOnly)) return {};
    for (const auto &entry : QJsonDocument::fromJson(file.readAll()).object()["variables"].toArray()) {
        const auto row = entry.toObject();
        if (row["name"].toString() != "Accent/" + colorName + "/" + roleName) continue;
        const auto rgba = row["value"].toObject();
        return QColor::fromRgbF(rgba["r"].toDouble(), rgba["g"].toDouble(),
                                rgba["b"].toDouble(), rgba["a"].toDouble());
    }
    return {};
}

inline QColor blueComposite(qreal opacity)
{
    const QString state = qFuzzyCompare(opacity, qreal(.25)) ? "Selected"
        : qFuzzyCompare(opacity, qreal(.16)) ? "Subtle"
        : qFuzzyCompare(opacity, qreal(.36)) ? "Outline" : QString();
    return role(QStringLiteral("Blue"), state);
}
} // namespace AccentFixture
