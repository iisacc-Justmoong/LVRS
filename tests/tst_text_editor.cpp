#include <QtTest>

#include <QCoreApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopedPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QTemporaryDir>
#include <QQmlEngine>
#include <QtPlugin>

#include "test_utils.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

namespace {
constexpr int kFlickableStopAtBounds = 0;

bool objectClassContains(QObject *root, const QString &classNameFragment)
{
    const QString rootClassName = QString::fromLatin1(root->metaObject()->className());
    if (rootClassName.contains(classNameFragment))
        return true;

    const auto objects = root->findChildren<QObject *>();
    for (QObject *object : objects) {
        const QString className = QString::fromLatin1(object->metaObject()->className());
        if (className.contains(classNameFragment))
            return true;
    }
    return false;
}

bool hasMetaProperty(QObject *object, const char *propertyName)
{
    return object->metaObject()->indexOfProperty(propertyName) >= 0;
}

bool hasMetaMethod(QObject *object, const char *methodName)
{
    const QMetaObject *metaObject = object->metaObject();
    const QByteArray expectedName(methodName);
    for (int i = 0; i < metaObject->methodCount(); ++i) {
        if (metaObject->method(i).name() == expectedName)
            return true;
    }
    return false;
}

QObject *documentModelFor(QObject *editor)
{
    return editor->findChild<QObject *>(QStringLiteral("textDocumentModel"));
}

}

class TextEditorTests : public QObject
{
    Q_OBJECT

private slots:
    void text_editor_requires_file_path_contract();
    void text_editor_minimal_file_api_contract();
    void text_editor_api_usage_manual_contract();
    void text_editor_file_realtime_sync_contract();
    void text_editor_chunked_lazy_read_contract();
    void text_editor_keyboard_realtime_sync_contract();
    void text_editor_ime_adapter_commit_contract();
    void text_editor_shift_selection_replaces_commit_contract();
    void text_editor_drag_selection_replaces_commit_contract();
    void text_editor_modifier_navigation_contract();
    void text_editor_clipboard_shortcut_contract();
    void text_editor_word_line_delete_shortcut_contract();
    void text_editor_internal_document_model_editing_contract();
    void text_editor_unicode_grapheme_editing_contract();
    void text_editor_view_keyboard_input_contract();
    void text_editor_ios_scroll_physics_contract();
    void text_editor_mobile_focus_suspends_viewport_flick_for_selection();
};

void TextEditorTests::text_editor_requires_file_path_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.TextEditor {
    width: 320
    height: 180
}
)";

    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root);
    QVERIFY(component.isError());
}

