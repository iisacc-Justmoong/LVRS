#pragma once

#include <QObject>
#include <QPointer>
#include <QtQml/qqml.h>

class RuntimeEvents;

class DebugLogger : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Debug)
    QML_SINGLETON

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool runtimeCaptureEnabled READ runtimeCaptureEnabled WRITE setRuntimeCaptureEnabled NOTIFY runtimeCaptureEnabledChanged)
    Q_PROPERTY(bool runtimeEchoEnabled READ runtimeEchoEnabled WRITE setRuntimeEchoEnabled NOTIFY runtimeEchoEnabledChanged)
    Q_PROPERTY(int runtimeEchoMinIntervalMs READ runtimeEchoMinIntervalMs WRITE setRuntimeEchoMinIntervalMs NOTIFY runtimeEchoMinIntervalMsChanged)
    Q_PROPERTY(QStringList runtimeEchoExcludeTypes READ runtimeEchoExcludeTypes WRITE setRuntimeEchoExcludeTypes NOTIFY runtimeEchoExcludeTypesChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool verboseOutput READ verboseOutput WRITE setVerboseOutput NOTIFY verboseOutputChanged)
    Q_PROPERTY(bool jsonOutput READ jsonOutput WRITE setJsonOutput NOTIFY jsonOutputChanged)
    Q_PROPERTY(QString stdoutMinimumLevel READ stdoutMinimumLevel WRITE setStdoutMinimumLevel NOTIFY stdoutMinimumLevelChanged)
    Q_PROPERTY(bool stdoutNoiseReductionEnabled READ stdoutNoiseReductionEnabled WRITE setStdoutNoiseReductionEnabled NOTIFY stdoutNoiseReductionEnabledChanged)
    Q_PROPERTY(int maxEntries READ maxEntries WRITE setMaxEntries NOTIFY maxEntriesChanged)
    Q_PROPERTY(int entryCount READ entryCount NOTIFY entriesChanged)
    Q_PROPERTY(int droppedCount READ droppedCount NOTIFY entriesChanged)
    Q_PROPERTY(qulonglong sequence READ sequence NOTIFY entriesChanged)
    Q_PROPERTY(bool runtimeAttached READ runtimeAttached NOTIFY runtimeAttachedChanged)
    Q_PROPERTY(QStringList levelFilter READ levelFilter WRITE setLevelFilter NOTIFY levelFilterChanged)
    Q_PROPERTY(QStringList componentFilter READ componentFilter WRITE setComponentFilter NOTIFY componentFilterChanged)
    Q_PROPERTY(QString textFilter READ textFilter WRITE setTextFilter NOTIFY textFilterChanged)
    Q_PROPERTY(QVariantMap lastEntry READ lastEntry NOTIFY entriesChanged)

public:
    explicit DebugLogger(QObject *parent = nullptr);

    bool enabled() const;
    void setEnabled(bool value);
    bool runtimeCaptureEnabled() const;
    void setRuntimeCaptureEnabled(bool value);
    bool runtimeEchoEnabled() const;
    void setRuntimeEchoEnabled(bool value);
    int runtimeEchoMinIntervalMs() const;
    void setRuntimeEchoMinIntervalMs(int value);
    QStringList runtimeEchoExcludeTypes() const;
    void setRuntimeEchoExcludeTypes(const QStringList &values);
    bool paused() const;
    void setPaused(bool value);
    bool verboseOutput() const;
    void setVerboseOutput(bool value);
    bool jsonOutput() const;
    void setJsonOutput(bool value);
    QString stdoutMinimumLevel() const;
    void setStdoutMinimumLevel(const QString &value);
    bool stdoutNoiseReductionEnabled() const;
    void setStdoutNoiseReductionEnabled(bool value);
    int maxEntries() const;
    void setMaxEntries(int value);
    int entryCount() const;
    int droppedCount() const;
    qulonglong sequence() const;
    bool runtimeAttached() const;
    QStringList levelFilter() const;
    void setLevelFilter(const QStringList &values);
    QStringList componentFilter() const;
    void setComponentFilter(const QStringList &values);
    QString textFilter() const;
    void setTextFilter(const QString &value);
    QVariantMap lastEntry() const;

    Q_INVOKABLE void log(const QString &component, const QString &event, const QVariant &data = QVariant());
    Q_INVOKABLE void warn(const QString &component, const QString &event, const QVariant &data = QVariant());
    Q_INVOKABLE void error(const QString &component, const QString &event, const QVariant &data = QVariant());
    Q_INVOKABLE void attachRuntimeEvents();
    Q_INVOKABLE void detachRuntimeEvents();
    Q_INVOKABLE void clearEntries();
    Q_INVOKABLE QVariantList entries(int limit = -1) const;
    Q_INVOKABLE QVariantList filteredEntries(int limit = -1) const;
    Q_INVOKABLE QVariantMap summary() const;
    Q_INVOKABLE void setFilters(const QStringList &levels,
                                const QStringList &components,
                                const QString &text);

