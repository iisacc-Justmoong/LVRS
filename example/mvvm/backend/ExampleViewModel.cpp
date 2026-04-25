#include "ExampleViewModel.h"

#include "ExampleModel.h"

ExampleViewModel::ExampleViewModel(ExampleModel *model, QObject *parent)
    : ViewModel(parent)
    , m_model(model)
{
    setKey(QStringLiteral("Example"));
    setDisplayName(QStringLiteral("Example"));
    setMetadata({
        {QStringLiteral("domain"), QStringLiteral("example")},
        {QStringLiteral("role"), QStringLiteral("demo")}
    });

    if (m_model) {
        connect(m_model, &ExampleModel::statusChanged, this, &ExampleViewModel::statusChanged);
    }
}

QString ExampleViewModel::status() const
{
    return m_model ? m_model->status() : QString();
}

void ExampleViewModel::simulateWork()
{
    if (!m_model)
        return;
    if (m_model->status() == QStringLiteral("Idle"))
        m_model->setStatus(QStringLiteral("Working"));
    else
        m_model->setStatus(QStringLiteral("Idle"));
}
