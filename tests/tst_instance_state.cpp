#include <QtTest>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include "test_utils.h"

class InstanceStateTests : public QObject
{
    Q_OBJECT
private slots:
    void owned_input_lifecycle_data();
    void owned_input_lifecycle();
    void cancellation_and_motion_preferences();
    void model_states_remain_independent();
    void menu_selected_uses_press_fill_data();
    void menu_selected_uses_press_fill();
    void mobile_hierarchy_input();
    void native_focus_capture();
    void recorded_figma_contract();
};

static QObject *fixture(QQmlEngine &engine, const QString &type,
                        const QString &configuration = {})
{
    engine.addImportPath(TestUtils::qmlImportBase());
    return TestUtils::createFromQml(engine, QString(R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    visible: true; width: 700; height: 400
    color: LV.Theme.window
    function reduceMotion() { LV.Motion.reducedMotion = true }
    function restoreMotion() { LV.Motion.reducedMotion = false; LV.Motion.enabled = true }
    LV.%1 { objectName: "control"; x: 40; y: 40; width: 300; text: "Label"; %2 }
    LV.%1 { objectName: "other"; x: 40; y: 210; width: 300; text: "Label"; %2 }
    LV.PushButton { objectName: "next"; x: 500; y: 300; text: "Next" }
})").arg(type, configuration).toUtf8());
}

static QString phase(QObject *control)
{
    return control->property("interactionPhase").toString();
}

void InstanceStateTests::owned_input_lifecycle_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("configuration");
    for (const char *type : {"PushButton", "DropdownButton", "IconButton", "IconMenuButton",
                            "MenuItem", "ContextMenuItem", "HierarchyItem"})
        QTest::newRow(type) << QString::fromLatin1(type) << QString();
    for (int type = 0; type < 17; ++type) {
        const QByteArray row = "ListItem-" + QByteArray::number(type);
        QTest::newRow(row.constData()) << QStringLiteral("ListItem")
            << QStringLiteral("type: %1").arg(type);
    }
}

void InstanceStateTests::owned_input_lifecycle()
{
    QFETCH(QString, type);
    QFETCH(QString, configuration);
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine, type, configuration));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    auto *other = root->findChild<QQuickItem *>("other");
    auto *next = root->findChild<QQuickItem *>("next");
    QVERIFY(control && other && next);
    auto *state = control->property("interaction").value<QObject *>();
    auto *otherState = other->property("interaction").value<QObject *>();
    QVERIFY2(state && otherState, "Each component must own its interaction state object");
    QVERIFY(state != otherState);
    QCOMPARE(state, control->findChild<QObject *>("controlInstanceState"));
    QVERIFY(QTest::qWaitForWindowExposed(window));
    next->forceActiveFocus(Qt::MouseFocusReason);
    QTest::mouseMove(window, QPoint(650, 350));
    QTRY_COMPARE(phase(control), QStringLiteral("default"));
    QCOMPARE(control->property("interactionInput").toString(), QStringLiteral("pointer"));
    const QRectF geometry = control->mapRectToScene(control->boundingRect());
    // The leading edge activates the row without stealing embedded accessory input.
    const QPoint point = control->mapToScene(QPointF(2, control->height() / 2)).toPoint();
    QSignalSpy clicks(control, SIGNAL(clicked()));
    QTest::mouseMove(window, point);
    QTRY_COMPARE(phase(control), QStringLiteral("hover"));
    QCOMPARE(phase(other), QStringLiteral("default"));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(phase(control), QStringLiteral("press"));
    QCOMPARE(clicks.count(), 0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(phase(control), QStringLiteral("release"));
    QCOMPARE(clicks.count(), 1);
    QTRY_COMPARE(phase(control), QStringLiteral("hover"));
    QTest::mouseMove(window, QPoint(650, 350));
    next->forceActiveFocus(Qt::TabFocusReason);
    control->forceActiveFocus(Qt::TabFocusReason);
    QTRY_COMPARE(phase(control), QStringLiteral("focus"));
    QCOMPARE(control->property("interactionInput").toString(), QStringLiteral("keyboard"));
    QVERIFY(state->property("focusVisible").toBool());
    auto *ring = control->findChild<QQuickItem *>("buttonFocusRing");
    QVERIFY(ring && ring->property("active").toBool());
    // The row must not clip its own 3px external focus outline.
    QVERIFY(!control->clip());
    for (Qt::Key key : {Qt::Key_Space, Qt::Key_Return, Qt::Key_Enter}) {
        const int before = clicks.count();
        QTest::keyPress(window, key);
        QCOMPARE(phase(control), QStringLiteral("press"));
        QVERIFY(state->property("focusVisible").toBool());
        QCOMPARE(clicks.count(), before);
        QTest::keyRelease(window, key);
        QCOMPARE(phase(control), QStringLiteral("release"));
        QCOMPARE(clicks.count(), before + 1);
        QVERIFY(ring->property("active").toBool());
        QTRY_COMPARE(phase(control), QStringLiteral("focus"));
    }
    QCOMPARE(control->mapRectToScene(control->boundingRect()), geometry);
    QCOMPARE(phase(other), QStringLiteral("default"));
    // Enter completion must restore native pointer tracking on this instance.
    QTest::mouseMove(window, point);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(phase(control), QStringLiteral("press"));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(phase(control), QStringLiteral("release"));
    control->setEnabled(false);
    QCOMPARE(phase(control), QStringLiteral("disabled"));
    QVERIFY(!state->property("focusVisible").toBool());
    const int before = clicks.count();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(clicks.count(), before);
}

