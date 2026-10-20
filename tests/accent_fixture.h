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

inline QColor blueComposite(qreal opacity)
{
    for (const auto &entry : colors()) {
        const auto row = entry.toObject();
        if (row["property"].toString() == QStringLiteral("accentBlue")) {
            const auto rgba = row["rgba"].toObject();
            constexpr qreal base = 36.0 / 255.0;
            return QColor::fromRgbF(base * (1 - opacity) + rgba["r"].toDouble() * opacity,
                                    base * (1 - opacity) + rgba["g"].toDouble() * opacity,
                                    base * (1 - opacity) + rgba["b"].toDouble() * opacity);
        }
    }
    return {};
}
} // namespace AccentFixture
