#include <QtTest>

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

class CodeEditorTests : public QObject
{
    Q_OBJECT

private slots:
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
    QVERIFY(root);
    QVERIFY(root->property("defaultContractReady").toBool());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("codeEditor"));
    QVERIFY(editor);
    QVERIFY(editor->property("empty").toBool());

    QVERIFY(QMetaObject::invokeMethod(editor, "insertText", Q_ARG(QVariant, QVariant(QStringLiteral("const value = 1;")))));
    QCOMPARE(editor->property("text").toString(), QStringLiteral("const value = 1;"));
    QVERIFY(!editor->property("empty").toBool());

    QVERIFY(QMetaObject::invokeMethod(editor, "clear"));
    QCOMPARE(editor->property("text").toString(), QString());
    QVERIFY(editor->property("empty").toBool());
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
    QVERIFY(root);
    QVERIFY(root->property("fixedModeReady").toBool());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("codeEditor"));
    QVERIFY(editor);
    QObject *textEdit = root->findChild<QObject *>(QStringLiteral("codeTextEdit"));
    QVERIFY(textEdit);

    QVERIFY(QMetaObject::invokeMethod(editor, "forceEditorFocus"));
    const QString beforeReturnText = editor->property("text").toString();
    const int submitBeforeReturn = root->property("submitCount").toInt();
    QKeyEvent returnEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QStringLiteral("\n"));
    QCoreApplication::sendEvent(textEdit, &returnEvent);
    const QString afterReturnText = editor->property("text").toString();
    QTRY_COMPARE(afterReturnText.size(), beforeReturnText.size() + 1);
    QVERIFY(afterReturnText.contains(QLatin1Char('\n')));
    QCOMPARE(root->property("submitCount").toInt(), submitBeforeReturn);

    QKeyEvent submitEvent(QEvent::KeyPress, Qt::Key_Return, Qt::ControlModifier, QStringLiteral("\n"));
    QCoreApplication::sendEvent(textEdit, &submitEvent);
    QTRY_COMPARE(root->property("submitCount").toInt(), 1);
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("iosNativeTextReady").toBool());
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
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("codeEditor"));
    QVERIFY(editor);
    auto *textEdit = root->findChild<QQuickItem *>(QStringLiteral("codeTextEdit"));
    QVERIFY(textEdit);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("codeEditorViewportFlickable"));
    QVERIFY(viewport);
    QVERIFY(!viewport->property("interactive").toBool());

    const QPoint wordPoint = scenePoint(textEdit, QPointF(26.0, 8.0));
    const QPoint dragStart = scenePoint(textEdit, QPointF(8.0, 8.0));
    const QPoint dragEnd = scenePoint(textEdit, QPointF(128.0, 8.0));

    QVERIFY(!mouseAreaContainsScenePoint(root.data(), wordPoint));

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, wordPoint, 10);
    QTRY_VERIFY(textEdit->property("activeFocus").toBool());

    textEdit->setProperty("cursorPosition", 0);
    QTest::keyClick(window, Qt::Key_Z, Qt::NoModifier, 10);
    QTRY_VERIFY(editor->property("text").toString().startsWith(QStringLiteral("z")));

    QInputMethodEvent preeditEvent(QStringLiteral("preedit"), {});
    QCoreApplication::sendEvent(textEdit, &preeditEvent);
    QTRY_COMPARE(textEdit->property("preeditText").toString(), QStringLiteral("preedit"));

    QInputMethodEvent commitEvent;
    commitEvent.setCommitString(QStringLiteral("한"));
    QCoreApplication::sendEvent(textEdit, &commitEvent);
    QTRY_VERIFY(editor->property("text").toString().contains(QStringLiteral("한")));

    textEdit->setProperty("text", QStringLiteral("alpha beta gamma\nsecond line words\nthird paragraph text"));
    textEdit->setProperty("cursorPosition", 0);
    QVERIFY(QMetaObject::invokeMethod(editor, "deselect"));
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, wordPoint, 10);
    QTRY_VERIFY(textEdit->property("selectedText").toString().contains(QStringLiteral("alpha")));
    const QString doubleClickSelection = textEdit->property("selectedText").toString();
    QVERIFY(!doubleClickSelection.isEmpty());

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, wordPoint, 10);
    QTRY_VERIFY(textEdit->property("selectedText").toString() != doubleClickSelection);

    QVERIFY(QMetaObject::invokeMethod(editor, "deselect"));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, dragStart, 10);
    QTest::mouseMove(window, dragEnd, 10);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, dragEnd, 10);
    QTRY_VERIFY(textEdit->property("selectedText").toString().size() > 0);
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
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("codeEditor"));
    QVERIFY(editor);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("codeEditorViewportFlickable"));
    QVERIFY(viewport);

    QCOMPARE(viewport->property("boundsBehavior").toInt(), kFlickableStopAtBounds);
    QCOMPARE(viewport->property("boundsMovement").toInt(), kFlickableStopAtBounds);
    QCOMPARE(viewport->property("boundsBehavior").toInt(), editor->property("viewportBoundsBehavior").toInt());
    QCOMPARE(viewport->property("boundsMovement").toInt(), editor->property("viewportBoundsMovement").toInt());
    QCOMPARE(viewport->property("flickDeceleration").toInt(), editor->property("viewportFlickDeceleration").toInt());
    QCOMPARE(viewport->property("maximumFlickVelocity").toInt(), editor->property("viewportMaximumFlickVelocity").toInt());
    QVERIFY(!viewport->property("interactive").toBool());
}

QTEST_MAIN(CodeEditorTests)
#include "tst_code_editor.moc"
