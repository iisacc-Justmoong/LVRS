#include "backend/navigation/pagemonitor.h"

PageMonitor::PageMonitor(QObject *parent)
    : QObject(parent)
{
}

void PageMonitor::record(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty())
        return;

    const bool changed = m_history.isEmpty() || m_history.last() != trimmed;
    if (changed) {
        m_history.append(trimmed);
    }
    emitIfChanged(changed);
}

QString PageMonitor::undo()
{
    if (m_history.size() <= 1)
        return current();

    m_history.removeLast();
    emit historyChanged();
    return current();
}

void PageMonitor::clear()
{
    if (m_history.isEmpty())
        return;
    m_history.clear();
    emit historyChanged();
}

QStringList PageMonitor::history() const
{
    return m_history;
}

int PageMonitor::count() const
{
    return m_history.size();
}

QString PageMonitor::current() const
{
    return m_history.isEmpty() ? QString() : m_history.last();
}

bool PageMonitor::canUndo() const
{
    return m_history.size() > 1;
}

void PageMonitor::emitIfChanged(bool changed)
{
    if (changed)
        emit historyChanged();
}
