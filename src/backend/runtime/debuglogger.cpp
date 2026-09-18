#include "backend/runtime/debuglogger.h"

#include "backend/runtime/runtimeevents.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QThread>
#include <QTimeZone>

namespace {

QStringList normalizeFilterList(const QStringList &raw)
{
    QStringList normalized;
    for (const QString &value : raw) {
        const QString token = value.trimmed().toLower();
        if (token.isEmpty() || normalized.contains(token))
            continue;
        normalized.append(token);
    }
    return normalized;
}

QString normalizeLevelName(const QString &rawLevel)
{
    const QString level = rawLevel.trimmed().toUpper();
    if (level == QStringLiteral("WARN")
        || level == QStringLiteral("WARNING")) {
        return QStringLiteral("WARN");
    }
    if (level == QStringLiteral("ERR")
        || level == QStringLiteral("ERROR")) {
        return QStringLiteral("ERROR");
    }
    if (level == QStringLiteral("RUNTIME"))
        return QStringLiteral("RUNTIME");
    if (level == QStringLiteral("NONE")
        || level == QStringLiteral("OFF")) {
        return QStringLiteral("NONE");
    }
    return QStringLiteral("LOG");
}

bool containsToken(const QStringList &tokens, const QString &value)
{
    return tokens.contains(value.trimmed().toLower());
}

QString formatTimestampCentiseconds(qint64 epochMs)
{
    const QTime time = QDateTime::fromMSecsSinceEpoch(epochMs).time();
    const int centiseconds = time.msec() / 10;
    return QStringLiteral("%1:%2:%3.%4")
        .arg(time.hour(), 2, 10, QChar('0'))
        .arg(time.minute(), 2, 10, QChar('0'))
        .arg(time.second(), 2, 10, QChar('0'))
        .arg(centiseconds, 2, 10, QChar('0'));
}

} // namespace

DebugLogger::DebugLogger(QObject *parent)
    : QObject(parent)
{
    m_sessionStartEpochMs = QDateTime::currentMSecsSinceEpoch();
    m_runtimeEchoExcludeTypes = normalizeFilterList(m_runtimeEchoExcludeTypes);
}

bool DebugLogger::enabled() const
{
    return m_enabled;
}

void DebugLogger::setEnabled(bool value)
{
    if (m_enabled == value)
        return;
    m_enabled = value;
    emit enabledChanged();
}

bool DebugLogger::runtimeCaptureEnabled() const
{
    return m_runtimeCaptureEnabled;
}

void DebugLogger::setRuntimeCaptureEnabled(bool value)
{
    if (m_runtimeCaptureEnabled == value)
        return;
    m_runtimeCaptureEnabled = value;
    if (m_runtimeCaptureEnabled)
        attachRuntimeEvents();
    emit runtimeCaptureEnabledChanged();
}

bool DebugLogger::runtimeEchoEnabled() const
{
    return m_runtimeEchoEnabled;
}

void DebugLogger::setRuntimeEchoEnabled(bool value)
{
    if (m_runtimeEchoEnabled == value)
        return;
    m_runtimeEchoEnabled = value;
    emit runtimeEchoEnabledChanged();
}

int DebugLogger::runtimeEchoMinIntervalMs() const
{
    return m_runtimeEchoMinIntervalMs;
}

void DebugLogger::setRuntimeEchoMinIntervalMs(int value)
{
    const int bounded = qMax(0, value);
    if (m_runtimeEchoMinIntervalMs == bounded)
        return;
    m_runtimeEchoMinIntervalMs = bounded;
    emit runtimeEchoMinIntervalMsChanged();
}

QStringList DebugLogger::runtimeEchoExcludeTypes() const
{
    return m_runtimeEchoExcludeTypes;
}

void DebugLogger::setRuntimeEchoExcludeTypes(const QStringList &values)
{
    const QStringList next = normalizeFilterList(values);
    if (m_runtimeEchoExcludeTypes == next)
        return;
    m_runtimeEchoExcludeTypes = next;
    emit runtimeEchoExcludeTypesChanged();
}

