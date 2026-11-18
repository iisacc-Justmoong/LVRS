#include "QtEventWait.h"
#include <QGuiApplication>

#include "backend/runtime/permissionsequencer.h"

class PermissionSequencerTests : public QObject
{
public:
    void sequencer_runs_steps_in_priority_order_and_records_history();
    void sequencer_stops_on_required_failure_and_can_continue_when_configured();
};

void PermissionSequencerTests::sequencer_runs_steps_in_priority_order_and_records_history()
{
    lvrs::PermissionRequestSequencer sequencer;
    int historyCount = 0;
    QObject observer;
    const auto historyConnection = QObject::connect(&sequencer,
        &lvrs::PermissionRequestSequencer::historyChanged, &observer, [&] { ++historyCount; });
    CTEST_CHECK(historyConnection);

    QStringList calls;
    QList<int> previousResultCounts;

    auto makeStep = [&](const QString &name,
                        int priority,
                        bool required,
                        lvrs::PermissionRequestStatus status) {
        lvrs::PermissionRequestStep step;
        step.name = name;
        step.priority = priority;
        step.required = required;
        step.metadata = {{QStringLiteral("permission"), name}};
        step.request = [&, name, status](const lvrs::PermissionRequestStepContext &context,
                                         QVariantMap *details,
                                         QString *) {
            calls.append(name);
            previousResultCounts.append(context.previousResults.size());
            if (details) {
                details->insert(QStringLiteral("runPhase"),
                                context.runMetadata.value(QStringLiteral("phase")));
                details->insert(QStringLiteral("contextName"), context.name);
            }
            return status;
        };
        return step;
    };

    lvrs::PermissionRequestRunOptions options;
    options.logDiagnostics = false;
    options.metadata = {{QStringLiteral("phase"), QStringLiteral("startup")}};

    const lvrs::PermissionRequestRunResult result = sequencer.run({
        makeStep(QStringLiteral("photo-library"), 20, true, lvrs::PermissionRequestStatus::Granted),
        makeStep(QStringLiteral("full-disk-access"), 10, true, lvrs::PermissionRequestStatus::Granted),
        makeStep(QStringLiteral("hub-folder"), 30, false, lvrs::PermissionRequestStatus::Skipped)
    }, options);

    CTEST_CHECK_MESSAGE(result.ok, qPrintable(result.errorMessage()));
    CTEST_CHECK(!result.requiredFailure());
    CTEST_CHECK(!result.stoppedEarly);
    CTEST_COMPARE(result.runId, 1);
    CTEST_COMPARE(result.completedCount, 3);
    CTEST_COMPARE(result.grantedCount, 2);
    CTEST_COMPARE(result.skippedCount, 1);
    CTEST_COMPARE(calls,
             (QStringList {
                 QStringLiteral("full-disk-access"),
                 QStringLiteral("photo-library"),
                 QStringLiteral("hub-folder")
             }));
    CTEST_COMPARE(previousResultCounts, (QList<int> {0, 1, 2}));
    CTEST_COMPARE(result.stepResults.at(0).metadata.value(QStringLiteral("permission")).toString(),
             QStringLiteral("full-disk-access"));
    CTEST_COMPARE(result.stepResults.at(0).details.value(QStringLiteral("runPhase")).toString(),
             QStringLiteral("startup"));
    CTEST_COMPARE(result.diagnostics().size(), 3);
    CTEST_COMPARE(sequencer.history().size(), 3);
    CTEST_COMPARE(sequencer.runCount(), 1);
    CTEST_COMPARE(historyCount, 1);

    sequencer.clearHistory();
    CTEST_COMPARE(sequencer.history().size(), 0);
    CTEST_COMPARE(sequencer.runCount(), 0);
    CTEST_COMPARE(historyCount, 2);
}

