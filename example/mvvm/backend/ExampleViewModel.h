#pragma once

#include "backend/state/viewmodel.h"

class ExampleModel;

class ExampleViewModel : public ViewModel
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit ExampleViewModel(ExampleModel *model, QObject *parent = nullptr);

    QString status() const;

    Q_INVOKABLE void simulateWork();

signals:
    void statusChanged();

private:
    ExampleModel *m_model = nullptr;
};
