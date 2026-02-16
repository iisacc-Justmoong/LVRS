#include <QtTest>

#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QtPlugin>

#include "backend/runtime/debuglogger.h"
#include "backend/runtime/renderingmonitor.h"
#include "backend/runtime/runtimeevents.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class RuntimeServicesTests : public QObject
{
    Q_OBJECT

private slots:
    void runtime_events_measurement_boundaries();
    void runtime_events_daemon_health_and_recent_log();
    void runtime_events_idle_and_reset_signal_contract();
    void runtime_events_context_menu_signal_contract();
    void runtime_events_single_input_event_is_counted_once();
    void runtime_events_window_replacement_survives_old_window_destruction();
    void render_monitor_counts_frames_when_swapped();
    void render_monitor_active_signal_and_destroy_path();
    void debug_logger_enabled_signal_and_message_format();
};

void RuntimeServicesTests::runtime_events_measurement_boundaries()
{
    RuntimeEvents events;
    QQuickWindow window;
    window.setWidth(800);
    window.setHeight(500);
    events.attachWindow(&window);
    QTRY_VERIFY(events.running());
    QCOMPARE(events.uiCreatedCount(), quint64(0));
    QCOMPARE(events.captureProfile(), static_cast<int>(RuntimeEvents::LowLatencyCapture));
    QVERIFY(!events.pointerHitTestingEnabled());
    QVERIFY(!events.uiTrackingEnabled());

    events.setIdleTimeoutMs(-10);
    QCOMPARE(events.idleTimeoutMs(), 250);
    events.setIdleTimeoutMs(24 * 60 * 60 * 1000 + 999);
    QCOMPARE(events.idleTimeoutMs(), 24 * 60 * 60 * 1000);

    events.setOsSampleIntervalMs(-10);
    QCOMPARE(events.osSampleIntervalMs(), 250);
    events.setOsSampleIntervalMs(70 * 1000);
    QCOMPARE(events.osSampleIntervalMs(), 60 * 1000);

    events.setCaptureProfile(RuntimeEvents::LowLatencyCapture);
    QCOMPARE(events.captureProfile(), static_cast<int>(RuntimeEvents::LowLatencyCapture));
    QVERIFY(!events.pointerHitTestingEnabled());
    QVERIFY(!events.uiTrackingEnabled());

    events.setCaptureProfile(RuntimeEvents::FullCapture);
    QCOMPARE(events.captureProfile(), static_cast<int>(RuntimeEvents::FullCapture));
    QVERIFY(events.pointerHitTestingEnabled());
    QVERIFY(events.uiTrackingEnabled());
    QVERIFY(events.uiCreatedCount() > 0);
    events.setPointerHitTestMinIntervalMs(48);
    QCOMPARE(events.pointerHitTestMinIntervalMs(), 48);

    const qint64 previousActivityEpoch = events.lastActivityEpochMs();
    QTest::qWait(2);
    events.markActivity();
    QVERIFY(events.lastActivityEpochMs() >= previousActivityEpoch);

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_K, Qt::ControlModifier, QStringLiteral("k"));
    QCoreApplication::sendEvent(&window, &keyPress);
    QVERIFY(events.isKeyPressed(Qt::Key_K));
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_K, Qt::ControlModifier, QStringLiteral("k"));
    QCoreApplication::sendEvent(&window, &keyRelease);
    QVERIFY(!events.isKeyPressed(Qt::Key_K));
    QVERIFY(events.keyPressCount() >= 1);
    QVERIFY(events.keyReleaseCount() >= 1);

    const QPointF p(24.0, 18.0);
    QMouseEvent mouseMove(QEvent::MouseMove, p, p, p, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QMouseEvent mousePress(QEvent::MouseButtonPress, p, p, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent mouseRelease(QEvent::MouseButtonRelease, p, p, p, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &mouseMove);
    QCoreApplication::sendEvent(&window, &mousePress);
    QCoreApplication::sendEvent(&window, &mouseRelease);
    QVERIFY(events.mouseMoveCount() >= 1);
    QVERIFY(events.mousePressCount() >= 1);
    QVERIFY(events.mouseReleaseCount() >= 1);

    const quint64 createdBefore = events.uiCreatedCount();
    auto *dynamicItem = new QQuickItem(window.contentItem());
    dynamicItem->setVisible(true);
    QCoreApplication::processEvents();
    QVERIFY(events.uiCreatedCount() > createdBefore);
    delete dynamicItem;
    QCoreApplication::processEvents();
    QVERIFY(events.uiDestroyedCount() >= 1);

    const QVariantMap snapshot = events.snapshot();
    QVERIFY(snapshot.contains(QStringLiteral("running")));
    QVERIFY(snapshot.contains(QStringLiteral("keyPressCount")));
    QVERIFY(snapshot.contains(QStringLiteral("mouseMoveCount")));
    QVERIFY(snapshot.contains(QStringLiteral("uiCreatedCount")));
    QVERIFY(snapshot.contains(QStringLiteral("idle")));
    QVERIFY(snapshot.contains(QStringLiteral("pid")));
    QVERIFY(snapshot.contains(QStringLiteral("rssBytes")));
    QVERIFY(snapshot.contains(QStringLiteral("uptimeMs")));
    QVERIFY(snapshot.contains(QStringLiteral("daemonBootEpochMs")));
    QVERIFY(snapshot.contains(QStringLiteral("eventSequence")));
    QVERIFY(snapshot.contains(QStringLiteral("recentEventCount")));
    QVERIFY(snapshot.value(QStringLiteral("pid")).toLongLong() > 0);

    events.stop();
    QVERIFY(!events.running());
}

void RuntimeServicesTests::runtime_events_daemon_health_and_recent_log()
{
    RuntimeEvents events;
    QQuickWindow window;
    window.setWidth(640);
    window.setHeight(360);
    events.attachWindow(&window);
    QTRY_VERIFY(events.running());

    QVERIFY(events.daemonBootEpochMs() > 0);
    const QVariantMap health = events.daemonHealth();
    QVERIFY(health.value(QStringLiteral("running")).toBool());
    QVERIFY(health.value(QStringLiteral("attachedWindow")).toBool());
    QVERIFY(health.contains(QStringLiteral("eventSequence")));
    QVERIFY(health.contains(QStringLiteral("recentEventCount")));

    const quint64 seqBefore = events.eventSequence();

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_D, Qt::NoModifier, QStringLiteral("d"));
    QCoreApplication::sendEvent(&window, &keyPress);
    QTRY_VERIFY(events.eventSequence() > seqBefore);

    const QVariantMap lastEvent = events.lastEvent();
    QVERIFY(!lastEvent.isEmpty());
    QVERIFY(lastEvent.contains(QStringLiteral("type")));
    QVERIFY(lastEvent.contains(QStringLiteral("sequence")));
    QVERIFY(events.recentEventCount() > 0);
    QVERIFY(!events.recentEvents().isEmpty());

    events.setRecentEventCapacity(16);
    QCOMPARE(events.recentEventCapacity(), 16);
    events.clearRecentEvents();
    QCOMPARE(events.recentEventCount(), 0);
}

