#include "QtEventWait.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <type_traits>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include "test_utils.h"

class InstanceStateSignalRecorder : public QObject
{
    Q_OBJECT
public:
    int count = 0;
public slots:
    void recordClicked() { ++count; }
};

namespace {
struct LifecycleVariant { QString tag; QString type; QString configuration; };
QList<LifecycleVariant> lifecycleVariants()
{
    QList<LifecycleVariant> result;
    for (const char *type : {"PushButton", "DropdownButton", "IconButton", "IconMenuButton",
                             "MenuItem", "ContextMenuItem", "HierarchyItem"})
        result.append({QString::fromLatin1(type), QString::fromLatin1(type), {}});
    for (int type = 0; type < 17; ++type)
        result.append({QStringLiteral("ListItem-") + QString::number(type),
                       QStringLiteral("ListItem"), QStringLiteral("type: %1").arg(type)});
    return result;
}

template <typename Actual, typename Expected>
bool stateEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}

Qt::MouseButtons pressedButtons = Qt::NoButton;
void sendMouse(QQuickWindow *window, QEvent::Type type, const QPoint &point, Qt::MouseButton button)
{
    if (type == QEvent::MouseButtonPress) pressedButtons |= button;
    if (type == QEvent::MouseButtonRelease) pressedButtons &= ~Qt::MouseButtons(button);
    const QPointF local(point), global(window->mapToGlobal(point));
    static ulong timestamp = 10000;
    QMouseEvent event(type, local, local, global, button, pressedButtons, Qt::NoModifier);
    event.setTimestamp(timestamp += 16);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}
void mouseMove(QQuickWindow *window, const QPoint &point) { sendMouse(window, QEvent::MouseMove, point, Qt::NoButton); }
void mousePress(QQuickWindow *window, const QPoint &point) { sendMouse(window, QEvent::MouseButtonPress, point, Qt::LeftButton); }
void mouseRelease(QQuickWindow *window, const QPoint &point) { sendMouse(window, QEvent::MouseButtonRelease, point, Qt::LeftButton); }
void mouseClick(QQuickWindow *window, const QPoint &point) { mousePress(window, point); mouseRelease(window, point); }
void sendKey(QQuickWindow *window, QEvent::Type type, Qt::Key key)
{
    const QString text = key == Qt::Key_Space ? QStringLiteral(" ") : QString();
    QKeyEvent event(type, key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}
void keyClick(QQuickWindow *window, Qt::Key key) { sendKey(window, QEvent::KeyPress, key); sendKey(window, QEvent::KeyRelease, key); }
}
#define STATE_COMPARE(actual, expected) CTEST_CHECK(stateEqual((actual), (expected)))
#define STATE_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(stateEqual((actual), (expected)))

