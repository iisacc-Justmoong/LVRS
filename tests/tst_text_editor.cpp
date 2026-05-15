#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQuickItem>
#include <QScopedPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QTemporaryDir>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtPlugin>

#include "test_utils.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

namespace {

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
    void text_editor_rich_text_surface_contract();
    void text_editor_mobile_native_gesture_viewport_scroll_contract();
    void text_editor_api_usage_manual_contract();
    void text_editor_rich_text_file_realtime_sync_contract();
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

void TextEditorTests::text_editor_rich_text_surface_contract()
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

    property bool richTextReady: editor.textFormat === TextEdit.RichText
        && editor.editorItem.textFormat === TextEdit.RichText
        && editor.editorItem.wrapMode === TextEdit.Wrap
        && editor.editorItem.objectName === "textEditorRichTextEdit"
    property bool nativeSurfaceReady: editor.editorItem.cursorVisible === false
        && editor.editorItem.selectByMouse
        && editor.editorItem.persistentSelection
    property bool bodyTokenReady: editor.fontPixelSize === LV.Theme.textBody
        && editor.fontWeight === LV.Theme.textBodyWeight
        && editor.textLineHeight === LV.Theme.textBodyLineHeight
    property bool backgroundReady: editor.backgroundColorFocused === editor.backgroundColor
        && editor.backgroundColorDisabled === editor.backgroundColor

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: "/tmp/lvrs_text_editor_default.html"
        width: 420
        height: 220
        placeholderText: "Write here"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("richTextReady").toBool());
    QVERIFY(root->property("nativeSurfaceReady").toBool());
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
    QVERIFY(hasMetaProperty(editor, "editorItem"));
    QVERIFY(hasMetaProperty(editor, "text"));
    QVERIFY(hasMetaProperty(editor, "textFormat"));
    QVERIFY(hasMetaProperty(editor, "textDocument"));

    QCOMPARE(editor->property("textFormat").toInt(), 1);
    QVERIFY(!editor->property("dirty").toBool());
    QVERIFY(!editor->property("reading").toBool());
    QCOMPARE(editor->property("bytesRead").toLongLong(), qint64(0));
    QCOMPARE(editor->property("bytesTotal").toLongLong(), qint64(0));
    QCOMPARE(editor->property("progress").toReal(), 1.0);
    QCOMPARE(editor->property("error").toString(), QString());
    QVERIFY(editor->property("empty").toBool());

    QVERIFY(hasMetaMethod(editor, "read"));
    QVERIFY(hasMetaMethod(editor, "syncFinished"));
    QVERIFY(hasMetaMethod(editor, "syncFailed"));
    QVERIFY(hasMetaMethod(editor, "forceEditorFocus"));
    QVERIFY(hasMetaMethod(editor, "insertText"));
    QVERIFY(hasMetaMethod(editor, "selectAll"));
    QVERIFY(hasMetaMethod(editor, "copy"));
    QVERIFY(hasMetaMethod(editor, "paste"));
}

void TextEditorTests::text_editor_mobile_native_gesture_viewport_scroll_contract()
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

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: ""
        width: 320
        height: 120
        editorHeight: 120
        text: "<p>line 01</p><p>line 02</p><p>line 03</p><p>line 04</p><p>line 05</p><p>line 06</p><p>line 07</p><p>line 08</p><p>line 09</p><p>line 10</p><p>line 11</p><p>line 12</p>"
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
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("editorViewportFlickable"));
    QVERIFY(viewport);

    QVERIFY(QMetaObject::invokeMethod(editor, "forceEditorFocus"));
    QTRY_VERIFY(editor->property("preferNativeGestures").toBool());
    QTRY_VERIFY(editor->property("preferNativeTextInteraction").toBool());
    QTRY_VERIFY(editor->property("focused").toBool());
    QTRY_VERIFY(viewport->property("contentHeight").toReal() > viewport->property("height").toReal());
    QTRY_VERIFY(viewport->property("interactive").toBool());
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
    QVERIFY(docs.contains(QStringLiteral("TextEdit.RichText")));
    QVERIFY(docs.contains(QStringLiteral("Mac TextEdit-style")));
    QVERIFY(docs.contains(QStringLiteral("The editor reads the connected file automatically")));
    QVERIFY(docs.contains(QStringLiteral("There is no public save API")));
    QVERIFY(docs.contains(QStringLiteral("Use `editorItem`")));
    QVERIFY(docs.contains(QStringLiteral("mobile touch drags scroll the internal viewport")));
    QVERIFY(docs.contains(QStringLiteral("syncFinished(path)")));
    QVERIFY(!docs.contains(QStringLiteral("plain-text editor")));
    QVERIFY(!docs.contains(QStringLiteral("Avoid `TextArea`/`TextEdit` as the document model")));
}

void TextEditorTests::text_editor_rich_text_file_realtime_sync_contract()
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

    const QString sourcePath = tempDir.path() + QStringLiteral("/source.html");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text));
    const QByteArray initialHtml = "<h1>Notes</h1><p>Hello <b>bold</b></p>";
    QCOMPARE(sourceFile.write(initialHtml), qint64(initialHtml.size()));
    sourceFile.close();

    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    QVERIFY(editor);
    QObject *model = documentModelFor(editor);
    QVERIFY(model);

    QSignalSpy readFinishedSpy(editor, SIGNAL(readFinished(QString)));
    QSignalSpy syncFinishedSpy(editor, SIGNAL(syncFinished(QString)));
    QVERIFY(readFinishedSpy.isValid());
    QVERIFY(syncFinishedSpy.isValid());

    QCOMPARE(editor->property("filePath").toString(), sourcePath);
    QCOMPARE(editor->property("bytesTotal").toLongLong(), QFileInfo(sourcePath).size());
    QTRY_VERIFY(!editor->property("reading").toBool());
    QTRY_COMPARE(readFinishedSpy.count(), 1);
    QCOMPARE(readFinishedSpy.takeFirst().at(0).toString(), sourcePath);

    const QString loadedText = editor->property("text").toString();
    QVERIFY(loadedText.contains(QStringLiteral("Notes")));
    QVERIFY(loadedText.contains(QStringLiteral("bold")));
    QCOMPARE(model->property("text").toString(), QString::fromUtf8(initialHtml));
    QVERIFY(!editor->property("dirty").toBool());
    QCOMPARE(editor->property("error").toString(), QString());

    const QString editedHtml = QStringLiteral("<h1>Notes</h1><p>Hello <b>bold</b> and <i>italic</i></p>");
    QVERIFY(editor->setProperty("text", editedHtml));
    QTRY_VERIFY(model->property("text").toString().contains(QStringLiteral("bold")));
    QTRY_VERIFY(model->property("text").toString().contains(QStringLiteral("italic")));
    QVERIFY(editor->property("dirty").toBool());

    QTRY_COMPARE(syncFinishedSpy.count(), 1);
    QVERIFY(!editor->property("dirty").toBool());
    QCOMPARE(editor->property("error").toString(), QString());

    QFile savedFile(sourcePath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString savedText = QString::fromUtf8(savedFile.readAll());
    QVERIFY(savedText.contains(QStringLiteral("qrichtext")));
    QVERIFY(savedText.contains(QStringLiteral("bold")));
    QVERIFY(savedText.contains(QStringLiteral("italic")));
    QVERIFY(savedText.contains(QStringLiteral("font-weight")));
    QVERIFY(savedText.contains(QStringLiteral("font-style")));
}

QTEST_MAIN(TextEditorTests)
#include "tst_text_editor.moc"