void PermissionSequencerTests::sequencer_stops_on_required_failure_and_can_continue_when_configured()
{
    lvrs::PermissionRequestSequencer sequencer;

    QStringList calls;

    lvrs::PermissionRequestStep requiredDenied;
    requiredDenied.name = QStringLiteral("full-disk-access");
    requiredDenied.priority = 10;
    requiredDenied.required = true;
    requiredDenied.request = [&](const lvrs::PermissionRequestStepContext &, QVariantMap *, QString *errorMessage) {
        calls.append(QStringLiteral("required-denied"));
        if (errorMessage)
            *errorMessage = QStringLiteral("user denied full disk access");
        return lvrs::PermissionRequestStatus::Denied;
    };

    lvrs::PermissionRequestStep optionalUnavailable;
    optionalUnavailable.name = QStringLiteral("photo-library");
    optionalUnavailable.priority = 20;
    optionalUnavailable.required = false;
    optionalUnavailable.request = [&](const lvrs::PermissionRequestStepContext &, QVariantMap *, QString *errorMessage) {
        calls.append(QStringLiteral("optional-unavailable"));
        if (errorMessage)
            *errorMessage = QStringLiteral("photo library is unavailable");
        return lvrs::PermissionRequestStatus::Unavailable;
    };

    lvrs::PermissionRequestStep finalGranted;
    finalGranted.name = QStringLiteral("hub-folder");
    finalGranted.priority = 30;
    finalGranted.required = true;
    finalGranted.request = [&](const lvrs::PermissionRequestStepContext &, QVariantMap *, QString *) {
        calls.append(QStringLiteral("final-granted"));
        return lvrs::PermissionRequestStatus::Granted;
    };

    lvrs::PermissionRequestRunOptions stopOptions;
    stopOptions.logDiagnostics = false;

    const lvrs::PermissionRequestRunResult stopped =
        sequencer.run({finalGranted, optionalUnavailable, requiredDenied}, stopOptions);

    CTEST_CHECK(!stopped.ok);
    CTEST_CHECK(stopped.requiredFailure());
    CTEST_CHECK(stopped.stoppedEarly);
    CTEST_COMPARE(stopped.completedCount, 1);
    CTEST_COMPARE(stopped.deniedCount, 1);
    CTEST_COMPARE(calls, (QStringList {QStringLiteral("required-denied")}));
    CTEST_CHECK(stopped.errorMessage().contains(QStringLiteral("user denied full disk access")));
    CTEST_COMPARE(sequencer.history().size(), 1);

    calls.clear();

    lvrs::PermissionRequestRunOptions continueOptions;
    continueOptions.logDiagnostics = false;
    continueOptions.stopOnRequiredFailure = false;

    const lvrs::PermissionRequestRunResult continued =
        sequencer.run({finalGranted, optionalUnavailable, requiredDenied}, continueOptions);

    CTEST_CHECK(!continued.ok);
    CTEST_CHECK(continued.requiredFailure());
    CTEST_CHECK(!continued.stoppedEarly);
    CTEST_COMPARE(continued.completedCount, 3);
    CTEST_COMPARE(continued.grantedCount, 1);
    CTEST_COMPARE(continued.deniedCount, 1);
    CTEST_COMPARE(continued.unavailableCount, 1);
    CTEST_COMPARE(calls,
             (QStringList {
                 QStringLiteral("required-denied"),
                 QStringLiteral("optional-unavailable"),
                 QStringLiteral("final-granted")
             }));
    CTEST_CHECK(continued.stepResults.at(1).ok);
    CTEST_COMPARE(continued.stepResults.at(1).status, lvrs::PermissionRequestStatus::Unavailable);
    CTEST_COMPARE(sequencer.history().size(), 4);
    CTEST_COMPARE(sequencer.runCount(), 2);
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    PermissionSequencerTests tests;
    runner.run("sequencer_runs_steps_in_priority_order_and_records_history", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.sequencer_runs_steps_in_priority_order_and_records_history(); });
    runner.run("sequencer_stops_on_required_failure_and_can_continue_when_configured", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.sequencer_stops_on_required_failure_and_can_continue_when_configured(); });
    return runner.result();
}