class InstanceStateTests
{
public:
    void owned_input_lifecycle(const QString &type, const QString &configuration);
    void cancellation_and_motion_preferences();
    void model_states_remain_independent();
    void menu_selected_uses_press_fill(bool expanded, const QString &primary);
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

void InstanceStateTests::owned_input_lifecycle(const QString &type, const QString &configuration)
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine, type, configuration));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    auto *other = root->findChild<QQuickItem *>("other");
    auto *next = root->findChild<QQuickItem *>("next");
    CTEST_CHECK(control && other && next);
    auto *state = control->property("interaction").value<QObject *>();
    auto *otherState = other->property("interaction").value<QObject *>();
    CTEST_CHECK_MESSAGE(state && otherState, "Each component must own its interaction state object");
    CTEST_CHECK(state != otherState);
    STATE_COMPARE(state, control->findChild<QObject *>("controlInstanceState"));
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    next->forceActiveFocus(Qt::MouseFocusReason);
    mouseMove(window, QPoint(650, 350));
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("default"));
    STATE_COMPARE(control->property("interactionInput").toString(), QStringLiteral("pointer"));
    const QRectF geometry = control->mapRectToScene(control->boundingRect());
    // The leading edge activates the row without stealing embedded accessory input.
    const QPoint point = control->mapToScene(QPointF(2, control->height() / 2)).toPoint();
    InstanceStateSignalRecorder clicks;
    const auto clickedConnection = QObject::connect(control, SIGNAL(clicked()), &clicks, SLOT(recordClicked()));
    CTEST_CHECK(clickedConnection);
    mouseMove(window, point);
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("hover"));
    STATE_COMPARE(phase(other), QStringLiteral("default"));
    mousePress(window, point);
    STATE_COMPARE(phase(control), QStringLiteral("press"));
    STATE_COMPARE(clicks.count, 0);
    mouseRelease(window, point);
    STATE_COMPARE(phase(control), QStringLiteral("release"));
    STATE_COMPARE(clicks.count, 1);
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("hover"));
    mouseMove(window, QPoint(650, 350));
    next->forceActiveFocus(Qt::TabFocusReason);
    control->forceActiveFocus(Qt::TabFocusReason);
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("focus"));
    STATE_COMPARE(control->property("interactionInput").toString(), QStringLiteral("keyboard"));
    CTEST_CHECK(state->property("focusVisible").toBool());
    auto *ring = control->findChild<QQuickItem *>("buttonFocusRing");
    CTEST_CHECK(ring && ring->property("active").toBool());
    // The row must not clip its own 3px external focus outline.
    CTEST_CHECK(!control->clip());
    for (Qt::Key key : {Qt::Key_Space, Qt::Key_Return, Qt::Key_Enter}) {
        const int before = clicks.count;
        sendKey(window, QEvent::KeyPress, key);
        STATE_COMPARE(phase(control), QStringLiteral("press"));
        CTEST_CHECK(state->property("focusVisible").toBool());
        STATE_COMPARE(clicks.count, before);
        sendKey(window, QEvent::KeyRelease, key);
        STATE_COMPARE(phase(control), QStringLiteral("release"));
        STATE_COMPARE(clicks.count, before + 1);
        CTEST_CHECK(ring->property("active").toBool());
        STATE_WAIT_COMPARE(phase(control), QStringLiteral("focus"));
    }
    STATE_COMPARE(control->mapRectToScene(control->boundingRect()), geometry);
    STATE_COMPARE(phase(other), QStringLiteral("default"));
    // Enter completion must restore native pointer tracking on this instance.
    mouseMove(window, point);
    mousePress(window, point);
    STATE_COMPARE(phase(control), QStringLiteral("press"));
    mouseRelease(window, point);
    STATE_COMPARE(phase(control), QStringLiteral("release"));
    control->setEnabled(false);
    STATE_COMPARE(phase(control), QStringLiteral("disabled"));
    CTEST_CHECK(!state->property("focusVisible").toBool());
    const int before = clicks.count;
    mouseClick(window, point);
    STATE_COMPARE(clicks.count, before);
}

void InstanceStateTests::cancellation_and_motion_preferences()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine, QStringLiteral("PushButton")));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    auto *next = root->findChild<QQuickItem *>("next");
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    CTEST_CHECK(control->property("interaction").value<QObject *>());
    const QPoint center = control->mapToScene(control->boundingRect().center()).toPoint();
    InstanceStateSignalRecorder clicks;
    const auto clickedConnection = QObject::connect(control, SIGNAL(clicked()), &clicks, SLOT(recordClicked()));
    CTEST_CHECK(clickedConnection);
    mousePress(window, center);
    mouseMove(window, QPoint(650, 350));
    mouseRelease(window, QPoint(650, 350));
    STATE_COMPARE(clicks.count, 0);
    CTEST_CHECK(phase(control) != QStringLiteral("release"));
    control->forceActiveFocus(Qt::TabFocusReason);
    sendKey(window, QEvent::KeyPress, Qt::Key_Return);
    next->forceActiveFocus(Qt::TabFocusReason);
    sendKey(window, QEvent::KeyRelease, Qt::Key_Return);
    STATE_COMPARE(clicks.count, 0);
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("default"));
    // Re-enter the control before clicking so the native hover state follows the pointer.
    mouseMove(window, center);
    mouseClick(window, center);
    STATE_COMPARE(phase(control), QStringLiteral("release"));
    mousePress(window, center);
    STATE_COMPARE(phase(control), QStringLiteral("press"));
    auto *motion = control->findChild<QObject *>("interactionMotion");
    CTEST_CHECK(motion && !motion->property("releasing").toBool());
    mouseRelease(window, center);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "reduceMotion"));
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("hover"));
    mousePress(window, center);
    STATE_COMPARE(phase(control), QStringLiteral("press"));
    mouseRelease(window, center);
    STATE_COMPARE(phase(control), QStringLiteral("hover"));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "restoreMotion"));
    control->setProperty("motionEnabled", false);
    mouseClick(window, center);
    STATE_COMPARE(phase(control), QStringLiteral("hover"));
}

