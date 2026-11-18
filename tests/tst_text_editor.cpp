#include "QtEventWait.h"
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMetaMethod>

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

class EditorSignalRecorder : public QObject
{
    Q_OBJECT
public:
    QList<QPair<QString, int>> documentEvents;
    QList<QString> readEvents;
    QList<QString> syncEvents;
public slots:
    void recordDocumentEdited(const QString &text, int revision) { documentEvents.append({text, revision}); }
    void recordReadFinished(const QString &path) { readEvents.append(path); }
    void recordSyncFinished(const QString &path) { syncEvents.append(path); }
};

class TextEditorTests
{
public:
    void text_editor_requires_file_path_contract();
    void text_editor_rich_text_surface_contract();
    void text_editor_document_edited_emits_latest_payload_and_revision();
    void text_editor_mobile_native_gesture_viewport_scroll_contract();
    void text_editor_api_usage_manual_contract();
    void text_editor_rich_text_file_realtime_sync_contract();
    void text_editor_return_key_moves_caret_to_inserted_line();
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
    CTEST_CHECK(!root);
    CTEST_CHECK(component.isError());
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
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("richTextReady").toBool());
    CTEST_CHECK(root->property("nativeSurfaceReady").toBool());
    CTEST_CHECK(root->property("bodyTokenReady").toBool());
    CTEST_CHECK(root->property("backgroundReady").toBool());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    CTEST_CHECK(editor);
    CTEST_CHECK(documentModelFor(editor));

    CTEST_CHECK(hasMetaProperty(editor, "filePath"));
    CTEST_CHECK(hasMetaProperty(editor, "chunkSize"));
    CTEST_CHECK(hasMetaProperty(editor, "dirty"));
    CTEST_CHECK(hasMetaProperty(editor, "reading"));
    CTEST_CHECK(hasMetaProperty(editor, "bytesRead"));
    CTEST_CHECK(hasMetaProperty(editor, "bytesTotal"));
    CTEST_CHECK(hasMetaProperty(editor, "progress"));
    CTEST_CHECK(hasMetaProperty(editor, "error"));
    CTEST_CHECK(hasMetaProperty(editor, "empty"));
    CTEST_CHECK(hasMetaProperty(editor, "editorItem"));
    CTEST_CHECK(hasMetaProperty(editor, "text"));
    CTEST_CHECK(hasMetaProperty(editor, "textFormat"));
    CTEST_CHECK(hasMetaProperty(editor, "textDocument"));
    CTEST_CHECK(hasMetaProperty(editor, "documentRevision"));

    CTEST_COMPARE(editor->property("textFormat").toInt(), 1);
    CTEST_CHECK(!editor->property("dirty").toBool());
    CTEST_CHECK(!editor->property("reading").toBool());
    CTEST_COMPARE(editor->property("bytesRead").toLongLong(), qint64(0));
    CTEST_COMPARE(editor->property("bytesTotal").toLongLong(), qint64(0));
    CTEST_COMPARE(editor->property("progress").toReal(), 1.0);
    CTEST_COMPARE(editor->property("error").toString(), QString());
    CTEST_CHECK(editor->property("empty").toBool());

    CTEST_CHECK(hasMetaMethod(editor, "read"));
    CTEST_CHECK(hasMetaMethod(editor, "syncFinished"));
    CTEST_CHECK(hasMetaMethod(editor, "syncFailed"));
    CTEST_CHECK(hasMetaMethod(editor, "documentEdited"));
    CTEST_CHECK(hasMetaMethod(editor, "forceEditorFocus"));
    CTEST_CHECK(hasMetaMethod(editor, "insertText"));
    CTEST_CHECK(hasMetaMethod(editor, "selectAll"));
    CTEST_CHECK(hasMetaMethod(editor, "copy"));
    CTEST_CHECK(hasMetaMethod(editor, "paste"));
}

