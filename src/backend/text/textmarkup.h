#pragma once

#include <QObject>
#include <QRegularExpression>
#include <QtQml/qqml.h>

class TextMarkup : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TextMarkup)
    QML_SINGLETON

public:
    explicit TextMarkup(QObject *parent = nullptr);

    Q_INVOKABLE QString normalize(const QString &input) const;
    Q_INVOKABLE QString renderHtml(const QString &input) const;
    Q_INVOKABLE QString renderPlainText(const QString &input) const;

private:
    static QString replaceTagPairs(const QString &input,
                                   const QRegularExpression &pattern,
                                   const QString &prefix,
                                   const QString &suffix);
    static QString replaceInlinePattern(const QString &input,
                                        const QRegularExpression &pattern,
                                        const QString &openTag,
                                        const QString &closeTag);
};