void TextEditorTests::text_editor_minimal_file_api_contract()
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

    property bool bodyTokenReady: editor.fontPixelSize === LV.Theme.textBody
        && editor.fontWeight === LV.Theme.textBodyWeight
        && editor.textLineHeight === LV.Theme.textBodyLineHeight
    property bool backgroundReady: editor.backgroundColorFocused === editor.backgroundColor
        && editor.backgroundColorDisabled === editor.backgroundColor

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: "/tmp/lvrs_text_editor_default.txt"
        width: 420
        height: 220
        placeholderText: "Write here"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("bodyTokenReady").toBool());
    QVERIFY(root->property("backgroundReady").toBool());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QVERIFY(documentModelFor(editor));

    QVERIFY(hasMetaProperty(editor, "filePath"));
    QVERIFY(hasMetaProperty(editor, "chunkSize"));
    QVERIFY(hasMetaProperty(editor, "dirty"));
    QVERIFY(hasMetaProperty(editor, "reading"));
    QVERIFY(hasMetaProperty(editor, "bytesRead"));
    QVERIFY(hasMetaProperty(editor, "bytesTotal"));
    QVERIFY(hasMetaProperty(editor, "progress"));
    QVERIFY(hasMetaProperty(editor, "error"));
    QVERIFY(hasMetaProperty(editor, "empty"));
    QVERIFY(!editor->property("dirty").toBool());
    QVERIFY(!editor->property("reading").toBool());
    QCOMPARE(editor->property("bytesRead").toLongLong(), qint64(0));
    QCOMPARE(editor->property("bytesTotal").toLongLong(), qint64(0));
    QCOMPARE(editor->property("progress").toReal(), 1.0);
    QCOMPARE(editor->property("error").toString(), QString());
    QVERIFY(editor->property("empty").toBool());

    const QList<const char *> retiredProperties = {
        "mode",
        "autoRead",
        "plainTextMode",
        "markdownMode",
        "richTextMode",
        "documentModel",
        "editorItem",
        "editorViewport",
        "text",
        "cursorPosition",
        "selectionStart",
        "selectionEnd",
        "selectedText",
        "selectionColor",
        "selectedTextColor",
        "contentWidth",
        "contentHeight",
        "lineCount",
        "textDocument",
        "canPaste",
        "fileBackedLineCount",
        "memoryLineCount",
        "loading",
        "loadedByteCount",
        "totalByteCount",
        "loadProgress",
        "loadChunkSize",
        "lastFileError",
        "submitShortcutEnabled",
        "effectiveSubmitShortcutEnabled",
        "preferNativeTextInteraction",
        "viewportBoundsBehavior",
        "viewportBoundsMovement"
    };
    for (const char *propertyName : retiredProperties) {
        QVERIFY2(!hasMetaProperty(editor, propertyName), propertyName);
    }

    const QList<const char *> retiredMethods = {
        "forceEditorFocus",
        "clearSelection",
        "insertText",
        "clear",
        "select",
        "selectAll",
        "deselect",
        "cut",
        "copy",
        "paste",
        "undo",
        "redo",
        "submit",
        "write",
        "writeFinished",
        "writeFailed",
        "reloadFile",
        "cancel",
        "cancelLoad",
        "loadFile",
        "saveFile",
        "markClean"
    };
    for (const char *methodName : retiredMethods) {
        QVERIFY2(!hasMetaMethod(editor, methodName), methodName);
    }

    QVERIFY(hasMetaMethod(editor, "read"));
    QVERIFY(hasMetaMethod(editor, "syncFinished"));
    QVERIFY(hasMetaMethod(editor, "syncFailed"));
}

void TextEditorTests::text_editor_api_usage_manual_contract()
{
    const QString docsPath = QDir(QString::fromUtf8(LVRS_TEST_SOURCE_DIR))
        .absoluteFilePath(QStringLiteral("../docs/components/control/TextEditor.md"));
    QFile docsFile(docsPath);
    QVERIFY2(docsFile.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(docsPath));

    const QString docs = QString::fromUtf8(docsFile.readAll());
    QVERIFY(docs.contains(QStringLiteral("## API Usage Manual")));
    QVERIFY(docs.contains(QStringLiteral("`filePath` is required")));
    QVERIFY(docs.contains(QStringLiteral("The editor reads the connected file automatically")));
    QVERIFY(docs.contains(QStringLiteral("Call `read()` only")));
    QVERIFY(docs.contains(QStringLiteral("There is no public save API")));
    QVERIFY(docs.contains(QStringLiteral("Use the read/sync state properties")));
    QVERIFY(docs.contains(QStringLiteral("Changing `filePath` changes the connected file")));
    QVERIFY(docs.contains(QStringLiteral("Application code should not depend on internal document objects")));
    QVERIFY(docs.contains(QStringLiteral("Option+Left")));
    QVERIFY(docs.contains(QStringLiteral("Cmd+A")));
    QVERIFY(docs.contains(QStringLiteral("These are internal input behaviors")));
    QVERIFY(docs.contains(QStringLiteral("`documentModel`")));
    QVERIFY(docs.contains(QStringLiteral("`write()`, `loadFile(path)`, `saveFile(path)`, `reloadFile()`")));
    QVERIFY(docs.contains(QStringLiteral("`loadFile(path)`, `saveFile(path)`, `reloadFile()`")));
    QVERIFY(docs.contains(QStringLiteral("syncFinished(path)")));
    QVERIFY(!docs.contains(QStringLiteral("autoRead")));
    QVERIFY(!docs.contains(QStringLiteral("cancel()")));
    QVERIFY(!docs.contains(QStringLiteral("Call `write()`")));
    QVERIFY(!docs.contains(QStringLiteral("onWriteFinished")));
}

