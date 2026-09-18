#include "backend/navigation/viewstatetracker.h"

ViewStateTracker::ViewStateTracker(QObject *parent)
    : QObject(parent)
{
}

QVariantList ViewStateTracker::stack() const
{
    QVariantList list;
    list.reserve(m_records.size());
    for (const ViewRecord &record : m_records)
        list.append(toMap(record));
    return list;
}

QStringList ViewStateTracker::loadedViews() const
{
    QStringList list;
    list.reserve(m_records.size());
    for (const ViewRecord &record : m_records)
        list.append(record.viewId);
    return list;
}

QStringList ViewStateTracker::activeViews() const
{
    QStringList list;
    for (const ViewRecord &record : m_records) {
        if (record.state == Active)
            list.append(record.viewId);
    }
    return list;
}

QStringList ViewStateTracker::inactiveViews() const
{
    QStringList list;
    for (const ViewRecord &record : m_records) {
        if (record.state == Inactive)
            list.append(record.viewId);
    }
    return list;
}

QStringList ViewStateTracker::disabledViews() const
{
    QStringList list;
    for (const ViewRecord &record : m_records) {
        if (record.state == Disabled)
            list.append(record.viewId);
    }
    return list;
}

QString ViewStateTracker::currentActiveView() const
{
    for (auto it = m_records.crbegin(); it != m_records.crend(); ++it) {
        if (it->state == Active)
            return it->viewId;
    }
    return QString();
}

int ViewStateTracker::loadedCount() const
{
    return static_cast<int>(m_records.size());
}

void ViewStateTracker::syncStack(const QVariantList &entries)
{
    const QVector<StackInput> inputs = parseEntries(entries);
    QVector<ViewRecord> nextRecords;
    nextRecords.reserve(inputs.size());

    for (qsizetype i = 0; i < inputs.size(); ++i) {
        const StackInput &input = inputs[i];
        ViewRecord record;
        record.viewId = input.viewId;
        record.path = input.path;
        record.index = static_cast<int>(i);
        record.baseEnabled = input.enabled;
        nextRecords.append(record);
    }

    recalculateStates(&nextRecords);
    if (updateRecords(nextRecords))
        emit stackChanged();
}

void ViewStateTracker::setViewDisabled(const QString &viewId, bool disabled)
{
    const QString normalized = normalizeId(viewId);
    if (normalized.isEmpty())
        return;

    const qsizetype previousSize = m_disabledOverrides.size();
    if (disabled)
        m_disabledOverrides.insert(normalized);
    else
        m_disabledOverrides.remove(normalized);

    if (previousSize == m_disabledOverrides.size())
        return;

    QVector<ViewRecord> nextRecords = m_records;
    recalculateStates(&nextRecords);
    if (updateRecords(nextRecords))
        emit stackChanged();
}

void ViewStateTracker::setViewEnabled(const QString &viewId, bool enabled)
{
    setViewDisabled(viewId, !enabled);
}

bool ViewStateTracker::isLoaded(const QString &viewId) const
{
    return findTopMostIndex(viewId) >= 0;
}

QString ViewStateTracker::stateOf(const QString &viewId) const
{
    const int index = findTopMostIndex(viewId);
    if (index < 0)
        return QString();
    return stateToString(m_records.at(index).state);
}

QVariantMap ViewStateTracker::view(const QString &viewId) const
{
    const int index = findTopMostIndex(viewId);
    if (index < 0)
        return QVariantMap();
    return toMap(m_records.at(index));
}

QVariantMap ViewStateTracker::snapshot() const
{
    QVariantMap data;
    data.insert(QStringLiteral("currentActiveView"), currentActiveView());
    data.insert(QStringLiteral("loadedViews"), loadedViews());
    data.insert(QStringLiteral("activeViews"), activeViews());
    data.insert(QStringLiteral("inactiveViews"), inactiveViews());
    data.insert(QStringLiteral("disabledViews"), disabledViews());
    data.insert(QStringLiteral("stack"), stack());
    return data;
}

void ViewStateTracker::clear()
{
    if (m_records.isEmpty() && m_disabledOverrides.isEmpty())
        return;

    m_records.clear();
    m_disabledOverrides.clear();
    emit stackChanged();
}