bool DebugLogger::paused() const
{
    return m_paused;
}

void DebugLogger::setPaused(bool value)
{
    if (m_paused == value)
        return;
    m_paused = value;
    emit pausedChanged();
}

bool DebugLogger::verboseOutput() const
{
    return m_verboseOutput;
}

void DebugLogger::setVerboseOutput(bool value)
{
    if (m_verboseOutput == value)
        return;
    m_verboseOutput = value;
    emit verboseOutputChanged();
}

bool DebugLogger::jsonOutput() const
{
    return m_jsonOutput;
}

void DebugLogger::setJsonOutput(bool value)
{
    if (m_jsonOutput == value)
        return;
    m_jsonOutput = value;
    emit jsonOutputChanged();
}

QString DebugLogger::stdoutMinimumLevel() const
{
    return m_stdoutMinimumLevel;
}

void DebugLogger::setStdoutMinimumLevel(const QString &value)
{
    const QString next = normalizeLevelName(value);
    if (m_stdoutMinimumLevel == next)
        return;
    m_stdoutMinimumLevel = next;
    emit stdoutMinimumLevelChanged();
}

bool DebugLogger::stdoutNoiseReductionEnabled() const
{
    return m_stdoutNoiseReductionEnabled;
}

void DebugLogger::setStdoutNoiseReductionEnabled(bool value)
{
    if (m_stdoutNoiseReductionEnabled == value)
        return;
    m_stdoutNoiseReductionEnabled = value;
    emit stdoutNoiseReductionEnabledChanged();
}

int DebugLogger::maxEntries() const
{
    return m_maxEntries;
}

void DebugLogger::setMaxEntries(int value)
{
    const int bounded = qMax(1, value);
    if (m_maxEntries == bounded)
        return;
    m_maxEntries = bounded;
    if (m_entries.size() > m_maxEntries) {
        const int overflow = m_entries.size() - m_maxEntries;
        m_entries = m_entries.mid(overflow);
        m_droppedCount += overflow;
        emit entriesChanged();
    }
    emit maxEntriesChanged();
}

int DebugLogger::entryCount() const
{
    return m_entries.size();
}

int DebugLogger::droppedCount() const
{
    return m_droppedCount;
}

qulonglong DebugLogger::sequence() const
{
    return m_sequence;
}

bool DebugLogger::runtimeAttached() const
{
    return !m_runtimeEvents.isNull();
}

QStringList DebugLogger::levelFilter() const
{
    return m_levelFilter;
}

void DebugLogger::setLevelFilter(const QStringList &values)
{
    const QStringList next = normalizeFilterList(values);
    if (m_levelFilter == next)
        return;
    m_levelFilter = next;
    emit levelFilterChanged();
}

QStringList DebugLogger::componentFilter() const
{
    return m_componentFilter;
}

void DebugLogger::setComponentFilter(const QStringList &values)
{
    const QStringList next = normalizeFilterList(values);
    if (m_componentFilter == next)
        return;
    m_componentFilter = next;
    emit componentFilterChanged();
}

QString DebugLogger::textFilter() const
{
    return m_textFilter;
}

void DebugLogger::setTextFilter(const QString &value)
{
    const QString next = value.trimmed().toLower();
    if (m_textFilter == next)
        return;
    m_textFilter = next;
    emit textFilterChanged();
}

QVariantMap DebugLogger::lastEntry() const
{
    return m_lastEntry;
}

void DebugLogger::log(const QString &component, const QString &event, const QVariant &data)
{
    output(QStringLiteral("LOG"), component, event, data);
}

void DebugLogger::warn(const QString &component, const QString &event, const QVariant &data)
{
    output(QStringLiteral("WARN"), component, event, data);
}

void DebugLogger::error(const QString &component, const QString &event, const QVariant &data)
{
    output(QStringLiteral("ERROR"), component, event, data);
}