void TextEditorTests::text_editor_file_realtime_sync_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    width: 480
    height: 320

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: initialFilePath
        width: 420
        height: 220
    }
}
)";

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString sourcePath = tempDir.path() + QStringLiteral("/source.txt");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(sourceFile.write("from disk"), qint64(9));
    sourceFile.close();

    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);

    QSignalSpy readFinishedSpy(editor, SIGNAL(readFinished(QString)));
    QSignalSpy readFailedSpy(editor, SIGNAL(readFailed(QString,QString)));
    QSignalSpy syncFinishedSpy(editor, SIGNAL(syncFinished(QString)));
    QSignalSpy syncFailedSpy(editor, SIGNAL(syncFailed(QString,QString)));
    QVERIFY(readFinishedSpy.isValid());
    QVERIFY(readFailedSpy.isValid());
    QVERIFY(syncFinishedSpy.isValid());
    QVERIFY(syncFailedSpy.isValid());

    QCOMPARE(editor->property("filePath").toString(), sourcePath);
    QCOMPARE(editor->property("bytesTotal").toLongLong(), QFileInfo(sourcePath).size());
    QTRY_VERIFY(!editor->property("reading").toBool());
    QCOMPARE(model->property("text").toString(), QStringLiteral("from disk"));
    QCOMPARE(model->property("fileBackedLineCount").toInt(), 1);
    QCOMPARE(model->property("memoryLineCount").toInt(), 0);
    QVERIFY(!editor->property("dirty").toBool());
    QCOMPARE(editor->property("error").toString(), QString());
    QTRY_COMPARE(readFinishedSpy.count(), 1);
    QCOMPARE(readFinishedSpy.takeFirst().at(0).toString(), sourcePath);

    QVERIFY(QMetaObject::invokeMethod(model, "moveCursor", Q_ARG(int, 0), Q_ARG(int, 9)));
    QVERIFY(QMetaObject::invokeMethod(model,
                                      "insertText",
                                      Q_ARG(QString, QStringLiteral(" + edit"))));
    QCOMPARE(model->property("text").toString(), QStringLiteral("from disk + edit"));
    QCOMPARE(model->property("fileBackedLineCount").toInt(), 0);
    QCOMPARE(model->property("memoryLineCount").toInt(), 1);
    QVERIFY(editor->property("dirty").toBool());

    QTRY_COMPARE(syncFinishedSpy.count(), 1);
    QCOMPARE(syncFinishedSpy.takeFirst().at(0).toString(), sourcePath);
    QCOMPARE(model->property("fileBackedLineCount").toInt(), 1);
    QCOMPARE(model->property("memoryLineCount").toInt(), 0);
    QVERIFY(!editor->property("dirty").toBool());
    QCOMPARE(editor->property("error").toString(), QString());

    QFile savedFile(sourcePath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(savedFile.readAll()), QStringLiteral("from disk + edit"));

    editor->setProperty("filePath", tempDir.path() + QStringLiteral("/missing-dir/out.txt"));
    QVERIFY(QMetaObject::invokeMethod(model, "insertText", Q_ARG(QString, QStringLiteral("!"))));
    QTRY_COMPARE(syncFailedSpy.count(), 1);
    QVERIFY(!editor->property("error").toString().isEmpty());
}