void InstanceStateTests::cancellation_and_motion_preferences()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine, QStringLiteral("PushButton")));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    auto *next = root->findChild<QQuickItem *>("next");
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(control->property("interaction").value<QObject *>());
    const QPoint center = control->mapToScene(control->boundingRect().center()).toPoint();
    QSignalSpy clicks(control, SIGNAL(clicked()));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QTest::mouseMove(window, QPoint(650, 350));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(650, 350));
    QCOMPARE(clicks.count(), 0);
    QVERIFY(phase(control) != QStringLiteral("release"));
    control->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyPress(window, Qt::Key_Return);
    next->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyRelease(window, Qt::Key_Return);
    QCOMPARE(clicks.count(), 0);
    QTRY_COMPARE(phase(control), QStringLiteral("default"));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(phase(control), QStringLiteral("release"));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(phase(control), QStringLiteral("press"));
    auto *motion = control->findChild<QObject *>("interactionMotion");
    QVERIFY(motion && !motion->property("releasing").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "reduceMotion"));
    QTRY_COMPARE(phase(control), QStringLiteral("hover"));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(phase(control), QStringLiteral("press"));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(phase(control), QStringLiteral("hover"));
    QVERIFY(QMetaObject::invokeMethod(root.data(), "restoreMotion"));
    control->setProperty("motionEnabled", false);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(phase(control), QStringLiteral("hover"));
}

void InstanceStateTests::model_states_remain_independent()
{
    for (const QString &type : {QStringLiteral("ListItem"), QStringLiteral("MenuItem"),
                                QStringLiteral("ContextMenuItem"), QStringLiteral("HierarchyItem")}) {
        const bool menu = type.contains(QStringLiteral("MenuItem"));
        QQmlEngine engine;
        QScopedPointer<QObject> root(fixture(engine, type, menu ? "state: selectedState" : "selected: true"));
        QVERIFY(root);
        auto *window = qobject_cast<QQuickWindow *>(root.data());
        auto *control = root->findChild<QQuickItem *>("control");
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QVERIFY(control->property("interaction").value<QObject *>());
        root->findChild<QQuickItem *>("next")->forceActiveFocus(Qt::TabFocusReason);
        control->forceActiveFocus(Qt::TabFocusReason);
        QTRY_COMPARE(phase(control), QStringLiteral("focus"));
        QTest::keyClick(window, Qt::Key_Return);
        QCOMPARE(phase(control), QStringLiteral("release"));
        if (menu) {
            QCOMPARE(control->property("state").toInt(), control->property("selectedState").toInt());
            control->setProperty("state", control->property("inactiveState"));
        } else if (type == "HierarchyItem") {
            QVERIFY(control->property("selected").toBool());
            control->setProperty("dragPreviewActive", true);
            QCOMPARE(phase(control), QStringLiteral("disabled"));
            QCOMPARE(control->property("uxStateName").toString(), QStringLiteral("Drag"));
            control->setProperty("dragPreviewActive", false);
            control->setProperty("activatable", false);
        } else {
            QVERIFY(control->property("selected").toBool());
            control->setEnabled(false);
        }
        QCOMPARE(phase(control), QStringLiteral("disabled"));
        QVERIFY(!control->property("interaction").value<QObject *>()->property("focusVisible").toBool());
    }
}

void InstanceStateTests::menu_selected_uses_press_fill_data()
{
    QTest::addColumn<bool>("expanded");
    QTest::addColumn<QString>("primary");
    for (bool expanded : {false, true}) {
        for (const QString &primary : {QStringLiteral("#0A84FF"), QStringLiteral("#A571E6")}) {
            const QByteArray name = (expanded ? "expanded-" : "collapsed-") + primary.toUtf8();
            QTest::newRow(name.constData()) << expanded << primary;
        }
    }
}

