#include "CTestSupport.h"
#include <QGuiApplication>

#include <QtPlugin>

#include "backend/text/textmarkup.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class TextMarkupTests
{
public:
    void normalize_converts_html_markup_to_canonical_text();
    void render_html_is_equivalent_for_markdown_and_html_inputs();
    void render_plain_text_returns_normalized_text();
};

void TextMarkupTests::normalize_converts_html_markup_to_canonical_text()
{
    TextMarkup markup;

    const QString input = QStringLiteral("<p>Hello <strong>bold</strong></p><p><em>world</em><br/>`x`</p>");
    const QString normalized = markup.normalize(input);

    CTEST_COMPARE(normalized, QStringLiteral("\nHello **bold**\n\n*world*\n`x`\n"));
}

void TextMarkupTests::render_html_is_equivalent_for_markdown_and_html_inputs()
{
    TextMarkup markup;

    const QString markdownInput = QStringLiteral("Hello **bold** and *accent* with `code`");
    const QString richInput = QStringLiteral("Hello <strong>bold</strong> and <em>accent</em> with <code>code</code>");

    CTEST_COMPARE(markup.renderHtml(markdownInput), markup.renderHtml(richInput));
}

void TextMarkupTests::render_plain_text_returns_normalized_text()
{
    TextMarkup markup;

    const QString input = QStringLiteral("A\r\nB<br/>C");
    CTEST_COMPARE(markup.renderPlainText(input), markup.normalize(input));
    CTEST_COMPARE(markup.renderPlainText(input), QStringLiteral("A\nB\nC"));
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner;
    TextMarkupTests tests;
    runner.run("normalize_converts_html_markup_to_canonical_text", [&] { tests.normalize_converts_html_markup_to_canonical_text(); });
    runner.run("render_html_is_equivalent_for_markdown_and_html_inputs", [&] { tests.render_html_is_equivalent_for_markdown_and_html_inputs(); });
    runner.run("render_plain_text_returns_normalized_text", [&] { tests.render_plain_text_returns_normalized_text(); });
    return runner.result();
}