void TextEditorTests::text_editor_chunked_lazy_read_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    width: 480
    height: 320

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: initialFilePath
        chunkSize: 1024
        width: 420
        height: 220
    }
}
)";

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const char32_t multibyteCodepoints[] = { 0x20, 0xD55C, 0xAE00, 0x1F642, 0x20, 0 };
    const QString multibyteText = QString::fromUcs4(multibyteCodepoints);
    QString expected;
    for (int i = 0; i < 500; ++i) {
        if (!expected.isEmpty())
            expected.append(QLatin1Char('\n'));
        expected.append(QStringLiteral("line %1").arg(i, 3, 10, QLatin1Char('0')));
        expected.append(multibyteText);
        expected.append(QStringLiteral("abcdefghijklmnopqrstuvwxyz"));
    }

    const QString sourcePath = tempDir.path() + QStringLiteral("/chunked.txt");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    const QByteArray payload = expected.toUtf8();
    QCOMPARE(sourceFile.write(payload), qint64(payload.size()));
    sourceFile.close();

    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);

    QSignalSpy readFinishedSpy(editor, SIGNAL(readFinished(QString)));
    QSignalSpy readProgressSpy(editor, SIGNAL(readProgress(QString,QVariant,QVariant)));
    QVERIFY(readFinishedSpy.isValid());
    QVERIFY(readProgressSpy.isValid());

    QVERIFY(editor->property("reading").toBool());
    QCOMPARE(model->property("text").toString(), QString());
    QCOMPARE(editor->property("bytesRead").toLongLong(), qint64(0));
    QCOMPARE(editor->property("bytesTotal").toLongLong(), qint64(payload.size()));
    QCOMPARE(editor->property("chunkSize").toInt(), 1024);

    QTRY_VERIFY(!editor->property("reading").toBool());
    QTRY_COMPARE(readFinishedSpy.count(), 1);
    QVERIFY(readProgressSpy.count() > 1);
    QCOMPARE(editor->property("bytesRead").toLongLong(), qint64(payload.size()));
    QCOMPARE(editor->property("bytesTotal").toLongLong(), qint64(payload.size()));
    QCOMPARE(editor->property("progress").toReal(), 1.0);
    QCOMPARE(model->property("lineCount").toInt(), 500);
    QCOMPARE(model->property("fileBackedLineCount").toInt(), 500);
    QCOMPARE(model->property("memoryLineCount").toInt(), 0);
    QCOMPARE(model->property("characterCount").toLongLong(), qint64(expected.size()));
    QCOMPARE(model->property("text").toString(), expected);
    QVERIFY(!editor->property("dirty").toBool());
}

void TextEditorTests::text_editor_keyboard_realtime_sync_contract()
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

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: initialFilePath
        anchors.fill: parent
    }
}
)";

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString sourcePath = tempDir.path() + QStringLiteral("/shortcut.txt");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(sourceFile.write("alpha"), qint64(5));
    sourceFile.close();

    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);

    QTRY_VERIFY(!editor->property("reading").toBool());
    QCOMPARE(model->property("text").toString(), QStringLiteral("alpha"));

    auto *editorItem = qobject_cast<QQuickItem *>(editor);
    QVERIFY(editorItem);
    editorItem->forceActiveFocus();
    QTRY_VERIFY(editorItem->hasActiveFocus());

    QSignalSpy syncFinishedSpy(editor, SIGNAL(syncFinished(QString)));
    QVERIFY(syncFinishedSpy.isValid());

    QTest::keyClick(window, Qt::Key_Z, Qt::NoModifier, 10);
    QTRY_VERIFY(model->property("text").toString().startsWith(QStringLiteral("z")));

    QTRY_COMPARE(syncFinishedSpy.count(), 1);
    QVERIFY(!editor->property("dirty").toBool());

    QFile savedFile(sourcePath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(QString::fromUtf8(savedFile.readAll()).startsWith(QStringLiteral("zalpha")));
    savedFile.close();

    QTest::keyClick(window, Qt::Key_Return, Qt::ControlModifier, 10);
    QTRY_VERIFY(model->property("text").toString().startsWith(QStringLiteral("z\nalpha")));
}

