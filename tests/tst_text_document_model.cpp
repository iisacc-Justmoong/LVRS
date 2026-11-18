#include "QtEventWait.h"
#include "backend/text/textdocumentmodel.h"

#include <iiFileProviderDevice.h>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QPersistentModelIndex>
#include <QTemporaryDir>

namespace {

// Keep observations inside signal callbacks; assertions run after control has
// returned to the test, so failures never propagate through Qt signal delivery.
class ModelObservations {
public:
    explicit ModelObservations(TextDocumentModel &model) : model_(model)
    {
        connected_ &= bool(QObject::connect(&model, &TextDocumentModel::fileLoaded, &context_,
            [this](const QString &path, int length) { loaded.append({path, length}); }));
        connected_ &= bool(QObject::connect(&model, &QAbstractItemModel::modelReset, &context_,
            [this] { ++resetCount; }));
        connected_ &= bool(QObject::connect(&model, &QAbstractItemModel::dataChanged, &context_,
            [this](const QModelIndex &first, const QModelIndex &last, const QList<int> &) {
                ++changedCount;
                if (!first.isValid() || !last.isValid() || first.parent().isValid()
                    || last.parent().isValid() || first.row() > last.row()
                    || first.column() != 0 || last.column() != 0
                    || last.row() >= model_.rowCount())
                    recordError("invalid dataChanged range");
            }));
        connected_ &= bool(QObject::connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &context_,
            [this](const QModelIndex &parent, int first, int last) {
                beforeChange(parent, first, last, true);
            }));
        connected_ &= bool(QObject::connect(&model, &QAbstractItemModel::rowsInserted, &context_,
            [this](const QModelIndex &parent, int first, int last) {
                ++insertedCount;
                afterChange(parent, first, last, true);
            }));
        connected_ &= bool(QObject::connect(&model, &QAbstractItemModel::rowsAboutToBeRemoved, &context_,
            [this](const QModelIndex &parent, int first, int last) {
                beforeChange(parent, first, last, false);
            }));
        connected_ &= bool(QObject::connect(&model, &QAbstractItemModel::rowsRemoved, &context_,
            [this](const QModelIndex &parent, int first, int last) {
                ++removedCount;
                afterChange(parent, first, last, false);
            }));
        connected_ &= bool(QObject::connect(&model, &TextDocumentModel::lineStorageChanged, &context_,
            [this] { ++storageCount; }));
        connected_ &= bool(QObject::connect(&model, &TextDocumentModel::filePathChanged, &context_,
            [this] { textsAtPathChange.append(model_.text()); }));
    }

    void validateModel() const
    {
        CTEST_CHECK(connected_);
        CTEST_CHECK_MESSAGE(error_.isEmpty(), qPrintable(error_));
        CTEST_CHECK(!pending_);
        const int count = model_.rowCount();
        CTEST_COMPARE(count, model_.lineCount());
        CTEST_COMPARE(static_cast<const QAbstractItemModel &>(model_).columnCount(), 1);
        CTEST_CHECK(!model_.index(-1, 0).isValid());
        CTEST_CHECK(!model_.index(count, 0).isValid());
        CTEST_CHECK(!model_.data(QModelIndex()).isValid());
        for (int row = 0; row < count; ++row) {
            const QModelIndex index = model_.index(row, 0);
            CTEST_CHECK(index.isValid());
            CTEST_CHECK(model_.hasIndex(row, 0));
            CTEST_CHECK(!index.parent().isValid());
            CTEST_COMPARE(model_.rowCount(index), 0);
            CTEST_COMPARE(index.row(), row);
            CTEST_COMPARE(index.column(), 0);
            CTEST_COMPARE(index.data(TextDocumentModel::LineIndexRole).toInt(), row);
            CTEST_COMPARE(index.data(TextDocumentModel::LineNumberRole).toInt(), row + 1);
            CTEST_COMPARE(index.data(TextDocumentModel::TextRole).toString(), model_.lineText(row));
            CTEST_COMPARE(index.data(TextDocumentModel::LengthRole).toInt(), model_.lineLength(row));
        }
    }

    void clearCounts() { resetCount = changedCount = insertedCount = removedCount = storageCount = 0; }

    QList<QPair<QString, int>> loaded;
    QStringList textsAtPathChange;
    int resetCount = 0;
    int changedCount = 0;
    int insertedCount = 0;
    int removedCount = 0;
    int storageCount = 0;

private:
    void recordError(const char *message) { if (error_.isEmpty()) error_ = QString::fromLatin1(message); }