void DebugLogger::attachRuntimeEvents()
{
    if (!m_runtimeCaptureEnabled)
        return;
    if (!m_runtimeEvents.isNull())
        return;
    RuntimeEvents *runtime = resolveRuntimeEvents();
    if (!runtime)
        return;

    m_runtimeEvents = runtime;
    m_runtimeRecordedConnection = connect(runtime,
                                          &RuntimeEvents::eventRecorded,
                                          this,
                                          [this](const QVariantMap &eventData) {
                                              if (!m_runtimeCaptureEnabled)
                                                  return;
                                              if (!m_enabled)
                                                  return;
                                              const QVariantMap appended = appendEntry(makeRuntimeEntry(eventData));
                                              if (appended.isEmpty())
                                                  return;
                                              if (!m_runtimeEchoEnabled)
                                                  return;
                                              const qint64 nowMs = appended.value(QStringLiteral("timestampEpochMs"))
                                                                    .toLongLong();
                                              if (!shouldEchoRuntimeEntry(appended, nowMs))
                                                  return;
                                              printSuppressedRuntimeEchoSummary(nowMs);
                                              printEntryToStdout(appended);
                                          });
    m_runtimeDestroyedConnection = connect(runtime,
                                           &QObject::destroyed,
                                           this,
                                           [this]() {
                                               m_runtimeEvents.clear();
                                               m_runtimeRecordedConnection = QMetaObject::Connection();
                                               m_runtimeDestroyedConnection = QMetaObject::Connection();
                                               emit runtimeAttachedChanged();
                                           });
    emit runtimeAttachedChanged();
}

void DebugLogger::detachRuntimeEvents()
{
    if (m_runtimeRecordedConnection)
        QObject::disconnect(m_runtimeRecordedConnection);
    if (m_runtimeDestroyedConnection)
        QObject::disconnect(m_runtimeDestroyedConnection);
    m_runtimeRecordedConnection = QMetaObject::Connection();
    m_runtimeDestroyedConnection = QMetaObject::Connection();
    if (!m_runtimeEvents.isNull()) {
        m_runtimeEvents.clear();
        emit runtimeAttachedChanged();
    }
}

void DebugLogger::clearEntries()
{
    if (m_entries.isEmpty() && m_lastEntry.isEmpty() && m_droppedCount == 0)
        return;
    m_entries.clear();
    m_lastEntry.clear();
    m_droppedCount = 0;
    emit entriesChanged();
}

QVariantList DebugLogger::entries(int limit) const
{
    return boundedEntries(m_entries, limit);
}

QVariantList DebugLogger::filteredEntries(int limit) const
{
    QVariantList filtered;
    filtered.reserve(m_entries.size());
    for (const QVariant &value : m_entries) {
        const QVariantMap entry = value.toMap();
        if (!matchesFilters(entry))
            continue;
        filtered.append(entry);
    }
    return boundedEntries(filtered, limit);
}

