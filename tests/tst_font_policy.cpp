#include <QtTest>

#include <QFont>
#include <QtPlugin>

#include "backend/fonts/fontpolicy.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class FontPolicyTests : public QObject
{
    Q_OBJECT

private slots:
    void font_policy_token_mapping_is_strict();
    void font_policy_family_resolution_and_compliance_edges();
};

void FontPolicyTests::font_policy_token_mapping_is_strict()
{
    FontPolicy policy;
    QVERIFY(!policy.preferredFamily().isEmpty());
    QVERIFY(!policy.effectiveFamily().isEmpty());
    QCOMPARE(policy.resolveFamily(QString()), policy.effectiveFamily());

    struct Token {
        int pixelSize;
        int weight;
        const char *style;
        int fallbackWeight;
        const char *fallbackStyle;
        int doubledPixelSize;
        bool scalable;
    };
    const QList<Token> expected = {
        {26, QFont::Bold, "Bold", QFont::Bold, "Bold", 52, true},
        {22, QFont::Bold, "Bold", QFont::Bold, "Bold", 44, true},
        {17, QFont::DemiBold, "SemiBold", QFont::DemiBold, "SemiBold", 34, true},
        {15, QFont::DemiBold, "SemiBold", QFont::DemiBold, "SemiBold", 30, true},
        {13, QFont::Medium, "Medium", QFont::Medium, "Medium", 26, false},
        {12, QFont::DemiBold, "SemiBold", QFont::DemiBold, "SemiBold", 24, true},
        {11, QFont::Normal, "Regular", QFont::Normal, "Regular", 22, true}
    };

    for (const Token &token : expected) {
        QCOMPARE(policy.weightForTextSize(token.pixelSize, token.fallbackWeight), token.weight);
        QCOMPARE(policy.styleNameForTextSize(token.pixelSize, QString::fromLatin1(token.fallbackStyle)),
                 QString::fromLatin1(token.style));
        QVERIFY2(policy.isThemeTextStyleCompliant(
                     token.pixelSize,
                     token.weight,
                     QString::fromLatin1(token.style)),
                 qPrintable(QStringLiteral("base token pixelSize=%1 style=%2")
                                .arg(token.pixelSize)
                                .arg(QString::fromLatin1(token.style))));

        if (token.scalable) {
            QCOMPARE(policy.weightForTextSize(token.doubledPixelSize, token.fallbackWeight), token.weight);
            QCOMPARE(policy.styleNameForTextSize(token.doubledPixelSize, QString::fromLatin1(token.fallbackStyle)),
                     QString::fromLatin1(token.style));
            QVERIFY(policy.isThemeTextStyleCompliant(token.doubledPixelSize,
                                                     token.weight,
                                                     QString::fromLatin1(token.style)));
        }
    }

    const struct {
        int legacyPixelSize;
        int weight;
        const char *style;
    } legacyScaleTokens[] = {
        {33, QFont::Bold, "Bold"},
        {28, QFont::Bold, "Bold"},
        {21, QFont::DemiBold, "SemiBold"},
        {19, QFont::DemiBold, "SemiBold"},
        {14, QFont::Normal, "Regular"}
    };
    for (const auto &token : legacyScaleTokens) {
        QVERIFY(!policy.isThemeTextStyleCompliant(
            token.legacyPixelSize,
            token.weight,
            QString::fromLatin1(token.style)));
    }

    QVERIFY(!policy.isThemeTextStyleCompliant(16, QFont::Medium, QStringLiteral("Medium")));
    QCOMPARE(policy.weightForTextSize(16, QFont::Light), QFont::Light);
    QCOMPARE(policy.styleNameForTextSize(16, QStringLiteral("Fallback")), QStringLiteral("Fallback"));
    QVERIFY(!policy.isThemeTextStyleCompliant(26, QFont::Medium, QStringLiteral("Medium")));
    QCOMPARE(policy.weightForTextSize(26, QFont::Medium), QFont::Bold);
    QCOMPARE(policy.styleNameForTextSize(26, QStringLiteral("Medium")), QStringLiteral("Bold"));

    QCOMPARE(policy.weightForTextSize(99, QFont::Light), QFont::Light);
    QCOMPARE(policy.styleNameForTextSize(99, QStringLiteral("Fallback")), QStringLiteral("Fallback"));
    QVERIFY(!policy.isThemeTextStyleCompliant(99, QFont::Bold, QStringLiteral("Bold")));
    QVERIFY(!policy.isThemeTextStyleCompliant(13, QFont::Bold, QStringLiteral("Bold")));
}

void FontPolicyTests::font_policy_family_resolution_and_compliance_edges()
{
    FontPolicy policy;
    QVERIFY(!policy.resolveFamily(QStringLiteral("__unlikely_missing_family__")).isEmpty());
    QCOMPARE(policy.resolveFamily(QStringLiteral("__unlikely_missing_family__")), policy.effectiveFamily());
    QVERIFY(policy.isThemeTextStyleCompliant(26, QFont::Bold, QStringLiteral("bold")));
    QVERIFY(!policy.isThemeTextStyleCompliant(26, QFont::Bold, QStringLiteral("SemiBold")));
    QCOMPARE(policy.weightForTextSize(10, QFont::Thin), QFont::Thin);
    QCOMPARE(policy.styleNameForTextSize(10, QStringLiteral("Thin")), QStringLiteral("Thin"));

    const bool applied = policy.enforceApplicationFallback();
    if (!applied && !policy.pretendardAvailable())
        QVERIFY(!policy.lastWarning().isEmpty());
}

QTEST_MAIN(FontPolicyTests)
#include "tst_font_policy.moc"