QString ViewStateTracker::normalizeId(const QString &rawId)
{
    return rawId.trimmed();
}

QString ViewStateTracker::stateToString(ViewStateTracker::ViewState state)
{
    switch (state) {
    case Active:
        return QStringLiteral("Active");
    case Inactive:
        return QStringLiteral("Inactive");
    case Disabled:
        return QStringLiteral("Disabled");
    }
    return QStringLiteral("Disabled");
}

bool ViewStateTracker::sameStack(const QVector<ViewStateTracker::ViewRecord> &left,
                                 const QVector<ViewStateTracker::ViewRecord> &right)
{
    if (left.size() != right.size())
        return false;

    for (qsizetype i = 0; i < left.size(); ++i) {
        const ViewRecord &lhs = left.at(i);
        const ViewRecord &rhs = right.at(i);
        if (lhs.viewId != rhs.viewId)
            return false;
        if (lhs.path != rhs.path)
            return false;
        if (lhs.index != rhs.index)
            return false;
        if (lhs.baseEnabled != rhs.baseEnabled)
            return false;
        if (lhs.state != rhs.state)
            return false;
    }
    return true;
}

QVector<ViewStateTracker::StackInput> ViewStateTracker::parseEntries(const QVariantList &entries) const
{
    QVector<StackInput> inputs;
    inputs.reserve(entries.size());

    int anonymousIndex = 0;
    for (const QVariant &entry : entries) {
        const QVariantMap map = entry.toMap();

        QString viewId = normalizeId(map.value(QStringLiteral("viewId")).toString());
        const QString path = map.value(QStringLiteral("path")).toString().trimmed();
        if (viewId.isEmpty() && !path.isEmpty())
            viewId = normalizeId(path);
        if (viewId.isEmpty())
            viewId = QStringLiteral("_component_%1").arg(anonymousIndex++);

        bool enabled = true;
        if (map.contains(QStringLiteral("enabled")))
            enabled = map.value(QStringLiteral("enabled")).toBool();
        if (map.value(QStringLiteral("disabled")).toBool())
            enabled = false;

        StackInput input;
        input.viewId = viewId;
        input.path = path;
        input.enabled = enabled;
        inputs.append(input);
    }

    return inputs;
}

void ViewStateTracker::recalculateStates(QVector<ViewStateTracker::ViewRecord> *records) const
{
    if (!records)
        return;

    int activeIndex = -1;
    for (int i = records->size() - 1; i >= 0; --i) {
        const ViewRecord &record = records->at(i);
        const bool effectiveEnabled = record.baseEnabled
            && !m_disabledOverrides.contains(record.viewId);
        if (effectiveEnabled) {
            activeIndex = i;
            break;
        }
    }

    for (int i = 0; i < records->size(); ++i) {
        ViewRecord &record = (*records)[i];
        const bool effectiveEnabled = record.baseEnabled
            && !m_disabledOverrides.contains(record.viewId);
        if (!effectiveEnabled)
            record.state = Disabled;
        else if (i == activeIndex)
            record.state = Active;
        else
            record.state = Inactive;
    }
}

bool ViewStateTracker::updateRecords(const QVector<ViewStateTracker::ViewRecord> &nextRecords)
{
    if (sameStack(m_records, nextRecords))
        return false;
    m_records = nextRecords;
    return true;
}

QVariantMap ViewStateTracker::toMap(const ViewStateTracker::ViewRecord &record) const
{
    QVariantMap map;
    map.insert(QStringLiteral("viewId"), record.viewId);
    map.insert(QStringLiteral("path"), record.path);
    map.insert(QStringLiteral("state"), stateToString(record.state));
    map.insert(QStringLiteral("stateCode"), static_cast<int>(record.state));
    map.insert(QStringLiteral("index"), record.index);
    map.insert(QStringLiteral("loaded"), true);
    map.insert(QStringLiteral("enabled"), record.state != Disabled);
    map.insert(QStringLiteral("active"), record.state == Active);
    return map;
}

int ViewStateTracker::findTopMostIndex(const QString &viewId) const
{
    const QString normalized = normalizeId(viewId);
    if (normalized.isEmpty())
        return -1;

    for (int i = m_records.size() - 1; i >= 0; --i) {
        if (m_records.at(i).viewId == normalized)
            return i;
    }
    return -1;
}
