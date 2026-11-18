#include "QtEventWait.h"
#include <QGuiApplication>
#include <QMouseEvent>

#include <QCoreApplication>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopedPointer>
#include <QQmlEngine>
#include <QtPlugin>

#include "test_utils.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

namespace {
constexpr int kFlickableStopAtBounds = 0;

void sendMouse(QQuickWindow *window, QEvent::Type type, const QPoint &point,
               Qt::MouseButton button, Qt::MouseButtons buttons)
{
    static ulong timestamp = 10000;
    timestamp += 10;
    const QPointF local(point);
    const QPointF global(window->mapToGlobal(point));
    QMouseEvent event(type, local, local, global, button, buttons, Qt::NoModifier);
    event.setTimestamp(timestamp);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}

void mouseClick(QQuickWindow *window, const QPoint &point)
{
    sendMouse(window, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
}

void mouseDoubleClick(QQuickWindow *window, const QPoint &point)
{
    mouseClick(window, point);
    sendMouse(window, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseButtonDblClick, point, Qt::LeftButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
}

QPoint scenePoint(QQuickItem *item, const QPointF &localPoint)
{
    const QPointF scene = item->mapToScene(localPoint);
    return QPoint(qRound(scene.x()), qRound(scene.y()));
}

bool mouseAreaContainsScenePoint(QObject *root, const QPoint &scenePoint)
{
    const QPointF point(scenePoint);
    const auto items = root->findChildren<QQuickItem *>();
    for (QQuickItem *item : items) {
        const QString className = QString::fromLatin1(item->metaObject()->className());
        if (!className.contains(QStringLiteral("MouseArea")))
            continue;
        if (!item->isVisible() || !item->property("enabled").toBool())
            continue;
        if (item->contains(item->mapFromScene(point)))
            return true;
    }
    return false;
}
}

class CodeEditorTests
{
public:
    void code_editor_default_contract_and_utility_api();
    void code_editor_submit_signal_and_fixed_plain_text_mode();
    void code_editor_ios_native_text_interaction_contract();
    void code_editor_native_event_input_matrix();
    void code_editor_ios_scroll_physics_contract();
};

void CodeEditorTests::code_editor_default_contract_and_utility_api()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 640
    height: 360

    property bool defaultContractReady: editor.snippetTitle === "Snippet"
        && editor.showSnippetHeader
        && editor.wrapMode === TextEdit.NoWrap
        && editor.textFormat === TextEdit.PlainText
        && editor.fontPixelSize === LV.Theme.textBody
        && editor.backgroundColorFocused === editor.backgroundColor
        && editor.backgroundColorDisabled === editor.backgroundColor

    LV.CodeEditor {
        id: editor
        objectName: "codeEditor"
        width: 460
        height: 240
        placeholderText: "Write code"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("defaultContractReady").toBool());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("codeEditor"));
    CTEST_CHECK(editor);
    CTEST_CHECK(editor->property("empty").toBool());

    CTEST_CHECK(QMetaObject::invokeMethod(editor, "insertText", Q_ARG(QVariant, QVariant(QStringLiteral("const value = 1;")))));
    CTEST_COMPARE(editor->property("text").toString(), QStringLiteral("const value = 1;"));
    CTEST_CHECK(!editor->property("empty").toBool());

    CTEST_CHECK(QMetaObject::invokeMethod(editor, "clear"));
    CTEST_COMPARE(editor->property("text").toString(), QString());
    CTEST_CHECK(editor->property("empty").toBool());
}

void CodeEditorTests::code_editor_submit_signal_and_fixed_plain_text_mode()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 640
    height: 420
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    property int submitCount: 0
    property bool fixedModeReady: editor.wrapMode === TextEdit.NoWrap
        && editor.textFormat === TextEdit.PlainText

    LV.CodeEditor {
        id: editor
        objectName: "codeEditor"
        anchors.fill: parent
        text: "let ready = true"
        onSubmitted: root.submitCount += 1
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("fixedModeReady").toBool());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("codeEditor"));
    CTEST_CHECK(editor);
    QObject *textEdit = root->findChild<QObject *>(QStringLiteral("codeTextEdit"));
    CTEST_CHECK(textEdit);

    CTEST_CHECK(QMetaObject::invokeMethod(editor, "forceEditorFocus"));
    const QString beforeReturnText = editor->property("text").toString();
    const int submitBeforeReturn = root->property("submitCount").toInt();
    QKeyEvent returnEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QStringLiteral("\n"));
    QCoreApplication::sendEvent(textEdit, &returnEvent);
    const QString afterReturnText = editor->property("text").toString();
    CTEST_WAIT_COMPARE(afterReturnText.size(), beforeReturnText.size() + 1);
    CTEST_CHECK(afterReturnText.contains(QLatin1Char('\n')));
    CTEST_COMPARE(root->property("submitCount").toInt(), submitBeforeReturn);

    QKeyEvent submitEvent(QEvent::KeyPress, Qt::Key_Return, Qt::ControlModifier, QStringLiteral("\n"));
    QCoreApplication::sendEvent(textEdit, &submitEvent);
    CTEST_WAIT_COMPARE(root->property("submitCount").toInt(), 1);
}

void CodeEditorTests::code_editor_ios_native_text_interaction_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 480
    height: 320

    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    property bool iosNativeTextReady:
        LV.Theme.mobileTarget
        && editor.preferNativeGestures
        && editor.preferNativeTextInteraction
        && editor.editorItem.renderType === TextEdit.NativeRendering
        && editor.editorItem.activeFocusOnPress === editor.autoFocusOnPress
        && !editor.pressed

    LV.CodeEditor {
        id: editor
        width: 320
        height: 180
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("iosNativeTextReady").toBool());
}