QVariantMap DebugLogger::summary() const
{
    QVariantMap levelCounts;
    QVariantMap componentCounts;
    for (const QVariant &value : m_entries) {
        const QVariantMap entry = value.toMap();
        const QString level = entry.value(QStringLiteral("level")).toString();
        const QString component = entry.value(QStringLiteral("component")).toString();
        if (!level.isEmpty())
            levelCounts.insert(level,
                               levelCounts.value(level, 0).toInt() + 1);
        if (!component.isEmpty())
            componentCounts.insert(component,
                                   componentCounts.value(component, 0).toInt() + 1);
    }

    QVariantMap map;
    map.insert(QStringLiteral("enabled"), m_enabled);
    map.insert(QStringLiteral("runtimeCaptureEnabled"), m_runtimeCaptureEnabled);
    map.insert(QStringLiteral("runtimeEchoEnabled"), m_runtimeEchoEnabled);
    map.insert(QStringLiteral("runtimeEchoMinIntervalMs"), m_runtimeEchoMinIntervalMs);
    map.insert(QStringLiteral("runtimeEchoExcludeTypes"), m_runtimeEchoExcludeTypes);
    map.insert(QStringLiteral("paused"), m_paused);
    map.insert(QStringLiteral("verboseOutput"), m_verboseOutput);
    map.insert(QStringLiteral("jsonOutput"), m_jsonOutput);
    map.insert(QStringLiteral("stdoutMinimumLevel"), m_stdoutMinimumLevel);
    map.insert(QStringLiteral("stdoutNoiseReductionEnabled"), m_stdoutNoiseReductionEnabled);
    map.insert(QStringLiteral("runtimeAttached"), runtimeAttached());
    map.insert(QStringLiteral("maxEntries"), m_maxEntries);
    map.insert(QStringLiteral("entryCount"), m_entries.size());
    map.insert(QStringLiteral("filteredCount"), filteredEntries(-1).size());
    map.insert(QStringLiteral("droppedCount"), m_droppedCount);
    map.insert(QStringLiteral("sequence"), QVariant::fromValue(m_sequence));
    map.insert(QStringLiteral("suppressedRuntimeEchoCount"), m_suppressedRuntimeEchoCount);
    map.insert(QStringLiteral("sessionStartEpochMs"), QVariant::fromValue(m_sessionStartEpochMs));
    map.insert(QStringLiteral("levelCounts"), levelCounts);
    map.insert(QStringLiteral("componentCounts"), componentCounts);
    map.insert(QStringLiteral("lastEntry"), m_lastEntry);
    return map;
}

void DebugLogger::setFilters(const QStringList &levels,
                             const QStringList &components,
                             const QString &text)
{
    setLevelFilter(levels);
    setComponentFilter(components);
    setTextFilter(text);
}

RuntimeEvents *DebugLogger::resolveRuntimeEvents() const
{
    if (RuntimeEvents::instance())
        return RuntimeEvents::instance();
    if (!qApp)
        return nullptr;
    const QList<RuntimeEvents *> runtimes = qApp->findChildren<RuntimeEvents *>();
    if (runtimes.isEmpty())
        return nullptr;
    return runtimes.constFirst();
}

QString DebugLogger::normalizedToken(const QString &value)
{
    return value.trimmed().toLower();
}

int DebugLogger::levelPriority(const QString &level)
{
    const QString normalized = normalizeLevelName(level);
    if (normalized == QStringLiteral("ERROR"))
        return 3;
    if (normalized == QStringLiteral("WARN"))
        return 2;
    if (normalized == QStringLiteral("RUNTIME"))
        return 1;
    if (normalized == QStringLiteral("NONE"))
        return 99;
    return 1;
}

QVariantMap DebugLogger::appendEntry(const QVariantMap &entry, bool forceAppend)
{
    if (m_paused || entry.isEmpty())
        return QVariantMap();
    if (!forceAppend && !m_enabled)
        return QVariantMap();

    QVariantMap normalized = entry;
    if (!normalized.contains(QStringLiteral("timestampEpochMs")))
        normalized.insert(QStringLiteral("timestampEpochMs"),
                          QDateTime::currentMSecsSinceEpoch());
    if (!normalized.contains(QStringLiteral("timestamp"))) {
        const qint64 ts = normalized.value(QStringLiteral("timestampEpochMs")).toLongLong();
        normalized.insert(QStringLiteral("timestamp"),
                          formatTimestampCentiseconds(ts));
    }
    const qint64 epochMs = normalized.value(QStringLiteral("timestampEpochMs")).toLongLong();
    normalized.insert(QStringLiteral("sequence"), QVariant::fromValue(++m_sequence));
    normalized.insert(QStringLiteral("sessionElapsedMs"),
                      QVariant::fromValue(epochMs - m_sessionStartEpochMs));
    normalized.insert(QStringLiteral("processId"),
                      QVariant::fromValue(static_cast<qlonglong>(QCoreApplication::applicationPid())));
    normalized.insert(QStringLiteral("threadId"),
                      QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId())));
    normalized.insert(QStringLiteral("applicationName"),
                      QCoreApplication::applicationName());
    normalized.insert(QStringLiteral("applicationVersion"),
                      QCoreApplication::applicationVersion());
    if (runtimeAttached() && !m_runtimeEvents.isNull())
        normalized.insert(QStringLiteral("runtimeEventSequence"),
                          QVariant::fromValue(m_runtimeEvents->eventSequence()));

    m_lastEntry = normalized;
    m_entries.append(normalized);
    if (m_entries.size() > m_maxEntries) {
        const int overflow = m_entries.size() - m_maxEntries;
        m_entries = m_entries.mid(overflow);
        m_droppedCount += overflow;
    }

    emit entriesChanged();
    emit entryAdded(normalized);
    return normalized;
}