void RuntimeServicesTests::runtime_events_idle_and_reset_signal_contract()
{
    RuntimeEvents events;
    QQuickWindow window;
    events.attachWindow(&window);
    QTRY_VERIFY(events.running());

    QSignalSpy idleChangedSpy(&events, &RuntimeEvents::idleChanged);
    QSignalSpy idleEnteredSpy(&events, &RuntimeEvents::idleEntered);
    QSignalSpy idleExitedSpy(&events, &RuntimeEvents::idleExited);
    QSignalSpy runningSpy(&events, &RuntimeEvents::runningChanged);
    QVERIFY(idleChangedSpy.isValid());
    QVERIFY(idleEnteredSpy.isValid());
    QVERIFY(idleExitedSpy.isValid());
    QVERIFY(runningSpy.isValid());

    events.setIdleTimeoutMs(250);
    events.markActivity();
    QTRY_VERIFY_WITH_TIMEOUT(events.idle(), 1200);
    QVERIFY(idleChangedSpy.count() >= 1);
    QVERIFY(idleEnteredSpy.count() >= 1);

    events.markActivity();
    QTRY_VERIFY(!events.idle());
    QVERIFY(idleExitedSpy.count() >= 1);

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_T, Qt::NoModifier, QStringLiteral("t"));
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_T, Qt::NoModifier, QStringLiteral("t"));
    QCoreApplication::sendEvent(&window, &keyPress);
    QVERIFY(events.anyKeyPressed());
    QCoreApplication::sendEvent(&window, &keyRelease);
    QVERIFY(!events.anyKeyPressed());

    const QPointF p(12.0, 14.0);
    QMouseEvent mousePress(QEvent::MouseButtonPress, p, p, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent mouseRelease(QEvent::MouseButtonRelease, p, p, p, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &mousePress);
    QVERIFY(events.mouseButtonPressed());
    QCoreApplication::sendEvent(&window, &mouseRelease);
    QVERIFY(!events.mouseButtonPressed());

    events.stop();
    QVERIFY(!events.running());

    events.resetCounters();
    QCOMPARE(events.keyPressCount(), 0u);
    QCOMPARE(events.keyReleaseCount(), 0u);
    QCOMPARE(events.mouseMoveCount(), 0u);
    QCOMPARE(events.mousePressCount(), 0u);
    QCOMPARE(events.mouseReleaseCount(), 0u);
    QCOMPARE(events.uiCreatedCount(), 0u);
    QCOMPARE(events.uiShownCount(), 0u);
    QCOMPARE(events.uiHiddenCount(), 0u);
    QCOMPARE(events.uiDestroyedCount(), 0u);
    QVERIFY(!events.anyKeyPressed());
    QVERIFY(!events.mouseButtonPressed());
    QCOMPARE(runningSpy.count(), 1);
}