void TextEditorTests::text_editor_ime_adapter_commit_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    width: 420
    height: 240
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: initialFilePath
        anchors.fill: parent
    }
}
)";

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourcePath = tempDir.path() + QStringLiteral("/ime.txt");
    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QObject *adapter = root->findChild<QObject *>(QStringLiteral("editorImeAdapter"));
    QVERIFY(adapter);

    QTRY_VERIFY(!editor->property("reading").toBool());
    qobject_cast<QQuickItem *>(editor)->forceActiveFocus();
    QTRY_VERIFY(qobject_cast<QQuickItem *>(adapter)->hasActiveFocus());

    QInputMethodEvent preeditEvent(QStringLiteral("ㅎ"), {});
    QCoreApplication::sendEvent(adapter, &preeditEvent);
    QCOMPARE(adapter->property("preeditText").toString(), QStringLiteral("ㅎ"));
    QCOMPARE(model->property("text").toString(), QString());

    QInputMethodEvent commitEvent;
    commitEvent.setCommitString(QStringLiteral("한"));
    QCoreApplication::sendEvent(adapter, &commitEvent);
    QTRY_COMPARE(model->property("text").toString(), QStringLiteral("한"));
    QCOMPARE(adapter->property("text").toString(), QString());
}

void TextEditorTests::text_editor_shift_selection_replaces_commit_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    width: 420
    height: 240
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: initialFilePath
        anchors.fill: parent
    }
}
)";

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourcePath = tempDir.path() + QStringLiteral("/shift-selection.txt");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(sourceFile.write("alpha"), qint64(5));
    sourceFile.close();
    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QObject *adapter = root->findChild<QObject *>(QStringLiteral("editorImeAdapter"));
    QVERIFY(adapter);

    QTRY_VERIFY(!editor->property("reading").toBool());
    qobject_cast<QQuickItem *>(editor)->forceActiveFocus();
    QTRY_VERIFY(qobject_cast<QQuickItem *>(adapter)->hasActiveFocus());

    QTest::keyClick(window, Qt::Key_Right, Qt::ShiftModifier, 10);
    QTest::keyClick(window, Qt::Key_Right, Qt::ShiftModifier, 10);

    QInputMethodEvent commitEvent;
    commitEvent.setCommitString(QStringLiteral("한"));
    QCoreApplication::sendEvent(adapter, &commitEvent);
    QTRY_COMPARE(model->property("text").toString(), QStringLiteral("한pha"));
}

void TextEditorTests::text_editor_drag_selection_replaces_commit_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    width: 420
    height: 240
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: initialFilePath
        anchors.fill: parent
    }
}
)";

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourcePath = tempDir.path() + QStringLiteral("/drag-selection.txt");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(sourceFile.write("alpha"), qint64(5));
    sourceFile.close();
    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QObject *adapter = root->findChild<QObject *>(QStringLiteral("editorImeAdapter"));
    QVERIFY(adapter);

    QTRY_VERIFY(!editor->property("reading").toBool());

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(12, 18), 10);
    QTest::mouseMove(window, QPoint(220, 18), 10);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(220, 18), 10);

    QInputMethodEvent commitEvent;
    commitEvent.setCommitString(QStringLiteral("한"));
    QCoreApplication::sendEvent(adapter, &commitEvent);
    QTRY_COMPARE(model->property("text").toString(), QStringLiteral("한"));
}

