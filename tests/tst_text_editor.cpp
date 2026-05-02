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

bool enabledObjectClassContains(QObject *root, const QString &classNameFragment)
{
    const auto objects = root->findChildren<QObject *>();
    for (QObject *object : objects) {
        const QString className = QString::fromLatin1(object->metaObject()->className());
        if (!className.contains(classNameFragment))
            continue;
        const QVariant enabled = object->property("enabled");
        if (enabled.isValid() && enabled.toBool())
            return true;
    }
    return false;
}
}

class TextEditorTests : public QObject
{
    Q_OBJECT

private slots:
    void text_editor_default_contract_and_utility_api();
    void text_editor_mode_independent_render_contract_and_submit_signal();
    void text_editor_ios_native_text_interaction_contract();
    void text_editor_native_event_input_matrix();
    void text_editor_ios_scroll_physics_contract();
    void text_editor_mobile_focus_suspends_viewport_flick_for_selection();
};

void TextEditorTests::text_editor_default_contract_and_utility_api()
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

    property bool defaultModeReady: editor.mode === editor.plainTextMode
        && editor.effectiveWrapMode === TextEdit.Wrap
        && editor.effectiveTextFormat === TextEdit.PlainText
        && editor.enforceModeDefaults
        && editor.backgroundColorFocused === editor.backgroundColor
        && editor.backgroundColorDisabled === editor.backgroundColor
    property bool bodyTokenReady: editor.fontPixelSize === LV.Theme.textBody
        && editor.fontWeight === LV.Theme.textBodyWeight
        && editor.textLineHeight === LV.Theme.textBodyLineHeight

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        width: 420
        height: 220
        placeholderText: "Write here"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("defaultModeReady").toBool());
    QVERIFY(root->property("bodyTokenReady").toBool());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QVERIFY(editor->property("empty").toBool());
    QVERIFY(editor->property("enforceModeDefaults").toBool());

    QVERIFY(QMetaObject::invokeMethod(editor, "insertText", Q_ARG(QVariant, QVariant(QStringLiteral("hello")))));
    QCOMPARE(editor->property("text").toString(), QStringLiteral("hello"));
    QVERIFY(!editor->property("empty").toBool());
    QCOMPARE(editor->property("normalizedInput").toString(), QStringLiteral("hello"));
    QCOMPARE(editor->property("renderedPlainText").toString(), QStringLiteral("hello"));
    QCOMPARE(editor->property("renderedOutput").toString(), QStringLiteral("hello"));

    QVERIFY(QMetaObject::invokeMethod(editor, "clear"));
    QCOMPARE(editor->property("text").toString(), QString());
    QVERIFY(editor->property("empty").toBool());
}

void TextEditorTests::text_editor_mode_independent_render_contract_and_submit_signal()
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
    property bool markdownModeReady: editor.mode === editor.markdownMode
        && editor.effectiveWrapMode === TextEdit.Wrap
        && editor.effectiveTextFormat === TextEdit.PlainText
    property bool richModeReady: editor.mode === editor.richTextMode
        && editor.effectiveWrapMode === TextEdit.Wrap
        && editor.effectiveTextFormat === TextEdit.PlainText

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        anchors.fill: parent
        text: "Hello **bold**"
        onSubmitted: root.submitCount += 1
    }

    function setPlainMode() { editor.mode = editor.plainTextMode }
    function setMarkdownMode() { editor.mode = editor.markdownMode }
    function setRichMode() { editor.mode = editor.richTextMode }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *textEdit = root->findChild<QObject *>(QStringLiteral("editorTextEdit"));
    QVERIFY(textEdit);

    const QString plainRendered = editor->property("renderedOutput").toString();
    const QString plainNormalized = editor->property("normalizedInput").toString();

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setMarkdownMode"));
    QVERIFY(root->property("markdownModeReady").toBool());
    QCOMPARE(editor->property("renderedOutput").toString(), plainRendered);
    QCOMPARE(editor->property("normalizedInput").toString(), plainNormalized);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setRichMode"));
    QVERIFY(root->property("richModeReady").toBool());
    QCOMPARE(editor->property("renderedOutput").toString(), plainRendered);
    QCOMPARE(editor->property("normalizedInput").toString(), plainNormalized);

    editor->setProperty("text", QStringLiteral("<strong>Hello</strong> **bold**"));
    const QString htmlMixedRendered = editor->property("renderedOutput").toString();
    const QString htmlMixedNormalized = editor->property("normalizedInput").toString();

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setPlainMode"));
    QCOMPARE(editor->property("renderedOutput").toString(), htmlMixedRendered);
    QCOMPARE(editor->property("normalizedInput").toString(), htmlMixedNormalized);

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