signals:
    void enabledChanged();
    void runtimeCaptureEnabledChanged();
    void runtimeEchoEnabledChanged();
    void runtimeEchoMinIntervalMsChanged();
    void runtimeEchoExcludeTypesChanged();
    void pausedChanged();
    void verboseOutputChanged();
    void jsonOutputChanged();
    void stdoutMinimumLevelChanged();
    void stdoutNoiseReductionEnabledChanged();
    void maxEntriesChanged();
    void entriesChanged();
    void runtimeAttachedChanged();
    void levelFilterChanged();
    void componentFilterChanged();
    void textFilterChanged();
    void entryAdded(const QVariantMap &entry);

private:
    RuntimeEvents *resolveRuntimeEvents() const;
    static QString normalizedToken(const QString &value);
    static int levelPriority(const QString &level);
    QVariantMap appendEntry(const QVariantMap &entry, bool forceAppend = false);
    QVariantMap makeLogEntry(const QString &level,
                             const QString &component,
                             const QString &event,
                             const QVariant &data) const;
    QVariantMap makeRuntimeEntry(const QVariantMap &eventData) const;
    QVariantList boundedEntries(const QVariantList &source, int limit) const;
    bool matchesFilters(const QVariantMap &entry) const;
    bool shouldOutputLevel(const QString &level) const;
    bool isStdoutNoiseEntry(const QVariantMap &entry) const;
    bool shouldEchoRuntimeEntry(const QVariantMap &entry, qint64 nowMs);
    void printSuppressedRuntimeEchoSummary(qint64 nowMs);
    QString formatStdoutMessage(const QVariantMap &entry) const;
    QString variantToCompactJson(const QVariant &value) const;
    void printEntryToStdout(const QVariantMap &entry);
    void output(const QString &level, const QString &component, const QString &event, const QVariant &data);

    bool m_enabled = false;
    bool m_runtimeCaptureEnabled = true;
    bool m_runtimeEchoEnabled = false;
    int m_runtimeEchoMinIntervalMs = 250;
    QStringList m_runtimeEchoExcludeTypes = {
        QStringLiteral("mouse-move"),
        QStringLiteral("hover-move"),
        QStringLiteral("ui-event"),
        QStringLiteral("mouse-wheel"),
        QStringLiteral("mouse-press"),
        QStringLiteral("mouse-release"),
        QStringLiteral("key-press"),
        QStringLiteral("key-release"),
        QStringLiteral("touch-event"),
        QStringLiteral("tablet-event"),
        QStringLiteral("native-gesture")
    };
    bool m_paused = false;
    bool m_verboseOutput = false;
    bool m_jsonOutput = false;
    QString m_stdoutMinimumLevel = QStringLiteral("WARN");
    bool m_stdoutNoiseReductionEnabled = true;
    int m_maxEntries = 512;
    int m_droppedCount = 0;
    qulonglong m_sequence = 0;
    qint64 m_sessionStartEpochMs = 0;
    qint64 m_lastRuntimeEchoEpochMs = -1;
    int m_suppressedRuntimeEchoCount = 0;
    QVariantList m_entries;
    QVariantMap m_lastEntry;
    QStringList m_levelFilter;
    QStringList m_componentFilter;
    QString m_textFilter;
    QPointer<RuntimeEvents> m_runtimeEvents;
    QMetaObject::Connection m_runtimeRecordedConnection;
    QMetaObject::Connection m_runtimeDestroyedConnection;
};