void InstanceStateTests::menu_selected_uses_press_fill()
{
    QFETCH(bool, expanded);
    QFETCH(QString, primary);
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine, "MenuItem",
        QStringLiteral("expanded: %1; motionEnabled: false").arg(expanded ? "true" : "false")));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    auto *other = root->findChild<QQuickItem *>("other");
    auto *next = root->findChild<QQuickItem *>("next");
    QVERIFY(control && other && next);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QScopedPointer<QObject> tokens(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; QtObject { property var theme: LV.Theme }"));
    QVERIFY(tokens);
    auto *theme = tokens->property("theme").value<QObject *>();
    QVERIFY(theme);
    QVERIFY(theme->setProperty("primaryColor", QColor(primary)));
    const QColor expected = other->property("backgroundColorPressed").value<QColor>();
    if (primary == "#0A84FF")
        QCOMPARE(expected, QColor("#25324D"));
    auto *background = control->property("background").value<QQuickItem *>();
    QVERIFY(background);
    QVERIFY(control->setProperty("state", control->property("selectedState")));
    const auto verifyFill = [&] {
        QCOMPARE(background->property("color").value<QColor>(), expected);
        const QImage frame = window->grabWindow();
        QVERIFY(!frame.isNull());
        const QPointF center = control->mapToScene(control->boundingRect().center()) * (qreal(frame.width()) / window->width());
        const QColor pixel = frame.pixelColor(qRound(center.x()), qRound(center.y()));
        QVERIFY2(qAbs(pixel.red() - expected.red()) <= 2
                 && qAbs(pixel.green() - expected.green()) <= 2
                 && qAbs(pixel.blue() - expected.blue()) <= 2,
                 qPrintable(QString("Selected rendered %1; press uses %2").arg(pixel.name(), expected.name())));
    };
    next->forceActiveFocus(Qt::MouseFocusReason);
    QTest::mouseMove(window, QPoint(650, 350));
    QTRY_COMPARE(phase(control), QStringLiteral("default"));
    verifyFill();
    const QPoint point = control->mapToScene(QPointF(2, control->height() / 2)).toPoint();
    QTest::mouseMove(window, point);
    QTRY_COMPARE(phase(control), QStringLiteral("hover"));
    verifyFill();
    control->setProperty("motionEnabled", true);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(phase(control), QStringLiteral("press"));
    verifyFill();
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(phase(control), QStringLiteral("release"));
    verifyFill();
    QTest::mouseMove(window, QPoint(650, 350));
    next->forceActiveFocus(Qt::TabFocusReason);
    control->forceActiveFocus(Qt::TabFocusReason);
    QTRY_COMPARE(phase(control), QStringLiteral("focus"));
    verifyFill();
    QTest::keyPress(window, Qt::Key_Return);
    QCOMPARE(phase(control), QStringLiteral("press"));
    verifyFill();
    QTest::keyRelease(window, Qt::Key_Return);
    QCOMPARE(phase(control), QStringLiteral("release"));
    verifyFill();
    QCOMPARE(control->property("state").toInt(), control->property("selectedState").toInt());
    QScopedPointer<QObject> compact(TestUtils::createFromQml(engine,
        "import LVRS as LV; LV.ContextMenuItem { state: selectedState }"));
    QVERIFY(compact);
    QCOMPARE(compact->property("resolvedBackgroundColor").value<QColor>(), QColor(primary));
}

void InstanceStateTests::recorded_figma_contract()
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-instance-state.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonParseError error;
    const QJsonObject contract = QJsonDocument::fromJson(file.readAll(), &error).object();
    QCOMPARE(error.error, QJsonParseError::NoError);
    QCOMPARE(contract.value("fileKey").toString(), QStringLiteral("0GkItQYSNIR0lZ3iJhfJzc"));
    QCOMPARE(contract.value("phases").toArray(), QJsonArray({"default", "hover", "press", "release", "focus"}));
    const QJsonArray sets = contract.value("sets").toArray();
    QCOMPARE(sets.size(), 25);
    int variants = 0;
    QSet<QString> ids;
    for (const QJsonValue &value : sets) {
        const QJsonObject set = value.toObject();
        const int count = set.value("count").toInt();
        QVERIFY(count > 0 && count <= 30);
        QCOMPARE(set.value("boundVariantCount").toInt(), count);
        QCOMPARE(set.value("verifiedInstanceOverrides").toInt(), count);
        QVERIFY(!ids.contains(set.value("id").toString()));
        ids.insert(set.value("id").toString());
        variants += count;
    }
    QCOMPARE(variants, 740);
    QCOMPARE(contract.value("remainingOverlayInstances").toInt(-1), 0);
    QCOMPARE(contract.value("removedOverlaySets").toArray().size(), 5);
    QCOMPARE(contract.value("auditedPages").toInt(), 23);
    QCOMPARE(contract.value("migratedInstances").toInt(), 539);
}

