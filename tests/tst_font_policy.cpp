#include "QtEventWait.h"
#include <QGuiApplication>

#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QtPlugin>

#include "backend/fonts/fontpolicy.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class FontPolicyTests : public QObject
{
public:
    void font_policy_token_mapping_is_strict();
    void font_policy_family_resolution_and_compliance_edges();
};

void FontPolicyTests::font_policy_token_mapping_is_strict()
{
    FontPolicy policy;
    CTEST_CHECK(!policy.preferredFamily().isEmpty());
    CTEST_COMPARE(policy.systemFamily(), QFontInfo(QFontDatabase::systemFont(QFontDatabase::GeneralFont)).family());
    CTEST_CHECK(!policy.systemFamily().isEmpty());
    CTEST_CHECK(!policy.effectiveFamily().isEmpty());
    CTEST_COMPARE(policy.resolveFamily(QString()), policy.effectiveFamily());

    struct Token {
        int pixelSize;
        int weight;
        const char *style;
        int fallbackWeight;
        const char *fallbackStyle;
    };
    const QList<Token> expected = {
        {26, QFont::Bold, "Bold", QFont::Bold, "Bold"},
        {22, QFont::Bold, "Bold", QFont::Bold, "Bold"},
        {17, QFont::DemiBold, "SemiBold", QFont::DemiBold, "SemiBold"},
        {15, QFont::DemiBold, "SemiBold", QFont::DemiBold, "SemiBold"},
        {13, QFont::Medium, "Medium", QFont::Medium, "Medium"},
        {12, QFont::DemiBold, "SemiBold", QFont::DemiBold, "SemiBold"},
        {11, QFont::Normal, "Regular", QFont::Normal, "Regular"}
    };

    for (const Token &token : expected) {
        CTEST_COMPARE(policy.weightForTextSize(token.pixelSize, token.fallbackWeight), token.weight);
        CTEST_COMPARE(policy.styleNameForTextSize(token.pixelSize, QString::fromLatin1(token.fallbackStyle)),
                 QString::fromLatin1(token.style));
        CTEST_CHECK_MESSAGE(policy.isThemeTextStyleCompliant(
                     token.pixelSize,
                     token.weight,
                     QString::fromLatin1(token.style)),
                 qPrintable(QStringLiteral("base token pixelSize=%1 style=%2")
                                .arg(token.pixelSize)
                                .arg(QString::fromLatin1(token.style))));
    }

    const struct {
        int legacyPixelSize;
        int weight;
        const char *style;
    } legacyScaleTokens[] = {
        {52, QFont::Bold, "Bold"},
        {44, QFont::Bold, "Bold"},
        {34, QFont::DemiBold, "SemiBold"},
        {30, QFont::DemiBold, "SemiBold"},
        {24, QFont::DemiBold, "SemiBold"},
        {33, QFont::Bold, "Bold"},
        {28, QFont::Bold, "Bold"},
        {21, QFont::DemiBold, "SemiBold"},
        {19, QFont::DemiBold, "SemiBold"},
        {14, QFont::Normal, "Regular"}
    };
    for (const auto &token : legacyScaleTokens) {
        CTEST_COMPARE(policy.weightForTextSize(token.legacyPixelSize, QFont::Light), QFont::Light);
        CTEST_COMPARE(policy.styleNameForTextSize(token.legacyPixelSize, QStringLiteral("Fallback")),
                 QStringLiteral("Fallback"));
        CTEST_CHECK(!policy.isThemeTextStyleCompliant(
            token.legacyPixelSize,
            token.weight,
            QString::fromLatin1(token.style)));
    }

    // 22px is Title2, not a doubled Caption alias.
    CTEST_CHECK(!policy.isThemeTextStyleCompliant(22, QFont::Normal, QStringLiteral("Regular")));
    CTEST_COMPARE(policy.weightForTextSize(22, QFont::Normal), QFont::Bold);
    CTEST_COMPARE(policy.styleNameForTextSize(22, QStringLiteral("Regular")), QStringLiteral("Bold"));

    CTEST_CHECK(!policy.isThemeTextStyleCompliant(16, QFont::Medium, QStringLiteral("Medium")));
    CTEST_COMPARE(policy.weightForTextSize(16, QFont::Light), QFont::Light);
    CTEST_COMPARE(policy.styleNameForTextSize(16, QStringLiteral("Fallback")), QStringLiteral("Fallback"));
    CTEST_CHECK(!policy.isThemeTextStyleCompliant(26, QFont::Medium, QStringLiteral("Medium")));
    CTEST_COMPARE(policy.weightForTextSize(26, QFont::Medium), QFont::Bold);
    CTEST_COMPARE(policy.styleNameForTextSize(26, QStringLiteral("Medium")), QStringLiteral("Bold"));

    CTEST_COMPARE(policy.weightForTextSize(99, QFont::Light), QFont::Light);
    CTEST_COMPARE(policy.styleNameForTextSize(99, QStringLiteral("Fallback")), QStringLiteral("Fallback"));
    CTEST_CHECK(!policy.isThemeTextStyleCompliant(99, QFont::Bold, QStringLiteral("Bold")));
    CTEST_CHECK(!policy.isThemeTextStyleCompliant(13, QFont::Bold, QStringLiteral("Bold")));
}

void FontPolicyTests::font_policy_family_resolution_and_compliance_edges()
{
    FontPolicy policy;
    CTEST_CHECK(!policy.resolveFamily(QStringLiteral("__unlikely_missing_family__")).isEmpty());
    CTEST_COMPARE(policy.resolveFamily(QStringLiteral("__unlikely_missing_family__")), policy.effectiveFamily());
    CTEST_CHECK(policy.isThemeTextStyleCompliant(26, QFont::Bold, QStringLiteral("bold")));
    CTEST_CHECK(!policy.isThemeTextStyleCompliant(26, QFont::Bold, QStringLiteral("SemiBold")));
    CTEST_COMPARE(policy.weightForTextSize(10, QFont::Thin), QFont::Thin);
    CTEST_COMPARE(policy.styleNameForTextSize(10, QStringLiteral("Thin")), QStringLiteral("Thin"));

    const bool applied = policy.enforceApplicationFallback();
    if (!applied && !policy.pretendardAvailable())
        CTEST_CHECK(!policy.lastWarning().isEmpty());
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    FontPolicyTests tests;
    runner.run("font_policy_token_mapping_is_strict", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.font_policy_token_mapping_is_strict(); });
    runner.run("font_policy_family_resolution_and_compliance_edges", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.font_policy_family_resolution_and_compliance_edges(); });
    return runner.result();
}