void InstanceStateTests::model_states_remain_independent()
{
    for (const QString &type : {QStringLiteral("ListItem"), QStringLiteral("MenuItem"),
                                QStringLiteral("ContextMenuItem"), QStringLiteral("HierarchyItem")}) {
        const bool menu = type.contains(QStringLiteral("MenuItem"));
        QQmlEngine engine;
        QScopedPointer<QObject> root(fixture(engine, type, menu ? "state: selectedState" : "selected: true"));
        CTEST_CHECK(root);
        auto *window = qobject_cast<QQuickWindow *>(root.data());
        auto *control = root->findChild<QQuickItem *>("control");
        CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
        CTEST_CHECK(control->property("interaction").value<QObject *>());
        root->findChild<QQuickItem *>("next")->forceActiveFocus(Qt::TabFocusReason);
        control->forceActiveFocus(Qt::TabFocusReason);
        STATE_WAIT_COMPARE(phase(control), QStringLiteral("focus"));
        keyClick(window, Qt::Key_Return);
        STATE_COMPARE(phase(control), QStringLiteral("release"));
        if (menu) {
            STATE_COMPARE(control->property("state").toInt(), control->property("selectedState").toInt());
            control->setProperty("state", control->property("inactiveState"));
        } else if (type == "HierarchyItem") {
            CTEST_CHECK(control->property("selected").toBool());
            control->setProperty("dragPreviewActive", true);
            STATE_COMPARE(phase(control), QStringLiteral("disabled"));
            STATE_COMPARE(control->property("uxStateName").toString(), QStringLiteral("Drag"));
            control->setProperty("dragPreviewActive", false);
            control->setProperty("activatable", false);
        } else {
            CTEST_CHECK(control->property("selected").toBool());
            control->setEnabled(false);
        }
        STATE_COMPARE(phase(control), QStringLiteral("disabled"));
        CTEST_CHECK(!control->property("interaction").value<QObject *>()->property("focusVisible").toBool());
    }
}

void InstanceStateTests::menu_selected_uses_press_fill(bool expanded, const QString &primary)
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine, "MenuItem",
        QStringLiteral("expanded: %1; motionEnabled: false").arg(expanded ? "true" : "false")));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    auto *other = root->findChild<QQuickItem *>("other");
    auto *next = root->findChild<QQuickItem *>("next");
    CTEST_CHECK(control && other && next);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    QScopedPointer<QObject> tokens(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; QtObject { property var theme: LV.Theme }"));
    CTEST_CHECK(tokens);
    auto *theme = tokens->property("theme").value<QObject *>();
    CTEST_CHECK(theme);
    CTEST_CHECK(theme->setProperty("primaryColor", QColor(primary)));
    const QColor expected = other->property("backgroundColorPressed").value<QColor>();
    STATE_COMPARE(expected, theme->property("accentMuted").value<QColor>());
    auto *background = control->property("background").value<QQuickItem *>();
    CTEST_CHECK(background);
    CTEST_CHECK(control->setProperty("state", control->property("selectedState")));
    const auto verifyFill = [&] {
        STATE_COMPARE(background->property("color").value<QColor>(), expected);
        const QImage frame = window->grabWindow();
        CTEST_CHECK(!frame.isNull());
        const QPointF center = control->mapToScene(control->boundingRect().center()) * (qreal(frame.width()) / window->width());
        const QColor pixel = frame.pixelColor(qRound(center.x()), qRound(center.y()));
        CTEST_CHECK_MESSAGE(qAbs(pixel.red() - expected.red()) <= 2
                 && qAbs(pixel.green() - expected.green()) <= 2
                 && qAbs(pixel.blue() - expected.blue()) <= 2,
                 qPrintable(QString("Selected rendered %1; press uses %2").arg(pixel.name(), expected.name())));
    };
    next->forceActiveFocus(Qt::MouseFocusReason);
    mouseMove(window, QPoint(650, 350));
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("default"));
    verifyFill();
    const QPoint point = control->mapToScene(QPointF(2, control->height() / 2)).toPoint();
    mouseMove(window, point);
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("hover"));
    verifyFill();
    control->setProperty("motionEnabled", true);
    mousePress(window, point);
    STATE_COMPARE(phase(control), QStringLiteral("press"));
    verifyFill();
    mouseRelease(window, point);
    STATE_COMPARE(phase(control), QStringLiteral("release"));
    verifyFill();
    mouseMove(window, QPoint(650, 350));
    next->forceActiveFocus(Qt::TabFocusReason);
    control->forceActiveFocus(Qt::TabFocusReason);
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("focus"));
    verifyFill();
    sendKey(window, QEvent::KeyPress, Qt::Key_Return);
    STATE_COMPARE(phase(control), QStringLiteral("press"));
    verifyFill();
    sendKey(window, QEvent::KeyRelease, Qt::Key_Return);
    STATE_COMPARE(phase(control), QStringLiteral("release"));
    verifyFill();
    STATE_COMPARE(control->property("state").toInt(), control->property("selectedState").toInt());
    QScopedPointer<QObject> compact(TestUtils::createFromQml(engine,
        "import LVRS as LV; LV.ContextMenuItem { state: selectedState }"));
    CTEST_CHECK(compact);
    STATE_COMPARE(compact->property("resolvedBackgroundColor").value<QColor>(), QColor(primary));
}