void RuntimeServicesTests::runtime_events_context_menu_signal_contract()
{
    RuntimeEvents events;
    QQuickWindow window;
    events.attachWindow(&window);
    QTRY_VERIFY(events.running());

    QSignalSpy contextSpy(&events, &RuntimeEvents::contextRequested);
    QVERIFY(contextSpy.isValid());

    const QPoint local(22, 16);
    const QPoint global(320, 240);
    QContextMenuEvent contextEvent(QContextMenuEvent::Mouse,
                                   local,
                                   global,
                                   Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &contextEvent);

    QTRY_VERIFY(contextSpy.count() >= 1);
    const QList<QVariant> args = contextSpy.takeLast();
    QVERIFY(args.size() >= 4);
    QCOMPARE(args.at(2).toInt(), static_cast<int>(Qt::NoModifier));
    QCOMPARE(args.at(3).toInt(), static_cast<int>(QContextMenuEvent::Mouse));
    QCOMPARE(events.lastMouseButtons(), static_cast<int>(Qt::RightButton));
    QCOMPARE(events.lastMouseX(), static_cast<qreal>(global.x()));
    QCOMPARE(events.lastMouseY(), static_cast<qreal>(global.y()));
}

void RuntimeServicesTests::runtime_events_single_input_event_is_counted_once()
{
    RuntimeEvents events;
    QQuickWindow window;
    events.attachWindow(&window);
    QTRY_VERIFY(events.running());

    events.resetCounters();

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_G, Qt::NoModifier, QStringLiteral("g"));
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_G, Qt::NoModifier, QStringLiteral("g"));
    QCoreApplication::sendEvent(&window, &keyPress);
    QCoreApplication::sendEvent(&window, &keyRelease);

    const QPointF p(30.0, 22.0);
    QMouseEvent mouseMove(QEvent::MouseMove, p, p, p, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QMouseEvent mousePress(QEvent::MouseButtonPress, p, p, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent mouseRelease(QEvent::MouseButtonRelease, p, p, p, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &mouseMove);
    QCoreApplication::sendEvent(&window, &mousePress);
    QCoreApplication::sendEvent(&window, &mouseRelease);

    QCOMPARE(events.keyPressCount(), 1u);
    QCOMPARE(events.keyReleaseCount(), 1u);
    QCOMPARE(events.mouseMoveCount(), 1u);
    QCOMPARE(events.mousePressCount(), 1u);
    QCOMPARE(events.mouseReleaseCount(), 1u);
    QCOMPARE(events.lastMouseX(), 30.0);
    QCOMPARE(events.lastMouseY(), 22.0);
}

void RuntimeServicesTests::runtime_events_window_replacement_survives_old_window_destruction()
{
    RuntimeEvents events;

    auto *firstWindow = new QQuickWindow;
    firstWindow->setWidth(640);
    firstWindow->setHeight(360);
    events.attachWindow(firstWindow);
    QTRY_VERIFY(events.running());

    auto *secondWindow = new QQuickWindow;
    secondWindow->setWidth(640);
    secondWindow->setHeight(360);
    events.attachWindow(secondWindow);
    QTRY_VERIFY(events.daemonHealth().value(QStringLiteral("attachedWindow")).toBool());

    delete firstWindow;
    QCoreApplication::processEvents();
    QTRY_VERIFY(events.daemonHealth().value(QStringLiteral("attachedWindow")).toBool());

    const quint64 sequenceBefore = events.eventSequence();
    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier, QStringLiteral("r"));
    QCoreApplication::sendEvent(secondWindow, &keyPress);
    QTRY_VERIFY(events.eventSequence() > sequenceBefore);

    delete secondWindow;
    QCoreApplication::processEvents();
    QTRY_VERIFY(!events.daemonHealth().value(QStringLiteral("attachedWindow")).toBool());
}

