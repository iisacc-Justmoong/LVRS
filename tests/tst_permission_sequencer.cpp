#include <QtTest>

#include <QSignalSpy>

#include "backend/runtime/permissionsequencer.h"

class PermissionSequencerTests : public QObject
{
    Q_OBJECT

private slots:
    void sequencer_runs_steps_in_priority_order_and_records_history();
    void sequencer_stops_on_required_failure_and_can_continue_when_configured();
};

void PermissionSequencerTests::sequencer_runs_steps_in_priority_order_and_records_history()
{
    lvrs::PermissionRequestSequencer sequencer;
    QSignalSpy historySpy(&sequencer, &lvrs::PermissionRequestSequencer::historyChanged);
    QVERIFY(historySpy.isValid());

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

    QVERIFY2(result.ok, qPrintable(result.errorMessage()));
    QVERIFY(!result.requiredFailure());
    QVERIFY(!result.stoppedEarly);
    QCOMPARE(result.runId, 1);
    QCOMPARE(result.completedCount, 3);
    QCOMPARE(result.grantedCount, 2);
    QCOMPARE(result.skippedCount, 1);
    QCOMPARE(calls,
             (QStringList {
                 QStringLiteral("full-disk-access"),
                 QStringLiteral("photo-library"),
                 QStringLiteral("hub-folder")
             }));
    QCOMPARE(previousResultCounts, (QList<int> {0, 1, 2}));
    QCOMPARE(result.stepResults.at(0).metadata.value(QStringLiteral("permission")).toString(),
             QStringLiteral("full-disk-access"));
    QCOMPARE(result.stepResults.at(0).details.value(QStringLiteral("runPhase")).toString(),
             QStringLiteral("startup"));
    QCOMPARE(result.diagnostics().size(), 3);
    QCOMPARE(sequencer.history().size(), 3);
    QCOMPARE(sequencer.runCount(), 1);
    QCOMPARE(historySpy.count(), 1);

    sequencer.clearHistory();
    QCOMPARE(sequencer.history().size(), 0);
    QCOMPARE(sequencer.runCount(), 0);
    QCOMPARE(historySpy.count(), 2);
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

    QVERIFY(!stopped.ok);
    QVERIFY(stopped.requiredFailure());
    QVERIFY(stopped.stoppedEarly);
    QCOMPARE(stopped.completedCount, 1);
    QCOMPARE(stopped.deniedCount, 1);
    QCOMPARE(calls, (QStringList {QStringLiteral("required-denied")}));
    QVERIFY(stopped.errorMessage().contains(QStringLiteral("user denied full disk access")));
    QCOMPARE(sequencer.history().size(), 1);

    calls.clear();

    lvrs::PermissionRequestRunOptions continueOptions;
    continueOptions.logDiagnostics = false;
    continueOptions.stopOnRequiredFailure = false;

    const lvrs::PermissionRequestRunResult continued =
        sequencer.run({finalGranted, optionalUnavailable, requiredDenied}, continueOptions);

    QVERIFY(!continued.ok);
    QVERIFY(continued.requiredFailure());
    QVERIFY(!continued.stoppedEarly);
    QCOMPARE(continued.completedCount, 3);
    QCOMPARE(continued.grantedCount, 1);
    QCOMPARE(continued.deniedCount, 1);
    QCOMPARE(continued.unavailableCount, 1);
    QCOMPARE(calls,
             (QStringList {
                 QStringLiteral("required-denied"),
                 QStringLiteral("optional-unavailable"),
                 QStringLiteral("final-granted")
             }));
    QVERIFY(continued.stepResults.at(1).ok);
    QCOMPARE(continued.stepResults.at(1).status, lvrs::PermissionRequestStatus::Unavailable);
    QCOMPARE(sequencer.history().size(), 4);
    QCOMPARE(sequencer.runCount(), 2);
}

QTEST_MAIN(PermissionSequencerTests)
#include "tst_permission_sequencer.moc"