QVariantMap DebugLogger::makeLogEntry(const QString &level,
                                      const QString &component,
                                      const QString &event,
                                      const QVariant &data) const
{
    const qint64 epochMs = QDateTime::currentMSecsSinceEpoch();
    const QString timestamp = formatTimestampCentiseconds(epochMs);
    const QString timestampIso = QDateTime::fromMSecsSinceEpoch(epochMs, QTimeZone::UTC)
        .toString(Qt::ISODateWithMs);
    const QString normalizedLevel = normalizeLevelName(level);
    const QString componentName = component.isEmpty() ? QStringLiteral("Unknown") : component;
    const QString eventName = event.isEmpty() ? QStringLiteral("event") : event;
    const QString message = QStringLiteral("[%1] [%2] %3.%4")
        .arg(timestamp, normalizedLevel, componentName, eventName);

    QVariantMap map;
    map.insert(QStringLiteral("source"), QStringLiteral("logger"));
    map.insert(QStringLiteral("level"), normalizedLevel);
    map.insert(QStringLiteral("component"), componentName);
    map.insert(QStringLiteral("event"), eventName);
    map.insert(QStringLiteral("timestampEpochMs"), epochMs);
    map.insert(QStringLiteral("timestamp"), timestamp);
    map.insert(QStringLiteral("timestampIso"), timestampIso);
    map.insert(QStringLiteral("message"), message);
    if (data.isValid())
        map.insert(QStringLiteral("data"), data);
    return map;
}

QVariantMap DebugLogger::makeRuntimeEntry(const QVariantMap &eventData) const
{
    const qint64 epochMs = eventData.value(QStringLiteral("timestampEpochMs"),
                                           QDateTime::currentMSecsSinceEpoch()).toLongLong();
    const QString timestamp = formatTimestampCentiseconds(epochMs);
    const QString timestampIso = QDateTime::fromMSecsSinceEpoch(epochMs, QTimeZone::UTC)
        .toString(Qt::ISODateWithMs);
    const QString eventType = eventData.value(QStringLiteral("type")).toString();
    const QString normalizedType = eventType.isEmpty()
        ? QStringLiteral("runtime-event")
        : eventType;
    const qlonglong sequence = eventData.value(QStringLiteral("sequence"), -1).toLongLong();
    QString message = QStringLiteral("[%1] [RUNTIME] RuntimeEvents.%2")
        .arg(timestamp, normalizedType);
    if (sequence >= 0)
        message += QStringLiteral(" #") + QString::number(sequence);

    QVariantMap map;
    map.insert(QStringLiteral("source"), QStringLiteral("runtime"));
    map.insert(QStringLiteral("level"), QStringLiteral("RUNTIME"));
    map.insert(QStringLiteral("component"), QStringLiteral("RuntimeEvents"));
    map.insert(QStringLiteral("event"), normalizedType);
    map.insert(QStringLiteral("sequence"), sequence);
    map.insert(QStringLiteral("timestampEpochMs"), epochMs);
    map.insert(QStringLiteral("timestamp"), timestamp);
    map.insert(QStringLiteral("timestampIso"), timestampIso);
    map.insert(QStringLiteral("uptimeMs"), eventData.value(QStringLiteral("uptimeMs"), 0));
    map.insert(QStringLiteral("rawEvent"), eventData);
    map.insert(QStringLiteral("data"), eventData.value(QStringLiteral("payload")));
    map.insert(QStringLiteral("message"), message);
    return map;
}

