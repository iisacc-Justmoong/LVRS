#include "backend/runtime/qmlcontextbinder.h"

#include "backend/state/viewmodel.h"
#include "backend/state/viewmodelregistry.h"

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSet>

namespace {

QString normalizeToken(const QString &value)
{
    return value.trimmed();
}

void appendError(lvrs::QmlContextBindResult *result, const QString &message)
{
    if (!result)
        return;
    result->ok = false;
    result->errors.append(message);
}

QString resolveViewModelKey(QObject *object, const QString &fallbackKey)
{
    const QString normalizedFallback = normalizeToken(fallbackKey);
    if (!normalizedFallback.isEmpty())
        return normalizedFallback;

    auto *viewModel = qobject_cast<ViewModel *>(object);
    return viewModel ? normalizeToken(viewModel->key()) : QString();
}

} // namespace

namespace lvrs {

QString QmlContextBindResult::errorMessage() const
{
    return errors.join(QStringLiteral("; "));
}

QmlContextBindResult applyQmlContextBindPlan(QQmlApplicationEngine &engine,
                                             const QmlContextBindPlan &plan)
{
    QmlContextBindResult result;
    QQmlContext *rootContext = engine.rootContext();
    if (!rootContext) {
        appendError(&result, QStringLiteral("QQmlApplicationEngine has no root context."));
        return result;
    }

    QSet<QString> contextNames;
    for (const QmlContextObjectBinding &binding : plan.contextObjects) {
        const QString contextName = normalizeToken(binding.contextName);
        if (contextName.isEmpty()) {
            if (binding.required)
                appendError(&result, QStringLiteral("Context object has an empty context name."));
            continue;
        }
        if (!binding.object) {
            if (binding.required)
                appendError(&result, QStringLiteral("Context object '%1' is null.").arg(contextName));
            continue;
        }
        if (contextNames.contains(contextName)) {
            appendError(&result, QStringLiteral("Context name '%1' is declared more than once.").arg(contextName));
            continue;
        }

        rootContext->setContextProperty(contextName, binding.object);
        contextNames.insert(contextName);
        result.contextNames.append(contextName);
    }

    if (!plan.viewModels.isEmpty()) {
        auto *registry = engine.singletonInstance<ViewModelRegistry *>(QStringLiteral("LVRS"),
                                                                       QStringLiteral("ViewModels"));
        if (!registry) {
            appendError(&result, QStringLiteral("LVRS ViewModels singleton is unavailable."));
            return result;
        }

        for (const QmlViewModelBinding &binding : plan.viewModels) {
            const QString key = resolveViewModelKey(binding.object, binding.key);
            if (key.isEmpty()) {
                if (binding.required)
                    appendError(&result, QStringLiteral("ViewModel binding has an empty key."));
                continue;
            }
            if (!binding.object) {
                if (binding.required)
                    appendError(&result, QStringLiteral("ViewModel '%1' is null.").arg(key));
                continue;
            }

            if (auto *viewModel = qobject_cast<ViewModel *>(binding.object)) {
                if (viewModel->key() != key)
                    viewModel->setKey(key);
                if (!binding.displayName.trimmed().isEmpty())
                    viewModel->setDisplayName(binding.displayName);
                if (!binding.metadata.isEmpty())
                    viewModel->setMetadata(binding.metadata);
            }

            if (!registry->registerViewModel(binding.object, key)) {
                appendError(&result,
                            QStringLiteral("ViewModel '%1' registration failed: %2")
                                .arg(key, registry->lastError()));
                continue;
            }

            const QString contextName = normalizeToken(binding.contextName);
            if (!contextName.isEmpty()) {
                if (contextNames.contains(contextName)) {
                    appendError(&result,
                                QStringLiteral("Context name '%1' is declared more than once.").arg(contextName));
                    continue;
                }
                rootContext->setContextProperty(contextName, binding.object);
                contextNames.insert(contextName);
                result.contextNames.append(contextName);
            }

            const QString viewId = normalizeToken(binding.viewId);
            if (!viewId.isEmpty() && !registry->bindView(viewId, key, binding.writable)) {
                appendError(&result,
                            QStringLiteral("ViewModel '%1' view binding failed: %2")
                                .arg(key, registry->lastError()));
                continue;
            }

            result.viewModelKeys.append(key);
        }
    }

    return result;
}

} // namespace lvrs
