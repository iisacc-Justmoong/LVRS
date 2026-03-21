#include "backend/fonts/fontpolicy.h"

#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QGuiApplication>
#include <QStringList>

#include <array>

namespace {
struct TextStyleToken {
    int pixelSize;
    int weight;
    const char *styleName;
};

constexpr std::array<TextStyleToken, 7> kThemeTextTokens = {{
    {26, QFont::Bold, "Bold"},
    {22, QFont::Bold, "Bold"},
    {17, QFont::DemiBold, "SemiBold"},
    {15, QFont::DemiBold, "SemiBold"},
    {12, QFont::Medium, "Medium"},
    {12, QFont::DemiBold, "SemiBold"},
    {11, QFont::Normal, "Regular"}
}};

const QStringList kSubstitutionTargets = {
    QStringLiteral("Arial"),
    QStringLiteral("Helvetica"),
    QStringLiteral("Sans Serif"),
    QStringLiteral("Noto Sans"),
    QStringLiteral("SF Pro Text"),
    QStringLiteral("SF Pro Display"),
    QStringLiteral("system-ui"),
    QStringLiteral("sans-serif")
};

const char *const kBundledPretendardFontResources[] = {
    ":/qt/qml/LVRS/resources/font/Pretendard-Regular.ttf",
    ":/qt/qml/LVRS/resources/font/Pretendard-Medium.ttf",
    ":/qt/qml/LVRS/resources/font/Pretendard-SemiBold.ttf",
    ":/qt/qml/LVRS/resources/font/Pretendard-Bold.ttf",
    ":/qt/qml/LVRS/resources/font/Pretendard-Light.ttf",
    ":/qt/qml/LVRS/resources/font/Pretendard-ExtraLight.ttf",
    ":/qt/qml/LVRS/resources/font/Pretendard-Thin.ttf",
    ":/qt/qml/LVRS/resources/font/Pretendard-ExtraBold.ttf",
    ":/qt/qml/LVRS/resources/font/Pretendard-Black.ttf"
};

bool familyLooksLikePretendard(const QString &family)
{
    return QString::compare(family, QStringLiteral("Pretendard"), Qt::CaseInsensitive) == 0
        || family.contains(QStringLiteral("Pretendard"), Qt::CaseInsensitive);
}

bool currentApplicationFontLooksLikePretendard()
{
    return qGuiApp && familyLooksLikePretendard(qGuiApp->font().family());
}
}

FontPolicy::FontPolicy(QObject *parent)
    : QObject(parent)
{
    loadBundledFonts();
    installPretendardFallbacks();
    if (!currentApplicationFontLooksLikePretendard())
        enforcePretendardFallback();
    refresh();
}

QString FontPolicy::preferredFamily() const
{
    return m_preferredFamily;
}

QString FontPolicy::effectiveFamily() const
{
    return m_effectiveFamily;
}

bool FontPolicy::pretendardAvailable() const
{
    return m_pretendardAvailable;
}

QString FontPolicy::lastWarning() const
{
    return m_lastWarning;
}

bool FontPolicy::enforceApplicationFallback()
{
    if (currentApplicationFontLooksLikePretendard()) {
        refresh();
        setLastWarning(QString());
        return true;
    }

    const bool applied = enforcePretendardFallback();
    refresh();
    if (!applied && !m_pretendardAvailable) {
        setLastWarning(QStringLiteral("Pretendard is unavailable. Falling back to application font."));
    } else {
        setLastWarning(QString());
    }
    return applied;
}

void FontPolicy::loadBundledFonts()
{
    static bool loaded = false;
    if (loaded)
        return;
    loaded = true;

    for (const char *fontResource : kBundledPretendardFontResources)
        QFontDatabase::addApplicationFont(QString::fromLatin1(fontResource));
}

void FontPolicy::refresh()
{
    const QString nextPretendard = detectPretendardFamily();
    const bool nextPretendardAvailable = !nextPretendard.isEmpty();
    const QString nextEffective = detectEffectiveFamily();

    bool changed = false;
    if (m_pretendardAvailable != nextPretendardAvailable) {
        m_pretendardAvailable = nextPretendardAvailable;
        changed = true;
    }
    if (m_effectiveFamily != nextEffective) {
        m_effectiveFamily = nextEffective;
        changed = true;
    }
    if (changed)
        emit effectiveFamilyChanged();
}

QString FontPolicy::resolveFamily(const QString &requestedFamily) const
{
    const QString requested = requestedFamily.trimmed();
    if (requested.isEmpty())
        return m_effectiveFamily;
    if (familyAvailable(requested))
        return requested;
    return m_effectiveFamily;
}

int FontPolicy::weightForTextSize(int pixelSize, int fallbackWeight) const
{
    int mappedWeight = fallbackWeight;
    if (!tokenForPixelSize(pixelSize, &mappedWeight, nullptr))
        return fallbackWeight;
    return mappedWeight;
}