void TextEditorTests::text_editor_document_edited_emits_latest_payload_and_revision()
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
        filePath: ""
        width: 420
        height: 220
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    CTEST_CHECK(editor);
    CTEST_CHECK(hasMetaProperty(editor, "documentRevision"));
    CTEST_CHECK(hasMetaMethod(editor, "documentEdited"));

    EditorSignalRecorder recorder;
    const auto documentConnection = QObject::connect(editor, SIGNAL(documentEdited(QString,int)),
                                                       &recorder, SLOT(recordDocumentEdited(QString,int)));
    CTEST_CHECK(documentConnection);
    CTEST_COMPARE(editor->property("documentRevision").toInt(), 0);

    const QString firstPayload = QStringLiteral("안녕하세");
    CTEST_CHECK(editor->setProperty("text", firstPayload));
    CTEST_WAIT_COMPARE(recorder.documentEvents.count(), 1);
    const auto firstSignal = recorder.documentEvents.takeFirst();
    CTEST_COMPARE(firstSignal.first, editor->property("text").toString());
    CTEST_CHECK(firstSignal.first.contains(firstPayload));
    CTEST_COMPARE(firstSignal.second, 1);
    CTEST_COMPARE(editor->property("documentRevision").toInt(), 1);

    const QString latestPayload = QStringLiteral("안녕하세요");
    CTEST_CHECK(editor->setProperty("text", latestPayload));
    CTEST_WAIT_COMPARE(recorder.documentEvents.count(), 1);
    const auto latestSignal = recorder.documentEvents.takeFirst();
    CTEST_COMPARE(latestSignal.first, editor->property("text").toString());
    CTEST_CHECK(latestSignal.first.contains(latestPayload));
    CTEST_COMPARE(latestSignal.second, 2);
    CTEST_COMPARE(editor->property("documentRevision").toInt(), 2);
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    window->show();
    CTEST_WAIT_DEFAULT(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    CTEST_CHECK(editor);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("editorViewportFlickable"));
    CTEST_CHECK(viewport);

    CTEST_CHECK(QMetaObject::invokeMethod(editor, "forceEditorFocus"));
    CTEST_WAIT_DEFAULT(editor->property("preferNativeGestures").toBool());
    CTEST_WAIT_DEFAULT(editor->property("preferNativeTextInteraction").toBool());
    CTEST_WAIT_DEFAULT(editor->property("focused").toBool());
    CTEST_WAIT_DEFAULT(viewport->property("contentHeight").toReal() > viewport->property("height").toReal());
    CTEST_WAIT_DEFAULT(viewport->property("interactive").toBool());
}