void TextEditorTests::text_editor_ios_native_text_interaction_contract()
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
        && editor.submitShortcutEnabled
        && !editor.effectiveSubmitShortcutEnabled
        && editor.editorItem.Keys.priority === Keys.AfterItem
        && !editor.pressed

    LV.TextEditor {
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

void TextEditorTests::text_editor_native_event_input_matrix()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 420
    height: 240
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0
    property int submitCount: 0

    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        anchors.fill: parent
        showRenderedOutput: false
        editorHeight: 180
        text: "alpha beta gamma\nsecond line words\nthird paragraph text"
        onSubmitted: root.submitCount += 1

        Component.onCompleted: editor.editorItem.objectName = "editorTextEdit"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    auto *textEdit = root->findChild<QQuickItem *>(QStringLiteral("editorTextEdit"));
    QVERIFY(textEdit);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("editorViewportFlickable"));
    QVERIFY(viewport);
    QVERIFY(!viewport->property("interactive").toBool());

    const qreal leftPadding = textEdit->property("leftPadding").toReal();
    const qreal topPadding = textEdit->property("topPadding").toReal();
    const QPoint wordPoint = scenePoint(textEdit, QPointF(leftPadding + 26.0, topPadding + 8.0));
    const QPoint dragStart = scenePoint(textEdit, QPointF(leftPadding + 8.0, topPadding + 8.0));
    const QPoint dragEnd = scenePoint(textEdit, QPointF(leftPadding + 128.0, topPadding + 8.0));

    QVERIFY(!mouseAreaContainsScenePoint(root.data(), wordPoint));
    QVERIFY(!enabledObjectClassContains(root.data(), QStringLiteral("HoverHandler")));

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

    const int submitBeforeShortcut = root->property("submitCount").toInt();
    textEdit->setProperty("cursorPosition", textEdit->property("text").toString().size());
    QTest::keyClick(window, Qt::Key_Return, Qt::ControlModifier, 10);
    QTRY_COMPARE(root->property("submitCount").toInt(), submitBeforeShortcut);

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

void TextEditorTests::text_editor_ios_scroll_physics_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 360
    height: 280
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        anchors.fill: parent
        showRenderedOutput: true
        editorHeight: 96
        outputMinHeight: 64
        text: "line 01\nline 02\nline 03\nline 04\nline 05\nline 06\nline 07\nline 08\nline 09\nline 10\nline 11\nline 12\nline 13\nline 14\nline 15"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *editorViewport = root->findChild<QObject *>(QStringLiteral("editorViewportFlickable"));
    QVERIFY(editorViewport);
    QObject *previewViewport = root->findChild<QObject *>(QStringLiteral("editorPreviewFlickable"));
    QVERIFY(previewViewport);

    QCOMPARE(editorViewport->property("boundsBehavior").toInt(), kFlickableStopAtBounds);
    QCOMPARE(editorViewport->property("boundsMovement").toInt(), kFlickableStopAtBounds);
    QCOMPARE(editorViewport->property("boundsBehavior").toInt(), editor->property("viewportBoundsBehavior").toInt());
    QCOMPARE(editorViewport->property("boundsMovement").toInt(), editor->property("viewportBoundsMovement").toInt());
    QCOMPARE(editorViewport->property("flickDeceleration").toInt(), editor->property("viewportFlickDeceleration").toInt());
    QCOMPARE(editorViewport->property("maximumFlickVelocity").toInt(), editor->property("viewportMaximumFlickVelocity").toInt());

    QCOMPARE(previewViewport->property("boundsBehavior").toInt(), kFlickableStopAtBounds);
    QCOMPARE(previewViewport->property("boundsMovement").toInt(), kFlickableStopAtBounds);
    QCOMPARE(previewViewport->property("flickDeceleration").toInt(), editor->property("viewportFlickDeceleration").toInt());
    QCOMPARE(previewViewport->property("maximumFlickVelocity").toInt(), editor->property("viewportMaximumFlickVelocity").toInt());
}

void TextEditorTests::text_editor_mobile_focus_suspends_viewport_flick_for_selection()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 360
    height: 240
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        anchors.fill: parent
        showRenderedOutput: false
        editorHeight: 96
        text: "line 01\nline 02\nline 03\nline 04\nline 05\nline 06\nline 07\nline 08\nline 09\nline 10\nline 11\nline 12"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *textEdit = root->findChild<QObject *>(QStringLiteral("editorTextEdit"));
    QVERIFY(textEdit);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("editorViewportFlickable"));
    QVERIFY(viewport);

    QTRY_VERIFY(viewport->property("contentHeight").toReal() > viewport->property("height").toReal());
    QCOMPARE(viewport->property("flickableDirection").toInt(), 2);
    QVERIFY(!viewport->property("interactive").toBool());
    QVERIFY(!textEdit->property("activeFocus").toBool());

    QVERIFY(QMetaObject::invokeMethod(editor, "forceEditorFocus"));
    QTRY_VERIFY(textEdit->property("activeFocus").toBool());
    QTRY_VERIFY(!viewport->property("interactive").toBool());
}

QTEST_MAIN(TextEditorTests)
#include "tst_text_editor.moc"