QVariantList DebugLogger::boundedEntries(const QVariantList &source, int limit) const
{
    if (limit <= 0 || source.size() <= limit)
        return source;
    return source.mid(source.size() - limit);
}

bool DebugLogger::matchesFilters(const QVariantMap &entry) const
{
    if (!m_levelFilter.isEmpty()) {
        const QString level = normalizedToken(entry.value(QStringLiteral("level")).toString());
        if (!m_levelFilter.contains(level))
            return false;
    }

    if (!m_componentFilter.isEmpty()) {
        const QString component = normalizedToken(entry.value(QStringLiteral("component")).toString());
        bool matched = false;
        for (const QString &token : m_componentFilter) {
            if (component.contains(token)) {
                matched = true;
                break;
            }
        }
        if (!matched)
            return false;
    }

    if (!m_textFilter.isEmpty()) {
        const QString message = entry.value(QStringLiteral("message")).toString();
        const QString component = entry.value(QStringLiteral("component")).toString();
        const QString eventName = entry.value(QStringLiteral("event")).toString();
        const QString dataText = QString::fromUtf8(
            QJsonDocument::fromVariant(entry.value(QStringLiteral("data"))).toJson(QJsonDocument::Compact));
        const QString haystack = normalizedToken(message + QLatin1Char(' ')
                                                 + component + QLatin1Char(' ')
                                                 + eventName + QLatin1Char(' ')
                                                 + dataText);
        if (!haystack.contains(m_textFilter))
            return false;
    }

    return true;
}

bool DebugLogger::shouldOutputLevel(const QString &level) const
{
    if (!m_enabled)
        return false;
    const int currentPriority = levelPriority(level);
    const int minimumPriority = levelPriority(m_stdoutMinimumLevel);
    return currentPriority >= minimumPriority;
}

bool DebugLogger::isStdoutNoiseEntry(const QVariantMap &entry) const
{
    if (!m_stdoutNoiseReductionEnabled)
        return false;

    const QString level = normalizeLevelName(entry.value(QStringLiteral("level")).toString());
    const QString eventType = normalizedToken(entry.value(QStringLiteral("event")).toString());
    const QString component = normalizedToken(entry.value(QStringLiteral("component")).toString());

    if (level == QStringLiteral("LOG")) {
        if (eventType == QStringLiteral("created")
            || eventType == QStringLiteral("shown")
            || eventType == QStringLiteral("hidden")
            || eventType == QStringLiteral("destroyed")) {
            return true;
        }

        if (component == QStringLiteral("rendermonitor")
            && eventType == QStringLiteral("render-stats")) {
            return true;
        }
    }

    if (level == QStringLiteral("RUNTIME")) {
        static const QStringList noisyRuntimeEvents = {
            QStringLiteral("ui-event"),
            QStringLiteral("mouse-move"),
            QStringLiteral("hover-move"),
            QStringLiteral("mouse-wheel"),
            QStringLiteral("mouse-press"),
            QStringLiteral("mouse-release"),
            QStringLiteral("mouse-double-click"),
            QStringLiteral("key-press"),
            QStringLiteral("key-release"),
            QStringLiteral("touch-event"),
            QStringLiteral("tablet-event"),
            QStringLiteral("tablet-proximity"),
            QStringLiteral("native-gesture")
        };
        if (containsToken(noisyRuntimeEvents, eventType))
            return true;
    }

    return false;
}

bool DebugLogger::shouldEchoRuntimeEntry(const QVariantMap &entry, qint64 nowMs)
{
    const QString eventType = normalizedToken(entry.value(QStringLiteral("event")).toString());
    if (m_runtimeEchoExcludeTypes.contains(eventType))
        return false;

    if (m_runtimeEchoMinIntervalMs <= 0)
        return true;

    if (m_lastRuntimeEchoEpochMs >= 0 && (nowMs - m_lastRuntimeEchoEpochMs) < m_runtimeEchoMinIntervalMs) {
        m_suppressedRuntimeEchoCount += 1;
        return false;
    }

    m_lastRuntimeEchoEpochMs = nowMs;
    return true;
}

