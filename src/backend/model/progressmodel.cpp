#include "backend/model/progressmodel.h"

#include "backend/state/statemodel.h"

#include <QMetaObject>
#include <QtMath>

#include <algorithm>
#include <cmath>

ProgressModel::ProgressModel(QObject *parent)
    : QObject(parent)
{
}

double ProgressModel::minimumValue() const
{
    return m_minimumValue;
}

void ProgressModel::setMinimumValue(double value)
{
    if (qFuzzyCompare(m_minimumValue, value))
        return;
    m_minimumValue = value;
    emit valuesChanged();
    emitEffectiveChanged();
}

double ProgressModel::maximumValue() const
{
    return m_maximumValue;
}

void ProgressModel::setMaximumValue(double value)
{
    if (qFuzzyCompare(m_maximumValue, value))
        return;
    m_maximumValue = value;
    emit valuesChanged();
    emitEffectiveChanged();
}

double ProgressModel::startValue() const
{
    return m_startValue;
}

void ProgressModel::setStartValue(double value)
{
    if (qFuzzyCompare(m_startValue, value))
        return;
    m_startValue = value;
    emit valuesChanged();
    emitEffectiveChanged();
}

double ProgressModel::currentValue() const
{
    return m_currentValue;
}

void ProgressModel::setCurrentValue(double value)
{
    if (qFuzzyCompare(m_currentValue, value))
        return;
    m_currentValue = value;
    emit valuesChanged();
    emitEffectiveChanged();
}

QObject *ProgressModel::stateModel() const
{
    return m_stateModel;
}

void ProgressModel::setStateModel(QObject *model)
{
    if (m_stateModel == model)
        return;
    if (m_stateRevisionConnection)
        disconnect(m_stateRevisionConnection);
    m_stateModel = model;
    reconnectStateModel();
    emit stateModelChanged();
    handleStateModelChanged();
}

QString ProgressModel::minimumValueStateKey() const
{
    return m_minimumValueStateKey;
}

void ProgressModel::setMinimumValueStateKey(const QString &value)
{
    const QString next = normalizedKey(value, QStringLiteral("minimumValue"));
    if (m_minimumValueStateKey == next)
        return;
    m_minimumValueStateKey = next;
    emit stateKeysChanged();
    emitEffectiveChanged();
}

QString ProgressModel::maximumValueStateKey() const
{
    return m_maximumValueStateKey;
}

void ProgressModel::setMaximumValueStateKey(const QString &value)
{
    const QString next = normalizedKey(value, QStringLiteral("maximumValue"));
    if (m_maximumValueStateKey == next)
        return;
    m_maximumValueStateKey = next;
    emit stateKeysChanged();
    emitEffectiveChanged();
}

QString ProgressModel::startValueStateKey() const
{
    return m_startValueStateKey;
}

void ProgressModel::setStartValueStateKey(const QString &value)
{
    const QString next = normalizedKey(value, QStringLiteral("startValue"));
    if (m_startValueStateKey == next)
        return;
    m_startValueStateKey = next;
    emit stateKeysChanged();
    emitEffectiveChanged();
}

QString ProgressModel::currentValueStateKey() const
{
    return m_currentValueStateKey;
}

void ProgressModel::setCurrentValueStateKey(const QString &value)
{
    const QString next = normalizedKey(value, QStringLiteral("currentValue"));
    if (m_currentValueStateKey == next)
        return;
    m_currentValueStateKey = next;
    emit stateKeysChanged();
    emitEffectiveChanged();
}

bool ProgressModel::usingStateModel() const
{
    return m_stateModel != nullptr;
}

int ProgressModel::stateRevision() const
{
    return m_stateRevision;
}

double ProgressModel::effectiveMinimumValue() const
{
    return stateNumber(m_minimumValueStateKey, m_minimumValue);
}

double ProgressModel::effectiveMaximumValue() const
{
    return stateNumber(m_maximumValueStateKey, m_maximumValue);
}

double ProgressModel::effectiveStartValue() const
{
    return stateNumber(m_startValueStateKey, m_startValue);
}

double ProgressModel::effectiveCurrentValue() const
{
    return stateNumber(m_currentValueStateKey, m_currentValue);
}

double ProgressModel::valueRange() const
{
    return effectiveMaximumValue() - effectiveMinimumValue();
}

double ProgressModel::normalizedStart() const
{
    return normalizedValue(effectiveStartValue());
}

double ProgressModel::normalizedCurrent() const
{
    return normalizedValue(effectiveCurrentValue());
}

double ProgressModel::fillStart() const
{
    return std::min(normalizedStart(), normalizedCurrent());
}

double ProgressModel::fillProgress() const
{
    return std::abs(normalizedCurrent() - normalizedStart());
}

double ProgressModel::progress() const
{
    return normalizedCurrent();
}

double ProgressModel::stateNumber(const QString &key, double fallbackValue) const
{
    const double fallback = std::isfinite(fallbackValue) ? fallbackValue : 0.0;
    if (!m_stateModel)
        return fallback;

    if (auto *state = qobject_cast<StateModel *>(m_stateModel.data())) {
        bool ok = false;
        const double value = state->valueOr(key, fallback).toDouble(&ok);
        return ok && std::isfinite(value) ? value : fallback;
    }

    QVariant returned;
    if (QMetaObject::invokeMethod(m_stateModel,
                                  "valueOr",
                                  Q_RETURN_ARG(QVariant, returned),
                                  Q_ARG(QString, key),
                                  Q_ARG(QVariant, fallback))) {
        bool ok = false;
        const double value = returned.toDouble(&ok);
        return ok && std::isfinite(value) ? value : fallback;
    }

    return fallback;
}

double ProgressModel::normalizedValue(double value) const
{
    const double range = valueRange();
    if (std::abs(range) < 0.000001)
        return value >= effectiveMaximumValue() ? 1.0 : 0.0;

    const double normalized = (value - effectiveMinimumValue()) / range;
    if (!std::isfinite(normalized))
        return 0.0;
    return std::clamp(normalized, 0.0, 1.0);
}

double ProgressModel::radiusFor(int shapeStyle, double cornerRadius, double rectWidth, double rectHeight) const
{
    if (shapeStyle == 1)
        return qMax(0.0, qMin(rectWidth, rectHeight) / 2.0);
    return cornerRadius;
}

void ProgressModel::handleStateModelChanged()
{
    ++m_stateRevision;
    emit revisionChanged();
    emitEffectiveChanged();
}

QString ProgressModel::normalizedKey(const QString &value, const QString &fallback)
{
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

void ProgressModel::reconnectStateModel()
{
    if (!m_stateModel)
        return;
    m_stateRevisionConnection = connect(m_stateModel,
                                        SIGNAL(revisionChanged()),
                                        this,
                                        SLOT(handleStateModelChanged()));
}

void ProgressModel::emitEffectiveChanged()
{
    emit effectiveValuesChanged();
}
