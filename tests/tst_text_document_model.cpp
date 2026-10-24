#include "backend/text/textdocumentmodel.h"
#include <iiFileProviderDevice.h>
#include <QtTest>
#include <QPersistentModelIndex>
#include <QTemporaryDir>

class TextDocumentStorageTests : public QObject {
    Q_OBJECT
private slots:
    void saving_preserves_rows_indexes_and_text() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        TextDocumentModel model;
        const QString text=QString::fromUtf8("alpha\nβeta\n한글 🎵\nomega\n");
        model.setText(text);
        model.moveCursor(1,2);
        QPersistentModelIndex index(model.index(2));
        QSignalSpy reset(&model,&QAbstractItemModel::modelReset);
        QSignalSpy changed(&model,&QAbstractItemModel::dataChanged);
        QSignalSpy storage(&model,&TextDocumentModel::lineStorageChanged);
        const auto path=directory.filePath("document.txt");
        QVERIFY(model.saveFile(path));
        {
            iiFileProvider::FileDevice bytes(path);
            QVERIFY(bytes.open(QIODevice::ReadOnly));
            QCOMPARE(bytes.readAll(), text.toUtf8());
        }
        QCOMPARE(reset.size(),0);QCOMPARE(changed.size(),0);QCOMPARE(storage.size(),1);
        QVERIFY(index.isValid());QCOMPARE(index.row(),2);
        QCOMPARE(index.data(TextDocumentModel::TextRole).toString(),QString::fromUtf8("한글 🎵"));
        QCOMPARE(model.text(),text);QCOMPARE(model.memoryLineCount(),0);
        QCOMPARE(model.fileBackedLineCount(),model.lineCount());
        QCOMPARE(model.cursorLine(),1);QCOMPARE(model.cursorColumn(),2);
        model.insertText("UPDATED");
        const auto edited=model.text();
        reset.clear();changed.clear();
        QVERIFY(model.saveFile());
        QCOMPARE(reset.size(),0);QCOMPARE(changed.size(),0);
        QVERIFY(index.isValid());QCOMPARE(model.text(),edited);
        iiFileProvider::FileDevice file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(QString::fromUtf8(file.readAll()),edited);
        file.close();
        const auto copied=directory.filePath("copy.txt");
        bool observed=false;
        connect(&model,&TextDocumentModel::filePathChanged,&model,[&] {
            observed=true;QCOMPARE(model.text(),edited);
        });
        QVERIFY(model.saveFile(copied));QVERIFY(observed);QVERIFY(index.isValid());
    }
    void saving_rejects_missing_or_truncated_source() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        for (bool missing : {false,true}) {
            TextDocumentModel model;model.setText("alpha\nbeta");
            const auto source=directory.filePath(missing ? "missing.txt" : "short.txt");
            QVERIFY(model.saveFile(source));
            if (missing) QVERIFY(iiFileProvider::FileDevice::remove(source));
            else {
                iiFileProvider::FileDevice file(source);QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));
                QCOMPARE(file.write("a"),1);file.close();
            }
            const auto target=directory.filePath(missing ? "missing-target.txt" : "short-target.txt");
            QSignalSpy reset(&model,&QAbstractItemModel::modelReset);
            QVERIFY(!model.saveFile(target));QCOMPARE(reset.size(),0);
            QVERIFY(!QFileInfo::exists(target));
        }
    }
};
QTEST_GUILESS_MAIN(TextDocumentStorageTests)
#include "tst_text_document_model.moc"