void InstanceStateTests::mobile_hierarchy_input()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    visible: true; width: 500; height: 260
    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""
    QtObject {
        id: owner
        property bool editableEnabled: true
        property int _dragTargetIndex: -1
        property int _dragTargetDepth: -1
    }
    LV.HierarchyItem {
        objectName: "control"; x: 40; y: 40; width: 300
        hierarchyList: owner; generatedByTreeModel: true; showChevron: false
    }
    LV.PushButton { objectName: "next"; x: 350; y: 200 }
})"));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(control->property("pointerDragRequiresLongPress").toBool());
    const QPoint center = control->mapToScene(control->boundingRect().center()).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(phase(control), QStringLiteral("press"));
    QVERIFY(!control->property("down").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(phase(control), QStringLiteral("release"));
    QTRY_COMPARE(phase(control), QStringLiteral("default"));
    root->findChild<QQuickItem *>("next")->forceActiveFocus(Qt::TabFocusReason);
    control->forceActiveFocus(Qt::TabFocusReason);
    QTRY_COMPARE(phase(control), QStringLiteral("focus"));
    for (Qt::Key key : {Qt::Key_Space, Qt::Key_Return}) {
        QTest::keyPress(window, key);
        QCOMPARE(phase(control), QStringLiteral("press"));
        QTest::keyRelease(window, key);
        QCOMPARE(phase(control), QStringLiteral("release"));
        QTRY_COMPARE(phase(control), QStringLiteral("focus"));
    }
    control->setProperty("dragPreviewActive", true);
    QCOMPARE(phase(control), QStringLiteral("disabled"));
}

void InstanceStateTests::native_focus_capture()
{
    const QString directory = qEnvironmentVariable("LVRS_STATE_CAPTURE_DIR");
    if (directory.isEmpty())
        QSKIP("Set LVRS_STATE_CAPTURE_DIR for rendered state captures.");
    QVERIFY(QDir().mkpath(directory));
    for (const QString &type : {QStringLiteral("PushButton"), QStringLiteral("DropdownButton"),
                                QStringLiteral("ListItem"), QStringLiteral("MenuItem"),
                                QStringLiteral("ContextMenuItem"), QStringLiteral("HierarchyItem")}) {
        QQmlEngine engine;
        QScopedPointer<QObject> root(fixture(engine, type, type == "ListItem" ? "type: LV.ListItem.Navigation" : ""));
        QVERIFY(root);
        auto *window = qobject_cast<QQuickWindow *>(root.data());
        auto *control = root->findChild<QQuickItem *>("control");
        QVERIFY(QTest::qWaitForWindowExposed(window));
        control->setProperty("motionEnabled", false);
        root->findChild<QQuickItem *>("next")->forceActiveFocus(Qt::TabFocusReason);
        control->forceActiveFocus(Qt::TabFocusReason);
        QTRY_COMPARE(phase(control), QStringLiteral("focus"));
        QTest::qWait(100);
        const QImage frame = window->grabWindow();
        QVERIFY(!frame.isNull());
        const qreal scale = qreal(frame.width()) / window->width();
        // Count primary-blue pixels above the row: the external ring must render.
        const QPointF top = control->mapToScene(QPointF(control->width() / 2, -2));
        int bluePixels = 0;
        for (int x = qRound((top.x() - 12) * scale); x <= qRound((top.x() + 12) * scale); ++x) {
            for (int y = qRound((top.y() - 1) * scale); y <= qRound(top.y() * scale); ++y) {
                const QColor pixel = frame.pixelColor(x, y);
                if (pixel.blue() > 180 && pixel.blue() > pixel.red() * 2)
                    ++bluePixels;
            }
        }
        QVERIFY2(bluePixels > 5, qPrintable(type + " external focus outline was clipped"));
        const QRectF bounds = control->mapRectToScene(control->boundingRect()).adjusted(-8, -8, 8, 8);
        const QRect crop(qRound(bounds.x() * scale), qRound(bounds.y() * scale),
                         qRound(bounds.width() * scale), qRound(bounds.height() * scale));
        QVERIFY(frame.copy(crop).save(directory + '/' + type + "-focus.png"));
    }
}

QTEST_MAIN(InstanceStateTests)
#include "tst_instance_state.moc"