void InstanceStateTests::recorded_figma_contract()
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-instance-state.json"));
    CTEST_CHECK(file.open(QIODevice::ReadOnly));
    QJsonParseError error;
    const QJsonObject contract = QJsonDocument::fromJson(file.readAll(), &error).object();
    STATE_COMPARE(error.error, QJsonParseError::NoError);
    STATE_COMPARE(contract.value("fileKey").toString(), QStringLiteral("0GkItQYSNIR0lZ3iJhfJzc"));
    STATE_COMPARE(contract.value("phases").toArray(), QJsonArray({"default", "hover", "press", "release", "focus"}));
    const QJsonArray sets = contract.value("sets").toArray();
    STATE_COMPARE(sets.size(), 25);
    int variants = 0;
    QSet<QString> ids;
    for (const QJsonValue &value : sets) {
        const QJsonObject set = value.toObject();
        const int count = set.value("count").toInt();
        CTEST_CHECK(count > 0 && count <= 30);
        STATE_COMPARE(set.value("boundVariantCount").toInt(), count);
        STATE_COMPARE(set.value("verifiedInstanceOverrides").toInt(), count);
        CTEST_CHECK(!ids.contains(set.value("id").toString()));
        ids.insert(set.value("id").toString());
        variants += count;
    }
    STATE_COMPARE(variants, 740);
    STATE_COMPARE(contract.value("remainingOverlayInstances").toInt(-1), 0);
    STATE_COMPARE(contract.value("removedOverlaySets").toArray().size(), 5);
    STATE_COMPARE(contract.value("auditedPages").toInt(), 23);
    STATE_COMPARE(contract.value("migratedInstances").toInt(), 539);
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    CTEST_CHECK(control->property("pointerDragRequiresLongPress").toBool());
    const QPoint center = control->mapToScene(control->boundingRect().center()).toPoint();
    mousePress(window, center);
    STATE_COMPARE(phase(control), QStringLiteral("press"));
    CTEST_CHECK(!control->property("down").toBool());
    mouseRelease(window, center);
    STATE_COMPARE(phase(control), QStringLiteral("release"));
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("default"));
    root->findChild<QQuickItem *>("next")->forceActiveFocus(Qt::TabFocusReason);
    control->forceActiveFocus(Qt::TabFocusReason);
    STATE_WAIT_COMPARE(phase(control), QStringLiteral("focus"));
    for (Qt::Key key : {Qt::Key_Space, Qt::Key_Return}) {
        sendKey(window, QEvent::KeyPress, key);
        STATE_COMPARE(phase(control), QStringLiteral("press"));
        sendKey(window, QEvent::KeyRelease, key);
        STATE_COMPARE(phase(control), QStringLiteral("release"));
        STATE_WAIT_COMPARE(phase(control), QStringLiteral("focus"));
    }
    control->setProperty("dragPreviewActive", true);
    STATE_COMPARE(phase(control), QStringLiteral("disabled"));
}