void RuntimeServicesTests::render_monitor_counts_frames_when_swapped()
{
    RenderingMonitor monitor;
    QQuickWindow window;
    monitor.attachWindow(&window);
    QVERIFY(monitor.active());
    QCOMPARE(monitor.frameCount(), 0u);

    QVERIFY(QMetaObject::invokeMethod(&monitor, "handleFrameSwapped", Qt::DirectConnection));
    QTest::qWait(5);
    QVERIFY(QMetaObject::invokeMethod(&monitor, "handleFrameSwapped", Qt::DirectConnection));
    QVERIFY(monitor.frameCount() >= 2);
    QVERIFY(monitor.lastFrameMs() >= 0.0);
    QVERIFY(monitor.fps() >= 0.0);

    monitor.reset();
    QCOMPARE(monitor.frameCount(), 0u);
    QCOMPARE(monitor.lastFrameMs(), 0.0);

    monitor.stop();
    QVERIFY(!monitor.active());
    monitor.start();
    QVERIFY(monitor.active());
    monitor.attachWindow(static_cast<QObject *>(nullptr));
    QVERIFY(!monitor.active());
}

void RuntimeServicesTests::render_monitor_active_signal_and_destroy_path()
{
    RenderingMonitor monitor;
    QSignalSpy activeSpy(&monitor, &RenderingMonitor::activeChanged);
    QSignalSpy statsSpy(&monitor, &RenderingMonitor::statsChanged);
    QVERIFY(activeSpy.isValid());
    QVERIFY(statsSpy.isValid());

    auto *window = new QQuickWindow;
    monitor.attachWindow(window);
    QVERIFY(monitor.active());
    QCOMPARE(activeSpy.count(), 1);
    QVERIFY(statsSpy.count() >= 1);

    QVERIFY(QMetaObject::invokeMethod(&monitor, "handleFrameSwapped", Qt::DirectConnection));
    const quint64 frameCountBeforeStop = monitor.frameCount();
    QVERIFY(frameCountBeforeStop >= 1);

    monitor.stop();
    QCOMPARE(activeSpy.count(), 2);
    QVERIFY(!monitor.active());
    QVERIFY(QMetaObject::invokeMethod(&monitor, "handleFrameSwapped", Qt::DirectConnection));
    QCOMPARE(monitor.frameCount(), frameCountBeforeStop);

    monitor.start();
    QCOMPARE(activeSpy.count(), 3);
    QVERIFY(monitor.active());

    delete window;
    QCoreApplication::processEvents();
    QTRY_VERIFY(!monitor.active());
    QCOMPARE(activeSpy.count(), 4);
}

void RuntimeServicesTests::debug_logger_enabled_signal_and_message_format()
{
    DebugLogger logger;
    QSignalSpy enabledSpy(&logger, &DebugLogger::enabledChanged);
    QVERIFY(enabledSpy.isValid());

    logger.setEnabled(false);
    QCOMPARE(enabledSpy.count(), 0);
    logger.setEnabled(true);
    QCOMPARE(enabledSpy.count(), 1);
    logger.setEnabled(true);
    QCOMPARE(enabledSpy.count(), 1);

    logger.log(QString(), QString(), 42);
    logger.warn(QStringLiteral("Runtime"), QStringLiteral("warning"));
    logger.error(QString(), QStringLiteral("failure"));

    const QVariantList entries = logger.entries();
    QCOMPARE(entries.size(), 3);

    const QVariantMap logEntry = entries.at(0).toMap();
    QCOMPARE(logEntry.value(QStringLiteral("level")).toString(), QStringLiteral("LOG"));
    QCOMPARE(logEntry.value(QStringLiteral("component")).toString(), QStringLiteral("Unknown"));
    QCOMPARE(logEntry.value(QStringLiteral("event")).toString(), QStringLiteral("event"));
    QVERIFY(logEntry.value(QStringLiteral("message")).toString().contains(QStringLiteral("Unknown.event")));

    const QVariantMap warnEntry = entries.at(1).toMap();
    QCOMPARE(warnEntry.value(QStringLiteral("level")).toString(), QStringLiteral("WARN"));
    QCOMPARE(warnEntry.value(QStringLiteral("component")).toString(), QStringLiteral("Runtime"));
    QCOMPARE(warnEntry.value(QStringLiteral("event")).toString(), QStringLiteral("warning"));

    const QVariantMap errorEntry = entries.at(2).toMap();
    QCOMPARE(errorEntry.value(QStringLiteral("level")).toString(), QStringLiteral("ERROR"));
    QCOMPARE(errorEntry.value(QStringLiteral("component")).toString(), QStringLiteral("Unknown"));
    QCOMPARE(errorEntry.value(QStringLiteral("event")).toString(), QStringLiteral("failure"));

    logger.setEnabled(false);
    QCOMPARE(enabledSpy.count(), 2);
    logger.log(QStringLiteral("PostDisable"), QStringLiteral("ignored"));
    QCOMPARE(logger.entries().size(), 3);
}

QTEST_MAIN(RuntimeServicesTests)
#include "tst_runtime_services.moc"