void DebugLogger::printSuppressedRuntimeEchoSummary(qint64 nowMs)
{
    if (m_suppressedRuntimeEchoCount <= 0)
        return;

    QVariantMap summaryEntry;
    summaryEntry.insert(QStringLiteral("source"), QStringLiteral("runtime"));
    summaryEntry.insert(QStringLiteral("level"), QStringLiteral("RUNTIME"));
    summaryEntry.insert(QStringLiteral("component"), QStringLiteral("RuntimeEvents"));
    summaryEntry.insert(QStringLiteral("event"), QStringLiteral("echo-suppressed"));
    summaryEntry.insert(QStringLiteral("timestampEpochMs"), nowMs);
    summaryEntry.insert(QStringLiteral("timestamp"), formatTimestampCentiseconds(nowMs));
    summaryEntry.insert(QStringLiteral("message"),
                        QStringLiteral("[runtime-echo] suppressed %1 events")
                        .arg(m_suppressedRuntimeEchoCount));
    summaryEntry.insert(QStringLiteral("data"),
                        QVariantMap{{QStringLiteral("suppressedCount"), m_suppressedRuntimeEchoCount}});

    m_suppressedRuntimeEchoCount = 0;
    printEntryToStdout(summaryEntry);
}

QString DebugLogger::formatStdoutMessage(const QVariantMap &entry) const
{
    const QString timestamp = entry.value(QStringLiteral("timestamp")).toString();
    const QString level = entry.value(QStringLiteral("level")).toString();
    const QString component = entry.value(QStringLiteral("component")).toString();
    const QString eventName = entry.value(QStringLiteral("event")).toString();
    const QString message = entry.value(QStringLiteral("message")).toString();
    const qlonglong sequence = entry.value(QStringLiteral("sequence"), -1).toLongLong();

    if (!m_verboseOutput)
        return message;

    QString line = QStringLiteral("[%1] [%2] #%3 %4.%5")
        .arg(timestamp, level)
        .arg(sequence)
        .arg(component, eventName);

    line += QStringLiteral(" pid=%1 tid=%2")
        .arg(entry.value(QStringLiteral("processId")).toString(),
             entry.value(QStringLiteral("threadId")).toString());

    if (entry.contains(QStringLiteral("runtimeEventSequence")))
        line += QStringLiteral(" runtimeSeq=%1")
            .arg(entry.value(QStringLiteral("runtimeEventSequence")).toString());

    const QVariant data = entry.value(QStringLiteral("data"));
    if (data.isValid())
        line += QStringLiteral(" data=%1").arg(variantToCompactJson(data));

    return line;
}

QString DebugLogger::variantToCompactJson(const QVariant &value) const
{
    if (!value.isValid())
        return QStringLiteral("null");
    const QByteArray json = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
    if (!json.isEmpty())
        return QString::fromUtf8(json);
    return value.toString();
}

void DebugLogger::printEntryToStdout(const QVariantMap &entry)
{
    if (!shouldOutputLevel(entry.value(QStringLiteral("level")).toString()))
        return;
    if (isStdoutNoiseEntry(entry))
        return;

    qInfo().noquote() << formatStdoutMessage(entry);
    if (m_jsonOutput)
        qInfo().noquote() << QStringLiteral("[DEBUG-ENTRY]") << variantToCompactJson(entry);
}

void DebugLogger::output(const QString &level, const QString &component, const QString &event, const QVariant &data)
{
    if (!m_enabled)
        return;
    const QVariantMap entry = makeLogEntry(level, component, event, data);
    const QVariantMap appended = appendEntry(entry);
    if (appended.isEmpty())
        return;
    printEntryToStdout(appended);
}
