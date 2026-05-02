#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QtQml/qqml.h>

class StateModel;

class ProgressModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ProgressModel)

    Q_PROPERTY(double minimumValue READ minimumValue WRITE setMinimumValue NOTIFY valuesChanged)
    Q_PROPERTY(double maximumValue READ maximumValue WRITE setMaximumValue NOTIFY valuesChanged)
    Q_PROPERTY(double startValue READ startValue WRITE setStartValue NOTIFY valuesChanged)
    Q_PROPERTY(double currentValue READ currentValue WRITE setCurrentValue NOTIFY valuesChanged)
    Q_PROPERTY(QObject *stateModel READ stateModel WRITE setStateModel NOTIFY stateModelChanged)
    Q_PROPERTY(QString minimumValueStateKey READ minimumValueStateKey WRITE setMinimumValueStateKey NOTIFY stateKeysChanged)
    Q_PROPERTY(QString maximumValueStateKey READ maximumValueStateKey WRITE setMaximumValueStateKey NOTIFY stateKeysChanged)
    Q_PROPERTY(QString startValueStateKey READ startValueStateKey WRITE setStartValueStateKey NOTIFY stateKeysChanged)
    Q_PROPERTY(QString currentValueStateKey READ currentValueStateKey WRITE setCurrentValueStateKey NOTIFY stateKeysChanged)
    Q_PROPERTY(bool usingStateModel READ usingStateModel NOTIFY stateModelChanged)
    Q_PROPERTY(int stateRevision READ stateRevision NOTIFY revisionChanged)
    Q_PROPERTY(double effectiveMinimumValue READ effectiveMinimumValue NOTIFY effectiveValuesChanged)
    Q_PROPERTY(double effectiveMaximumValue READ effectiveMaximumValue NOTIFY effectiveValuesChanged)
    Q_PROPERTY(double effectiveStartValue READ effectiveStartValue NOTIFY effectiveValuesChanged)
    Q_PROPERTY(double effectiveCurrentValue READ effectiveCurrentValue NOTIFY effectiveValuesChanged)
    Q_PROPERTY(double valueRange READ valueRange NOTIFY effectiveValuesChanged)
    Q_PROPERTY(double normalizedStart READ normalizedStart NOTIFY effectiveValuesChanged)
    Q_PROPERTY(double normalizedCurrent READ normalizedCurrent NOTIFY effectiveValuesChanged)
    Q_PROPERTY(double fillStart READ fillStart NOTIFY effectiveValuesChanged)
    Q_PROPERTY(double fillProgress READ fillProgress NOTIFY effectiveValuesChanged)
    Q_PROPERTY(double progress READ progress NOTIFY effectiveValuesChanged)

public:
    explicit ProgressModel(QObject *parent = nullptr);

    double minimumValue() const;
    void setMinimumValue(double value);

    double maximumValue() const;
    void setMaximumValue(double value);

    double startValue() const;
    void setStartValue(double value);

    double currentValue() const;
    void setCurrentValue(double value);

    QObject *stateModel() const;
    void setStateModel(QObject *model);

    QString minimumValueStateKey() const;
    void setMinimumValueStateKey(const QString &value);

    QString maximumValueStateKey() const;
    void setMaximumValueStateKey(const QString &value);

    QString startValueStateKey() const;
    void setStartValueStateKey(const QString &value);

    QString currentValueStateKey() const;
    void setCurrentValueStateKey(const QString &value);

    bool usingStateModel() const;
    int stateRevision() const;

    double effectiveMinimumValue() const;
    double effectiveMaximumValue() const;
    double effectiveStartValue() const;
    double effectiveCurrentValue() const;
    double valueRange() const;
    double normalizedStart() const;
    double normalizedCurrent() const;
    double fillStart() const;
    double fillProgress() const;
    double progress() const;

    Q_INVOKABLE double stateNumber(const QString &key, double fallbackValue) const;
    Q_INVOKABLE double normalizedValue(double value) const;
    Q_INVOKABLE double radiusFor(int shapeStyle, double cornerRadius, double rectWidth, double rectHeight) const;

signals:
    void valuesChanged();
    void stateModelChanged();
    void stateKeysChanged();
    void effectiveValuesChanged();
    void revisionChanged();

private slots:
    void handleStateModelChanged();

private:
    static QString normalizedKey(const QString &value, const QString &fallback);
    void reconnectStateModel();
    void emitEffectiveChanged();

    double m_minimumValue = 0.0;
    double m_maximumValue = 100.0;
    double m_startValue = 0.0;
    double m_currentValue = 0.0;
    QPointer<QObject> m_stateModel;
    QMetaObject::Connection m_stateRevisionConnection;
    QString m_minimumValueStateKey = QStringLiteral("minimumValue");
    QString m_maximumValueStateKey = QStringLiteral("maximumValue");
    QString m_startValueStateKey = QStringLiteral("startValue");
    QString m_currentValueStateKey = QStringLiteral("currentValue");
    int m_stateRevision = 0;
};