void InstanceStateTests::native_focus_capture()
{
    const QString directory = qEnvironmentVariable("LVRS_STATE_CAPTURE_DIR");
    if (directory.isEmpty())
        lvrs_ctest::skip("Set LVRS_STATE_CAPTURE_DIR for rendered state captures.");
    CTEST_CHECK(QDir().mkpath(directory));
    for (const QString &type : {QStringLiteral("PushButton"), QStringLiteral("DropdownButton"),
                                QStringLiteral("ListItem"), QStringLiteral("MenuItem"),
                                QStringLiteral("ContextMenuItem"), QStringLiteral("HierarchyItem")}) {
        QQmlEngine engine;
        QScopedPointer<QObject> root(fixture(engine, type, type == "ListItem" ? "type: LV.ListItem.Navigation" : ""));
        CTEST_CHECK(root);
        auto *window = qobject_cast<QQuickWindow *>(root.data());
        auto *control = root->findChild<QQuickItem *>("control");
        CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
        window->requestActivate();
        CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isActive(); }, 5000));
        control->setProperty("motionEnabled", false);
        root->findChild<QQuickItem *>("next")->forceActiveFocus(Qt::TabFocusReason);
        control->forceActiveFocus(Qt::TabFocusReason);
        STATE_WAIT_COMPARE(phase(control), QStringLiteral("focus"));
        QImage frame;
        const auto hasFocusOutline = [&] {
            frame = window->grabWindow();
            if (frame.isNull()) return false;
            const qreal scale = qreal(frame.width()) / window->width();
            const QPointF top = control->mapToScene(QPointF(control->width() / 2, -2));
            int bluePixels = 0;
            for (int x = qRound((top.x() - 12) * scale); x <= qRound((top.x() + 12) * scale); ++x) {
                for (int y = qRound((top.y() - 1) * scale); y <= qRound(top.y() * scale); ++y) {
                    const QColor pixel = frame.pixelColor(x, y);
                    if (pixel.blue() > 180 && pixel.blue() > pixel.red() * 2)
                        ++bluePixels;
                }
            }
            return bluePixels > 5;
        };
        CTEST_CHECK_MESSAGE(lvrs_ctest::waitFor([&] { return hasFocusOutline(); }, 5000), qPrintable(type + " external focus outline was clipped"));
        CTEST_CHECK(frame.save(directory + '/' + type + "-window.png"));
        const qreal scale = qreal(frame.width()) / window->width();
        const QRectF bounds = control->mapRectToScene(control->boundingRect()).adjusted(-8, -8, 8, 8);
        const QRect crop(qRound(bounds.x() * scale), qRound(bounds.y() * scale),
                         qRound(bounds.width() * scale), qRound(bounds.height() * scale));
        CTEST_CHECK(frame.copy(crop).save(directory + '/' + type + "-focus.png"));
    }
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    InstanceStateTests tests;
    for (const LifecycleVariant &variant : lifecycleVariants()) {
        const auto name = QStringLiteral("owned_input_lifecycle:") + variant.tag;
        runner.run(name.toStdString(), [&] { pressedButtons = Qt::NoButton; tests.owned_input_lifecycle(variant.type, variant.configuration); });
    }
    for (bool expanded : {false, true}) {
        for (const QString &primary : {QStringLiteral("#0A84FF"), QStringLiteral("#A571E6")}) {
            const auto name = QStringLiteral("menu_selected_uses_press_fill:")
                + (expanded ? QStringLiteral("expanded-") : QStringLiteral("collapsed-")) + primary;
            runner.run(name.toStdString(), [&] { pressedButtons = Qt::NoButton; tests.menu_selected_uses_press_fill(expanded, primary); });
        }
    }
    runner.run("cancellation_and_motion_preferences", [&] { pressedButtons = Qt::NoButton; tests.cancellation_and_motion_preferences(); });
    runner.run("model_states_remain_independent", [&] { pressedButtons = Qt::NoButton; tests.model_states_remain_independent(); });
    runner.run("mobile_hierarchy_input", [&] { pressedButtons = Qt::NoButton; tests.mobile_hierarchy_input(); });
    runner.run("native_focus_capture", [&] { pressedButtons = Qt::NoButton; tests.native_focus_capture(); });
    runner.run("recorded_figma_contract", [&] { pressedButtons = Qt::NoButton; tests.recorded_figma_contract(); });
    return runner.result();
}
#include "tst_instance_state.moc"