QString FontPolicy::styleNameForTextSize(int pixelSize, const QString &fallbackStyleName) const
{
    QString mappedStyleName = fallbackStyleName;
    if (!tokenForPixelSize(pixelSize, nullptr, &mappedStyleName))
        return fallbackStyleName;
    return mappedStyleName;
}

bool FontPolicy::isThemeTextStyleCompliant(int pixelSize, int weight, const QString &styleName) const
{
    const QString normalizedStyleName = styleName.trimmed();
    for (const TextStyleToken &token : kThemeTextTokens) {
        if (token.pixelSize != pixelSize)
            continue;
        if (token.weight != weight)
            continue;
        if (QString::compare(normalizedStyleName,
                             QString::fromLatin1(token.styleName),
                             Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void FontPolicy::installPretendardFallbacks()
{
    static bool installed = false;
    if (installed)
        return;
    installed = true;

    loadBundledFonts();
    for (const QString &target : kSubstitutionTargets)
        QFont::insertSubstitution(target, QStringLiteral("Pretendard"));
}

bool FontPolicy::enforcePretendardFallback()
{
    loadBundledFonts();
    if (!qGuiApp)
        return false;

    QString pretendardFamily;
    const QStringList families = QFontDatabase::families();
    for (const QString &family : families) {
        if (familyLooksLikePretendard(family)) {
            pretendardFamily = family;
            break;
        }
    }

    if (pretendardFamily.isEmpty())
        return false;

    QFont current = qGuiApp->font();
    if (QString::compare(current.family(), pretendardFamily, Qt::CaseInsensitive) == 0)
        return true;

    current.setFamily(pretendardFamily);
    current.setStyleName(styleNameForWeight(current.weight()));
    qGuiApp->setFont(current);
    const QFontInfo info(qGuiApp->font());
    return QString::compare(info.family(), pretendardFamily, Qt::CaseInsensitive) == 0;
}

bool FontPolicy::tokenForPixelSize(int pixelSize, int *weight, QString *styleName)
{
    const int requestedWeight = weight ? *weight : 0;
    const QString requestedStyleName = styleName ? styleName->trimmed() : QString();

    const TextStyleToken *firstMatch = nullptr;
    const TextStyleToken *preferredByWeight = nullptr;
    const TextStyleToken *preferredByStyle = nullptr;
    const TextStyleToken *preferredByBoth = nullptr;

    for (const TextStyleToken &token : kThemeTextTokens) {
        if (token.pixelSize != pixelSize)
            continue;

        if (!firstMatch)
            firstMatch = &token;

        const bool weightMatch = weight && requestedWeight == token.weight;
        const bool styleMatch = styleName
            && !requestedStyleName.isEmpty()
            && QString::compare(requestedStyleName,
                                QString::fromLatin1(token.styleName),
                                Qt::CaseInsensitive) == 0;

        if (weightMatch && styleMatch) {
            preferredByBoth = &token;
            break;
        }
        if (!preferredByWeight && weightMatch)
            preferredByWeight = &token;
        if (!preferredByStyle && styleMatch)
            preferredByStyle = &token;
    }

    if (!firstMatch)
        return false;

    const TextStyleToken *resolved = preferredByBoth
        ? preferredByBoth
        : (preferredByWeight ? preferredByWeight : (preferredByStyle ? preferredByStyle : firstMatch));

    if (weight)
        *weight = resolved->weight;
    if (styleName)
        *styleName = QString::fromLatin1(resolved->styleName);
    return true;
}

QString FontPolicy::styleNameForWeight(int weight)
{
    if (weight >= QFont::Bold)
        return QStringLiteral("Bold");
    if (weight >= QFont::DemiBold)
        return QStringLiteral("SemiBold");
    if (weight >= QFont::Medium)
        return QStringLiteral("Medium");
    return QStringLiteral("Regular");
}

bool FontPolicy::familyAvailable(const QString &family) const
{
    const QStringList families = QFontDatabase::families();
    for (const QString &candidate : families) {
        if (QString::compare(candidate, family, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

QString FontPolicy::detectPretendardFamily() const
{
    const QStringList families = QFontDatabase::families();
    for (const QString &family : families) {
        if (QString::compare(family, m_preferredFamily, Qt::CaseInsensitive) == 0)
            return family;
    }
    for (const QString &family : families) {
        if (family.contains(m_preferredFamily, Qt::CaseInsensitive))
            return family;
    }
    return QString();
}

QString FontPolicy::detectEffectiveFamily() const
{
    const QString pretendard = detectPretendardFamily();
    if (!pretendard.isEmpty())
        return pretendard;
    if (qGuiApp)
        return qGuiApp->font().family();
    return QFont().family();
}

void FontPolicy::setLastWarning(const QString &message)
{
    if (m_lastWarning == message)
        return;
    m_lastWarning = message;
    emit lastWarningChanged();
}