    void beforeChange(const QModelIndex &parent, int first, int last, bool inserting)
    {
        const int count = model_.rowCount();
        if (pending_ || parent.isValid() || first < 0 || last < first
            || (inserting ? first > count : last >= count))
            recordError("invalid rows-about-to-change notification");
        pending_ = true;
        inserting_ = inserting;
        first_ = first;
        last_ = last;
        oldCount_ = count;
    }

    void afterChange(const QModelIndex &parent, int first, int last, bool inserting)
    {
        const int delta = last - first + 1;
        if (!pending_ || parent.isValid() || inserting_ != inserting || first_ != first || last_ != last
            || model_.rowCount() != oldCount_ + (inserting ? delta : -delta))
            recordError("unbalanced rows-changed notification");
        pending_ = false;
    }

    TextDocumentModel &model_;
    QObject context_;
    bool connected_ = true;
    bool pending_ = false;
    bool inserting_ = false;
    int first_ = 0;
    int last_ = 0;
    int oldCount_ = 0;
    QString error_;
};

QTemporaryDir workspace()
{
    CTEST_CHECK(QDir().mkpath(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources")));
    return QTemporaryDir(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources/ctest-document-XXXXXX"));
}

void provider_chunked_load_and_cancellation()
{
    auto directory = workspace();
    CTEST_CHECK(directory.isValid());
    const auto path = directory.filePath("chunked.txt");
    const QString expected = QString::fromUtf8("한글 line\n").repeated(300);
    {
        iiFileProvider::FileDevice file(path);
        CTEST_CHECK(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        CTEST_COMPARE(file.write(expected.toUtf8()), expected.toUtf8().size());
    }
    TextDocumentModel model;
    ModelObservations observed(model);
    model.setLoadChunkSize(1024);
    CTEST_CHECK(model.loadFile(path));
    CTEST_WAIT_COMPARE_TIMEOUT(observed.loaded.size(), 1, 5000);
    CTEST_COMPARE(observed.loaded.constFirst().first, path);
    CTEST_COMPARE(observed.loaded.constFirst().second, expected.size());
    CTEST_COMPARE(model.text(), expected);
    CTEST_CHECK(!model.loading());
    observed.validateModel();
    CTEST_CHECK(model.loadFile(path));
    model.cancelLoad();
    CTEST_CHECK(!model.loading());
    QCoreApplication::processEvents();
    CTEST_COMPARE(observed.loaded.size(), 1);
    observed.validateModel();
}

void range_edits_preserve_unaffected_rows_and_counts()
{
    TextDocumentModel model;
    ModelObservations observed(model);
    model.setText(QString::fromUtf8("head\nalpha\nbravo\n한글 🎵\ntail"));
    observed.validateModel();
    observed.clearCounts();
    QPersistentModelIndex tail(model.index(4));
    model.replaceRange(6, 14, QStringLiteral("X\r\nY"));
    CTEST_COMPARE(model.text(), QString::fromUtf8("head\naX\nYvo\n한글 🎵\ntail"));
    CTEST_COMPARE(model.characterCount(), qint64(model.text().size()));
    CTEST_COMPARE(observed.resetCount, 0); CTEST_COMPARE(observed.insertedCount, 0); CTEST_COMPARE(observed.removedCount, 0);
    CTEST_CHECK(tail.isValid()); CTEST_COMPARE(tail.row(), 4);
    CTEST_COMPARE(tail.data(TextDocumentModel::TextRole).toString(), QStringLiteral("tail"));
    observed.validateModel();

    model.replaceRange(5, 9, QStringLiteral("Z"));
    CTEST_COMPARE(model.text(), QString::fromUtf8("head\nZvo\n한글 🎵\ntail"));
    CTEST_COMPARE(model.characterCount(), qint64(model.text().size()));
    CTEST_COMPARE(observed.resetCount, 0); CTEST_COMPARE(observed.removedCount, 1);
    CTEST_CHECK(tail.isValid()); CTEST_COMPARE(tail.row(), 3);
    observed.validateModel();

    model.moveCursor(1, 1); model.insertText(QStringLiteral("a\r\nb"));
    CTEST_COMPARE(model.text(), QString::fromUtf8("head\nZa\nbvo\n한글 🎵\ntail"));
    CTEST_COMPARE(model.characterCount(), qint64(model.text().size()));
    CTEST_CHECK(tail.isValid()); CTEST_COMPARE(tail.row(), 4);
    observed.validateModel();
    CTEST_CHECK(model.removePreviousCharacter());
    CTEST_COMPARE(model.characterCount(), qint64(model.text().size()));
    CTEST_COMPARE(observed.resetCount, 0);
    observed.validateModel();
}

void saving_preserves_rows_indexes_and_text()
{
    auto directory = workspace();
    CTEST_CHECK(directory.isValid());
    TextDocumentModel model;
    const QString text = QString::fromUtf8("alpha\nβeta\n한글 🎵\nomega\n");
    model.setText(text);
    model.moveCursor(1, 2);
    QPersistentModelIndex index(model.index(2));
    ModelObservations observed(model);
    const auto path = directory.filePath("document.txt");
    CTEST_CHECK(model.saveFile(path));
    {
        iiFileProvider::FileDevice bytes(path);
        CTEST_CHECK(bytes.open(QIODevice::ReadOnly));
        CTEST_COMPARE(bytes.readAll(), text.toUtf8());
    }
    CTEST_COMPARE(observed.resetCount, 0); CTEST_COMPARE(observed.changedCount, 0); CTEST_COMPARE(observed.storageCount, 1);
    CTEST_CHECK(index.isValid()); CTEST_COMPARE(index.row(), 2);
    CTEST_COMPARE(index.data(TextDocumentModel::TextRole).toString(), QString::fromUtf8("한글 🎵"));
    CTEST_COMPARE(model.text(), text); CTEST_COMPARE(model.memoryLineCount(), 0);
    CTEST_COMPARE(model.fileBackedLineCount(), model.lineCount());
    CTEST_COMPARE(model.cursorLine(), 1); CTEST_COMPARE(model.cursorColumn(), 2);
    observed.validateModel();
    model.insertText("UPDATED");
    const auto edited = model.text();
    observed.validateModel();
    observed.clearCounts();
    CTEST_CHECK(model.saveFile());
    CTEST_COMPARE(observed.resetCount, 0); CTEST_COMPARE(observed.changedCount, 0);
    CTEST_CHECK(index.isValid()); CTEST_COMPARE(model.text(), edited);
    iiFileProvider::FileDevice file(path);
    CTEST_CHECK(file.open(QIODevice::ReadOnly)); CTEST_COMPARE(QString::fromUtf8(file.readAll()), edited);
    file.close();
    const auto copied = directory.filePath("copy.txt");
    observed.textsAtPathChange.clear();
    CTEST_CHECK(model.saveFile(copied)); CTEST_CHECK(!observed.textsAtPathChange.isEmpty());
    CTEST_COMPARE(observed.textsAtPathChange.constLast(), edited); CTEST_CHECK(index.isValid());
    observed.validateModel();
}

void saving_rejects_missing_or_truncated_source()
{
    auto directory = workspace();
    CTEST_CHECK(directory.isValid());
    for (bool missing : {false, true}) {
        TextDocumentModel model; model.setText("alpha\nbeta");
        const auto source = directory.filePath(missing ? "missing.txt" : "short.txt");
        CTEST_CHECK(model.saveFile(source));
        if (missing) CTEST_CHECK(iiFileProvider::FileDevice::remove(source));
        else {
            iiFileProvider::FileDevice file(source); CTEST_CHECK(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            CTEST_COMPARE(file.write("a"), 1); file.close();
        }
        const auto target = directory.filePath(missing ? "missing-target.txt" : "short-target.txt");
        ModelObservations observed(model);
        CTEST_CHECK(!model.saveFile(target)); CTEST_COMPARE(observed.resetCount, 0);
        CTEST_CHECK(!QFileInfo::exists(target));
        // The intentionally absent source can no longer supply file-backed row data.
        CTEST_CHECK(observed.textsAtPathChange.isEmpty());
    }
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner;
    runner.run("provider_chunked_load_and_cancellation", provider_chunked_load_and_cancellation);
    runner.run("range_edits_preserve_unaffected_rows_and_counts", range_edits_preserve_unaffected_rows_and_counts);
    runner.run("saving_preserves_rows_indexes_and_text", saving_preserves_rows_indexes_and_text);
    runner.run("saving_rejects_missing_or_truncated_source", saving_rejects_missing_or_truncated_source);
    return runner.result();
}