void CodeEditorTests::code_editor_native_event_input_matrix()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 460
    height: 260
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.CodeEditor {
        id: editor
        objectName: "codeEditor"
        anchors.fill: parent
        showSnippetHeader: false
        editorHeight: 190
        text: "alpha beta gamma\nsecond line words\nthird paragraph text"

        Component.onCompleted: editor.editorItem.objectName = "codeTextEdit"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    window->show();
    CTEST_WAIT_DEFAULT(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("codeEditor"));
    CTEST_CHECK(editor);
    auto *textEdit = root->findChild<QQuickItem *>(QStringLiteral("codeTextEdit"));
    CTEST_CHECK(textEdit);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("codeEditorViewportFlickable"));
    CTEST_CHECK(viewport);
    CTEST_CHECK(!viewport->property("interactive").toBool());

    const QPoint wordPoint = scenePoint(textEdit, QPointF(26.0, 8.0));
    const QPoint dragStart = scenePoint(textEdit, QPointF(8.0, 8.0));
    const QPoint dragEnd = scenePoint(textEdit, QPointF(128.0, 8.0));

    CTEST_CHECK(!mouseAreaContainsScenePoint(root.data(), wordPoint));

    mouseClick(window, wordPoint);
    CTEST_WAIT_DEFAULT(textEdit->property("activeFocus").toBool());

    textEdit->setProperty("cursorPosition", 0);
    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_Z, Qt::NoModifier, QStringLiteral("z"));
    QCoreApplication::sendEvent(window, &keyPress);
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_Z, Qt::NoModifier, QStringLiteral("z"));
    QCoreApplication::sendEvent(window, &keyRelease);
    CTEST_WAIT_DEFAULT(editor->property("text").toString().startsWith(QStringLiteral("z")));

    QInputMethodEvent preeditEvent(QStringLiteral("preedit"), {});
    QCoreApplication::sendEvent(textEdit, &preeditEvent);
    CTEST_WAIT_COMPARE(textEdit->property("preeditText").toString(), QStringLiteral("preedit"));

    QInputMethodEvent commitEvent;
    commitEvent.setCommitString(QStringLiteral("한"));
    QCoreApplication::sendEvent(textEdit, &commitEvent);
    CTEST_WAIT_DEFAULT(editor->property("text").toString().contains(QStringLiteral("한")));

    textEdit->setProperty("text", QStringLiteral("alpha beta gamma\nsecond line words\nthird paragraph text"));
    textEdit->setProperty("cursorPosition", 0);
    CTEST_CHECK(QMetaObject::invokeMethod(editor, "deselect"));
    mouseDoubleClick(window, wordPoint);
    CTEST_WAIT_DEFAULT(textEdit->property("selectedText").toString().contains(QStringLiteral("alpha")));
    const QString doubleClickSelection = textEdit->property("selectedText").toString();
    CTEST_CHECK(!doubleClickSelection.isEmpty());

    mouseClick(window, wordPoint);
    CTEST_WAIT_DEFAULT(textEdit->property("selectedText").toString() != doubleClickSelection);

    CTEST_CHECK(QMetaObject::invokeMethod(editor, "deselect"));
    sendMouse(window, QEvent::MouseButtonPress, dragStart, Qt::LeftButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseMove, dragEnd, Qt::NoButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseButtonRelease, dragEnd, Qt::LeftButton, Qt::NoButton);
    CTEST_WAIT_DEFAULT(textEdit->property("selectedText").toString().size() > 0);
}

void CodeEditorTests::code_editor_ios_scroll_physics_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 380
    height: 260
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.CodeEditor {
        id: editor
        objectName: "codeEditor"
        anchors.fill: parent
        editorHeight: 120
        text: "line 01\nline 02\nline 03\nline 04\nline 05\nline 06\nline 07\nline 08\nline 09\nline 10\nline 11\nline 12\nline 13\nline 14\nline 15"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("codeEditor"));
    CTEST_CHECK(editor);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("codeEditorViewportFlickable"));
    CTEST_CHECK(viewport);

    CTEST_COMPARE(viewport->property("boundsBehavior").toInt(), kFlickableStopAtBounds);
    CTEST_COMPARE(viewport->property("boundsMovement").toInt(), kFlickableStopAtBounds);
    CTEST_COMPARE(viewport->property("boundsBehavior").toInt(), editor->property("viewportBoundsBehavior").toInt());
    CTEST_COMPARE(viewport->property("boundsMovement").toInt(), editor->property("viewportBoundsMovement").toInt());
    CTEST_COMPARE(viewport->property("flickDeceleration").toInt(), editor->property("viewportFlickDeceleration").toInt());
    CTEST_COMPARE(viewport->property("maximumFlickVelocity").toInt(), editor->property("viewportMaximumFlickVelocity").toInt());
    CTEST_CHECK(!viewport->property("interactive").toBool());
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    CodeEditorTests tests;
    runner.run("code_editor_default_contract_and_utility_api", [&] { tests.code_editor_default_contract_and_utility_api(); });
    runner.run("code_editor_submit_signal_and_fixed_plain_text_mode", [&] { tests.code_editor_submit_signal_and_fixed_plain_text_mode(); });
    runner.run("code_editor_ios_native_text_interaction_contract", [&] { tests.code_editor_ios_native_text_interaction_contract(); });
    runner.run("code_editor_native_event_input_matrix", [&] { tests.code_editor_native_event_input_matrix(); });
    runner.run("code_editor_ios_scroll_physics_contract", [&] { tests.code_editor_ios_scroll_physics_contract(); });
    return runner.result();
}
