#include "backend/text/textmarkup.h"

#include <QStringList>

TextMarkup::TextMarkup(QObject *parent)
    : QObject(parent)
{
}

QString TextMarkup::normalize(const QString &input) const
{
    QString normalized = input;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QChar('\r'), QChar('\n'));

    const QRegularExpression brTag(QStringLiteral("<\\s*br\\s*/?\\s*>"),
                                   QRegularExpression::CaseInsensitiveOption);
    normalized.replace(brTag, QStringLiteral("\n"));

    const QRegularExpression paragraphTag(QStringLiteral("<\\s*/?\\s*p\\s*>"),
                                          QRegularExpression::CaseInsensitiveOption);
    normalized.replace(paragraphTag, QStringLiteral("\n"));

    const QRegularExpression strongTag(
        QStringLiteral("<\\s*(strong|b)\\s*>(.*?)<\\s*/\\s*\\1\\s*>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    normalized = replaceTagPairs(normalized, strongTag, QStringLiteral("**"), QStringLiteral("**"));

    const QRegularExpression emTag(
        QStringLiteral("<\\s*(em|i)\\s*>(.*?)<\\s*/\\s*\\1\\s*>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    normalized = replaceTagPairs(normalized, emTag, QStringLiteral("*"), QStringLiteral("*"));

    const QRegularExpression codeTag(
        QStringLiteral("<\\s*(code)\\s*>(.*?)<\\s*/\\s*\\1\\s*>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    normalized = replaceTagPairs(normalized, codeTag, QStringLiteral("`"), QStringLiteral("`"));

    const QRegularExpression remainingTags(QStringLiteral("<[^>]+>"));
    normalized.remove(remainingTags);

    return normalized;
}

QString TextMarkup::renderHtml(const QString &input) const
{
    QString escaped = normalize(input).toHtmlEscaped();

    QStringList codeReplacements;
    const QRegularExpression codePattern(QStringLiteral("`([^`\\n]+)`"));
    for (;;) {
        const QRegularExpressionMatch match = codePattern.match(escaped);
        if (!match.hasMatch())
            break;
        const QString placeholder = QStringLiteral("{{{CODE_%1}}}").arg(codeReplacements.size());
        codeReplacements.append(QStringLiteral("<code>%1</code>").arg(match.captured(1)));
        escaped.replace(match.capturedStart(0), match.capturedLength(0), placeholder);
    }

    escaped = replaceInlinePattern(escaped,
                                   QRegularExpression(QStringLiteral("\\*\\*([^*\\n][^\\n]*?)\\*\\*")),
                                   QStringLiteral("<strong>"),
                                   QStringLiteral("</strong>"));
    escaped = replaceInlinePattern(escaped,
                                   QRegularExpression(QStringLiteral("__([^_\\n][^\\n]*?)__")),
                                   QStringLiteral("<strong>"),
                                   QStringLiteral("</strong>"));
    escaped = replaceInlinePattern(escaped,
                                   QRegularExpression(QStringLiteral("(?<!\\*)\\*([^*\\n]+)\\*(?!\\*)")),
                                   QStringLiteral("<em>"),
                                   QStringLiteral("</em>"));
    escaped = replaceInlinePattern(escaped,
                                   QRegularExpression(QStringLiteral("(?<!_)_([^_\\n]+)_(?!_)")),
                                   QStringLiteral("<em>"),
                                   QStringLiteral("</em>"));

    escaped.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));

    for (int index = 0; index < codeReplacements.size(); ++index) {
        const QString placeholder = QStringLiteral("{{{CODE_%1}}}").arg(index);
        escaped.replace(placeholder, codeReplacements.at(index));
    }

    return escaped;
}

QString TextMarkup::renderPlainText(const QString &input) const
{
    return normalize(input);
}

QString TextMarkup::replaceTagPairs(const QString &input,
                                    const QRegularExpression &pattern,
                                    const QString &prefix,
                                    const QString &suffix)
{
    QString output = input;
    for (;;) {
        const QRegularExpressionMatch match = pattern.match(output);
        if (!match.hasMatch())
            break;
        const QString replacement = prefix + match.captured(2) + suffix;
        output.replace(match.capturedStart(0), match.capturedLength(0), replacement);
    }
    return output;
}

QString TextMarkup::replaceInlinePattern(const QString &input,
                                         const QRegularExpression &pattern,
                                         const QString &openTag,
                                         const QString &closeTag)
{
    QString output = input;
    for (;;) {
        const QRegularExpressionMatch match = pattern.match(output);
        if (!match.hasMatch())
            break;
        const QString replacement = openTag + match.captured(1) + closeTag;
        output.replace(match.capturedStart(0), match.capturedLength(0), replacement);
    }
    return output;
}