void TextEditorTests::text_editor_modifier_navigation_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    width: 480
    height: 260
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: initialFilePath
        anchors.fill: parent
    }
}
)";

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourcePath = tempDir.path() + QStringLiteral("/modifier-navigation.txt");
    const QString sourceText = QStringLiteral("alpha beta\n한글 delta");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(sourceFile.write(sourceText.toUtf8()), qint64(sourceText.toUtf8().size()));
    sourceFile.close();
    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QObject *adapter = root->findChild<QObject *>(QStringLiteral("editorImeAdapter"));
    QVERIFY(adapter);

    QTRY_VERIFY(!editor->property("reading").toBool());
    qobject_cast<QQuickItem *>(editor)->forceActiveFocus();
    QTRY_VERIFY(qobject_cast<QQuickItem *>(adapter)->hasActiveFocus());

    QTest::keyClick(window, Qt::Key_Right, Qt::AltModifier, 10);
    QTRY_COMPARE(model->property("cursorPosition").toInt(), 5);

    QTest::keyClick(window, Qt::Key_Right, Qt::AltModifier, 10);
    QTRY_COMPARE(model->property("cursorPosition").toInt(), 10);

    QTest::keyClick(window, Qt::Key_Left, Qt::AltModifier, 10);
    QTRY_COMPARE(model->property("cursorPosition").toInt(), 6);

    QTest::keyClick(window, Qt::Key_Right, Qt::MetaModifier, 10);
    QTRY_COMPARE(model->property("cursorPosition").toInt(), 10);

    QTest::keyClick(window, Qt::Key_Down, Qt::MetaModifier, 10);
    QTRY_COMPARE(model->property("cursorPosition").toInt(), sourceText.size());

    QTest::keyClick(window, Qt::Key_Left, Qt::AltModifier, 10);
    QTRY_COMPARE(model->property("cursorPosition").toInt(), sourceText.indexOf(QStringLiteral("delta")));

    QTest::keyClick(window, Qt::Key_Left, Qt::AltModifier, 10);
    QTRY_COMPARE(model->property("cursorPosition").toInt(), sourceText.indexOf(QStringLiteral("한글")));

    QTest::keyClick(window, Qt::Key_Up, Qt::MetaModifier, 10);
    QTRY_COMPARE(model->property("cursorPosition").toInt(), 0);

    QTest::keyClick(window, Qt::Key_Right, Qt::KeyboardModifiers(Qt::AltModifier | Qt::ShiftModifier), 10);

    QInputMethodEvent commitEvent;
    commitEvent.setCommitString(QStringLiteral("한"));
    QCoreApplication::sendEvent(adapter, &commitEvent);
    QTRY_COMPARE(model->property("text").toString(), QStringLiteral("한 beta\n한글 delta"));
}

void TextEditorTests::text_editor_clipboard_shortcut_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    width: 420
    height: 240
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: initialFilePath
        anchors.fill: parent
    }
}
)";

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourcePath = tempDir.path() + QStringLiteral("/clipboard.txt");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(sourceFile.write("alpha beta"), qint64(10));
    sourceFile.close();
    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QObject *adapter = root->findChild<QObject *>(QStringLiteral("editorImeAdapter"));
    QVERIFY(adapter);
    QClipboard *clipboard = QGuiApplication::clipboard();
    QVERIFY(clipboard);

    QTRY_VERIFY(!editor->property("reading").toBool());
    qobject_cast<QQuickItem *>(editor)->forceActiveFocus();
    QTRY_VERIFY(qobject_cast<QQuickItem *>(adapter)->hasActiveFocus());

    clipboard->clear();
    QTest::keyClick(window, Qt::Key_A, Qt::MetaModifier, 10);
    QTest::keyClick(window, Qt::Key_C, Qt::MetaModifier, 10);
    QTRY_COMPARE(clipboard->text(), QStringLiteral("alpha beta"));

    clipboard->setText(QStringLiteral("붙여넣기"));
    QTest::keyClick(window, Qt::Key_V, Qt::MetaModifier, 10);
    QTRY_COMPARE(model->property("text").toString(), QStringLiteral("붙여넣기"));

    QTest::keyClick(window, Qt::Key_A, Qt::MetaModifier, 10);
    QTest::keyClick(window, Qt::Key_X, Qt::MetaModifier, 10);
    QTRY_COMPARE(clipboard->text(), QStringLiteral("붙여넣기"));
    QTRY_COMPARE(model->property("text").toString(), QString());

    QVERIFY(model->setProperty("text", QStringLiteral("read only copy")));
    QVERIFY(model->setProperty("cursorPosition", 0));
    QVERIFY(editor->setProperty("readOnly", true));
    qobject_cast<QQuickItem *>(editor)->forceActiveFocus();
    clipboard->clear();
    QTest::keyClick(window, Qt::Key_A, Qt::MetaModifier, 10);
    QTest::keyClick(window, Qt::Key_C, Qt::MetaModifier, 10);
    QTRY_COMPARE(clipboard->text(), QStringLiteral("read only copy"));
}