void TextEditorTests::text_editor_api_usage_manual_contract()
{
    const QString docsPath = QDir(QString::fromUtf8(LVRS_TEST_SOURCE_DIR))
        .absoluteFilePath(QStringLiteral("../docs/components/control/TextEditor.md"));
    QFile docsFile(docsPath);
    CTEST_CHECK_MESSAGE(docsFile.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(docsPath));

    const QString docs = QString::fromUtf8(docsFile.readAll());
    CTEST_CHECK(docs.contains(QStringLiteral("id=\"api-usage-manual\"")));
    CTEST_CHECK(docs.contains(QStringLiteral("`filePath`가 필요")));
    CTEST_CHECK(docs.contains(QStringLiteral("TextEdit.RichText")));
    CTEST_CHECK(docs.contains(QStringLiteral("Mac TextEdit 스타일")));
    CTEST_CHECK(docs.contains(QStringLiteral("네이티브 Return/Enter 처리는 삽입된 줄에 캐럿")));
    CTEST_CHECK(docs.contains(QStringLiteral("id=\"read-the-connected-file\"")));
    CTEST_CHECK(docs.contains(QStringLiteral("공개 저장 API")));
    CTEST_CHECK(docs.contains(QStringLiteral("`editorItem`")));
    CTEST_CHECK(docs.contains(QStringLiteral("모바일 터치 드래그가 내부 뷰포트를 스크롤")));
    CTEST_CHECK(docs.contains(QStringLiteral("documentRevision")));
    CTEST_CHECK(docs.contains(QStringLiteral("documentEdited(documentText, documentRevision)")));
    CTEST_CHECK(docs.contains(QStringLiteral("입력 방법 구성")));
    CTEST_CHECK(docs.contains(QStringLiteral("커밋된 문서 페이로드")));
    CTEST_CHECK(docs.contains(QStringLiteral("syncFinished(path)")));
    CTEST_CHECK(!docs.contains(QStringLiteral("plain-text editor")));
    CTEST_CHECK(!docs.contains(QStringLiteral("Avoid `TextArea`/`TextEdit` as the document model")));

    const QString qmlPath = QDir(QString::fromUtf8(LVRS_TEST_SOURCE_DIR))
        .absoluteFilePath(QStringLiteral("../src/qml/components/control/input/TextEditor.qml"));
    QFile qmlFile(qmlPath);
    CTEST_CHECK_MESSAGE(qmlFile.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(qmlPath));

    const QString qmlSource = QString::fromUtf8(qmlFile.readAll());
    CTEST_CHECK(qmlSource.contains(QStringLiteral("property bool composingDocumentEditPending: false")));
    CTEST_CHECK(qmlSource.contains(QStringLiteral("function publishPendingComposedDocumentEdit()")));
    CTEST_CHECK(qmlSource.contains(QStringLiteral("if (editor.inputMethodComposing)")));
    CTEST_CHECK(qmlSource.contains(QStringLiteral("onInputMethodComposingChanged:")));
    CTEST_CHECK(qmlSource.contains(QStringLiteral("Qt.callLater(privateState.publishPendingComposedDocumentEdit)")));
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

    const QString resourceRoot = QDir::cleanPath(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources"));
    CTEST_CHECK(QDir().mkpath(resourceRoot));
    QTemporaryDir tempDir(QDir(resourceRoot).filePath(QStringLiteral("ctest-text-editor-XXXXXX")));
    qInfo().noquote() << "Text editor test workspace:" << tempDir.path();
    CTEST_CHECK(tempDir.isValid());

    const QString sourcePath = tempDir.path() + QStringLiteral("/source.html");
    QFile sourceFile(sourcePath);
    CTEST_CHECK(sourceFile.open(QIODevice::WriteOnly | QIODevice::Text));
    const QByteArray initialHtml = "<h1>Notes</h1><p>Hello <b>bold</b></p>";
    CTEST_COMPARE(sourceFile.write(initialHtml), qint64(initialHtml.size()));
    sourceFile.close();

    engine.rootContext()->setContextProperty(QStringLiteral("initialFilePath"), sourcePath);

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(root);

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    CTEST_CHECK(editor);
    QObject *model = documentModelFor(editor);
    CTEST_CHECK(model);

    EditorSignalRecorder recorder;
    const auto readConnection = QObject::connect(editor, SIGNAL(readFinished(QString)),
                                                  &recorder, SLOT(recordReadFinished(QString)));
    const auto syncConnection = QObject::connect(editor, SIGNAL(syncFinished(QString)),
                                                  &recorder, SLOT(recordSyncFinished(QString)));
    CTEST_CHECK(readConnection);
    CTEST_CHECK(syncConnection);

    CTEST_COMPARE(editor->property("filePath").toString(), sourcePath);
    CTEST_COMPARE(editor->property("bytesTotal").toLongLong(), QFileInfo(sourcePath).size());
    CTEST_WAIT_DEFAULT(!editor->property("reading").toBool());
    CTEST_WAIT_COMPARE(recorder.readEvents.count(), 1);
    CTEST_COMPARE(recorder.readEvents.takeFirst(), sourcePath);

    const QString loadedText = editor->property("text").toString();
    CTEST_CHECK(loadedText.contains(QStringLiteral("Notes")));
    CTEST_CHECK(loadedText.contains(QStringLiteral("bold")));
    CTEST_COMPARE(model->property("text").toString(), QString::fromUtf8(initialHtml));
    CTEST_CHECK(!editor->property("dirty").toBool());
    CTEST_COMPARE(editor->property("error").toString(), QString());

    const QString editedHtml = QStringLiteral("<h1>Notes</h1><p>Hello <b>bold</b> and <i>italic</i></p>");
    CTEST_CHECK(editor->setProperty("text", editedHtml));
    CTEST_WAIT_DEFAULT(model->property("text").toString().contains(QStringLiteral("bold")));
    CTEST_WAIT_DEFAULT(model->property("text").toString().contains(QStringLiteral("italic")));
    CTEST_CHECK(editor->property("dirty").toBool());

    CTEST_WAIT_COMPARE(recorder.syncEvents.count(), 1);
    CTEST_COMPARE(recorder.syncEvents.constFirst(), sourcePath);
    CTEST_CHECK(!editor->property("dirty").toBool());
    CTEST_COMPARE(editor->property("error").toString(), QString());

    QFile savedFile(sourcePath);
    CTEST_CHECK(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString savedText = QString::fromUtf8(savedFile.readAll());
    CTEST_CHECK(savedText.contains(QStringLiteral("qrichtext")));
    CTEST_CHECK(savedText.contains(QStringLiteral("bold")));
    CTEST_CHECK(savedText.contains(QStringLiteral("italic")));
    CTEST_CHECK(savedText.contains(QStringLiteral("font-weight")));
    CTEST_CHECK(savedText.contains(QStringLiteral("font-style")));
}

void TextEditorTests::text_editor_return_key_moves_caret_to_inserted_line()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 420
    height: 260
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    LV.TextEditor {
        id: editor
        objectName: "textEditor"
        filePath: ""
        width: 360
        height: 180
        editorHeight: 180
        text: "Alpha"
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    window->show();
    CTEST_WAIT_DEFAULT(window->isVisible());

    QObject *editor = root->findChild<QObject *>(QStringLiteral("textEditor"));
    CTEST_CHECK(editor);
    auto *editorItem = root->findChild<QQuickItem *>(QStringLiteral("textEditorRichTextEdit"));
    CTEST_CHECK(editorItem);
    CTEST_CHECK(QMetaObject::invokeMethod(editor, "forceEditorFocus"));
    CTEST_WAIT_DEFAULT(editor->property("focused").toBool());
    CTEST_CHECK(editor->setProperty("cursorPosition", 5));

    const QRectF firstLineCursorRect = editorItem->property("cursorRectangle").toRectF();
    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
    QCoreApplication::sendEvent(window, &keyPress);
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
    QCoreApplication::sendEvent(window, &keyRelease);

    CTEST_WAIT_DEFAULT(editor->property("cursorPosition").toInt() > 5);
    const QRectF insertedLineCursorRect = editorItem->property("cursorRectangle").toRectF();
    CTEST_CHECK_MESSAGE(
        insertedLineCursorRect.y() > firstLineCursorRect.y(),
        qPrintable(QStringLiteral("Return must place the caret on the inserted line: before y=%1, after y=%2")
                       .arg(firstLineCursorRect.y())
                       .arg(insertedLineCursorRect.y())));
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    TextEditorTests tests;
    runner.run("text_editor_requires_file_path_contract", [&] { tests.text_editor_requires_file_path_contract(); });
    runner.run("text_editor_rich_text_surface_contract", [&] { tests.text_editor_rich_text_surface_contract(); });
    runner.run("text_editor_document_edited_emits_latest_payload_and_revision", [&] { tests.text_editor_document_edited_emits_latest_payload_and_revision(); });
    runner.run("text_editor_mobile_native_gesture_viewport_scroll_contract", [&] { tests.text_editor_mobile_native_gesture_viewport_scroll_contract(); });
    runner.run("text_editor_api_usage_manual_contract", [&] { tests.text_editor_api_usage_manual_contract(); });
    runner.run("text_editor_rich_text_file_realtime_sync_contract", [&] { tests.text_editor_rich_text_file_realtime_sync_contract(); });
    runner.run("text_editor_return_key_moves_caret_to_inserted_line", [&] { tests.text_editor_return_key_moves_caret_to_inserted_line(); });
    return runner.result();
}
#include "tst_text_editor.moc"
