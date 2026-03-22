#include <QtTest>

#include <QCoreApplication>
#include <QKeyEvent>
#include <QScopedPointer>
#include <QQmlEngine>
#include <QtPlugin>

#include "test_utils.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class CodeEditorTests : public QObject
{
    Q_OBJECT

private slots:
    void code_editor_default_contract_and_utility_api();
    void code_editor_submit_signal_and_fixed_plain_text_mode();
    void code_editor_ios_native_text_interaction_contract();
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

QTEST_MAIN(CodeEditorTests)
#include "tst_code_editor.moc"