void TextEditorTests::text_editor_word_line_delete_shortcut_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    width: 420
    height: 240
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: initialFilePath
        anchors.fill: parent
    }
}
)";

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourcePath = tempDir.path() + QStringLiteral("/delete-shortcuts.txt");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(sourceFile.write("alpha beta"), qint64(10));
    sourceFile.close();
    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QObject *adapter = root->findChild<QObject *>(QStringLiteral("editorImeAdapter"));
    QVERIFY(adapter);

    QTRY_VERIFY(!editor->property("reading").toBool());
    qobject_cast<QQuickItem *>(editor)->forceActiveFocus();
    QTRY_VERIFY(qobject_cast<QQuickItem *>(adapter)->hasActiveFocus());

    QVERIFY(model->setProperty("text", QStringLiteral("alpha beta")));
    QVERIFY(model->setProperty("cursorPosition", 0));
    QTest::keyClick(window, Qt::Key_Delete, Qt::AltModifier, 10);
    QTRY_COMPARE(model->property("text").toString(), QStringLiteral(" beta"));

    QVERIFY(model->setProperty("text", QStringLiteral("alpha beta")));
    QVERIFY(model->setProperty("cursorPosition", 10));
    QTest::keyClick(window, Qt::Key_Backspace, Qt::AltModifier, 10);
    QTRY_COMPARE(model->property("text").toString(), QStringLiteral("alpha "));

    QVERIFY(model->setProperty("text", QStringLiteral("alpha beta")));
    QVERIFY(model->setProperty("cursorPosition", 6));
    QTest::keyClick(window, Qt::Key_Backspace, Qt::MetaModifier, 10);
    QTRY_COMPARE(model->property("text").toString(), QStringLiteral("beta"));

    QVERIFY(model->setProperty("text", QStringLiteral("alpha beta")));
    QVERIFY(model->setProperty("cursorPosition", 6));
    QTest::keyClick(window, Qt::Key_Delete, Qt::MetaModifier, 10);
    QTRY_COMPARE(model->property("text").toString(), QStringLiteral("alpha "));
}

void TextEditorTests::text_editor_internal_document_model_editing_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    width: 480
    height: 320

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: "/tmp/lvrs_text_editor_document_model.txt"
        width: 320
        height: 180
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QVERIFY(model->setProperty("text", QStringLiteral("alpha\nbeta\ngamma")));

    QCOMPARE(model->property("lineCount").toInt(), 3);
    QString lineText;
    QVERIFY(QMetaObject::invokeMethod(model,
                                      "lineText",
                                      Q_RETURN_ARG(QString, lineText),
                                      Q_ARG(int, 1)));
    QCOMPARE(lineText, QStringLiteral("beta"));

    QVERIFY(QMetaObject::invokeMethod(model, "moveCursor", Q_ARG(int, 1), Q_ARG(int, 4)));
    QVERIFY(QMetaObject::invokeMethod(model, "insertText", Q_ARG(QString, QStringLiteral("!"))));
    QCOMPARE(model->property("text").toString(), QStringLiteral("alpha\nbeta!\ngamma"));
    QCOMPARE(model->property("cursorLine").toInt(), 1);
    QCOMPARE(model->property("cursorColumn").toInt(), 5);

    bool removed = false;
    QVERIFY(QMetaObject::invokeMethod(model, "removePreviousCharacter", Q_RETURN_ARG(bool, removed)));
    QVERIFY(removed);
    QCOMPARE(model->property("text").toString(), QStringLiteral("alpha\nbeta\ngamma"));
}

void TextEditorTests::text_editor_unicode_grapheme_editing_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    width: 480
    height: 320

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: "/tmp/lvrs_text_editor_grapheme.txt"
        width: 320
        height: 180
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);

    const char32_t emojiCodepoints[] = { 0x1F642, 0 };
    const QString emoji = QString::fromUcs4(emojiCodepoints);

    QVERIFY(model->setProperty("text", emoji + QStringLiteral("a")));
    QVERIFY(model->setProperty("cursorPosition", emoji.size()));
    bool removed = false;
    QVERIFY(QMetaObject::invokeMethod(model, "removePreviousCharacter", Q_RETURN_ARG(bool, removed)));
    QVERIFY(removed);
    QCOMPARE(model->property("text").toString(), QStringLiteral("a"));

    QVERIFY(model->setProperty("text", emoji + QStringLiteral("a")));
    QVERIFY(model->setProperty("cursorPosition", 0));
    QVERIFY(QMetaObject::invokeMethod(model, "moveCursorRight"));
    QCOMPARE(model->property("cursorPosition").toInt(), emoji.size());

    removed = false;
    QVERIFY(QMetaObject::invokeMethod(model, "removeNextCharacter", Q_RETURN_ARG(bool, removed)));
    QVERIFY(removed);
    QCOMPARE(model->property("text").toString(), emoji);
}

