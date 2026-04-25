#include "ExampleBootstrap.h"

#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QtQml>

#include "ExampleModel.h"
#include "ExampleViewModel.h"
#include "backend/runtime/qmlcontextbinder.h"

void setupExampleViewModel(QQmlEngine *engine)
{
    if (!engine)
        return;

    auto *applicationEngine = qobject_cast<QQmlApplicationEngine *>(engine);
    if (!applicationEngine)
        return;

    auto *model = new ExampleModel(engine);
    auto *viewModel = new ExampleViewModel(model, engine);

    lvrs::QmlContextBindPlan plan;

    lvrs::QmlViewModelBinding viewModelBinding;
    viewModelBinding.key = QStringLiteral("Example");
    viewModelBinding.object = viewModel;
    viewModelBinding.contextName = QStringLiteral("exampleViewModel");
    viewModelBinding.displayName = QStringLiteral("Example");
    viewModelBinding.metadata = {
        {QStringLiteral("domain"), QStringLiteral("example")},
        {QStringLiteral("role"), QStringLiteral("demo")}
    };
    viewModelBinding.viewId = QStringLiteral("ExampleView");
    viewModelBinding.writable = true;
    plan.viewModels.append(viewModelBinding);

    const lvrs::QmlContextBindResult result =
        lvrs::applyQmlContextBindPlan(*applicationEngine, plan);
    if (!result.ok)
        qWarning().noquote() << "Example MVVM bootstrap failed:" << result.errorMessage();
}