void TextEditorTests::text_editor_view_keyboard_input_contract()
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

    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: "/tmp/lvrs_text_editor_native_matrix.txt"
        anchors.fill: parent
        editorHeight: 180
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
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("editorViewportFlickable"));
    QVERIFY(viewport);
    QVERIFY(QString::fromLatin1(viewport->metaObject()->className()).contains(QStringLiteral("ListView")));
    QVERIFY(!hasMetaProperty(editor, "editorItem"));
    QVERIFY(!hasMetaProperty(editor, "editorViewport"));
    QVERIFY(!objectClassContains(root.data(), QStringLiteral("QQuickTextArea")));
    QVERIFY(!objectClassContains(root.data(), QStringLiteral("QQuickTextEdit")));

    auto *editorItem = qobject_cast<QQuickItem *>(editor);
    QVERIFY(editorItem);
    editorItem->forceActiveFocus();
    QTRY_VERIFY(editorItem->hasActiveFocus());

    QTest::keyClick(window, Qt::Key_Z, Qt::NoModifier, 10);
    QTRY_VERIFY(model->property("text").toString().startsWith(QStringLiteral("z")));
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
        filePath: "/tmp/lvrs_text_editor_scroll_physics.txt"
        anchors.fill: parent
        editorHeight: 96
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QVERIFY(model->setProperty("text", QStringLiteral("line 01\nline 02\nline 03\nline 04\nline 05\nline 06\nline 07\nline 08\nline 09\nline 10\nline 11\nline 12\nline 13\nline 14\nline 15")));

    QObject *editorViewport = root->findChild<QObject *>(QStringLiteral("editorViewportFlickable"));
    QVERIFY(editorViewport);

    QCOMPARE(editorViewport->property("boundsBehavior").toInt(), kFlickableStopAtBounds);
    QCOMPARE(editorViewport->property("boundsMovement").toInt(), kFlickableStopAtBounds);
    QVERIFY(!hasMetaProperty(editor, "viewportBoundsBehavior"));
    QVERIFY(!hasMetaProperty(editor, "viewportBoundsMovement"));
    QCOMPARE(editorViewport->property("flickDeceleration").toInt(), editor->property("viewportFlickDeceleration").toInt());
    QCOMPARE(editorViewport->property("maximumFlickVelocity").toInt(), editor->property("viewportMaximumFlickVelocity").toInt());
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
        filePath: "/tmp/lvrs_text_editor_mobile_focus.txt"
        anchors.fill: parent
        editorHeight: 96
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);
    QVERIFY(model->setProperty("text", QStringLiteral("line 01\nline 02\nline 03\nline 04\nline 05\nline 06\nline 07\nline 08\nline 09\nline 10\nline 11\nline 12")));

    QObject *viewport = root->findChild<QObject *>(QStringLiteral("editorViewportFlickable"));
    QVERIFY(viewport);

    QTRY_VERIFY(viewport->property("contentHeight").toReal() > viewport->property("height").toReal());
    QVERIFY(viewport->property("interactive").toBool());

    auto *editorItem = qobject_cast<QQuickItem *>(editor);
    QVERIFY(editorItem);
    QVERIFY(!editorItem->hasActiveFocus());
    editorItem->forceActiveFocus();
    QTRY_VERIFY(editorItem->hasActiveFocus());
    QTRY_VERIFY(!viewport->property("interactive").toBool());
}

QTEST_MAIN(TextEditorTests)
#include "tst_text_editor.moc"
