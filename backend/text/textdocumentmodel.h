#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QFile>
#include <QStringDecoder>
#include <QStringList>
#include <QVector>
#include <QtQml/qqml.h>

#include <memory>

class TextDocumentModel : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TextDocumentModel)

    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(bool hasFilePath READ hasFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qint64 characterCount READ characterCount NOTIFY textChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY lineCountChanged)
    Q_PROPERTY(int fileBackedLineCount READ fileBackedLineCount NOTIFY lineStorageChanged)
    Q_PROPERTY(int memoryLineCount READ memoryLineCount NOTIFY lineStorageChanged)
    Q_PROPERTY(int cursorLine READ cursorLine WRITE setCursorLine NOTIFY cursorChanged)
    Q_PROPERTY(int cursorColumn READ cursorColumn WRITE setCursorColumn NOTIFY cursorChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(qint64 loadedByteCount READ loadedByteCount NOTIFY loadProgressChanged)
    Q_PROPERTY(qint64 totalByteCount READ totalByteCount NOTIFY loadProgressChanged)
    Q_PROPERTY(qreal loadProgress READ loadProgress NOTIFY loadProgressChanged)
    Q_PROPERTY(int loadChunkSize READ loadChunkSize WRITE setLoadChunkSize NOTIFY loadChunkSizeChanged)

public:
    enum Roles {
        LineIndexRole = Qt::UserRole + 1,
        LineNumberRole,
        TextRole,
        LengthRole
    };
    Q_ENUM(Roles)

    explicit TextDocumentModel(QObject *parent = nullptr);

    QString filePath() const;
    void setFilePath(const QString &path);
    bool hasFilePath() const;

    QString text() const;
    void setText(const QString &text);
    qint64 characterCount() const;

    int lineCount() const;
    int fileBackedLineCount() const;
    int memoryLineCount() const;
    int cursorLine() const;
    void setCursorLine(int line);
    int cursorColumn() const;
    void setCursorColumn(int column);
    int cursorPosition() const;
    void setCursorPosition(int position);
    bool dirty() const;
    QString lastError() const;
    bool loading() const;
    qint64 loadedByteCount() const;
    qint64 totalByteCount() const;
    qreal loadProgress() const;
    int loadChunkSize() const;
    void setLoadChunkSize(int size);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QString lineText(int line) const;
    Q_INVOKABLE int lineLength(int line) const;
    Q_INVOKABLE QString textRange(int start, int end) const;
    Q_INVOKABLE int positionForLineColumn(int line, int column) const;
    Q_INVOKABLE int previousWordBoundaryPosition(int position) const;
    Q_INVOKABLE int nextWordBoundaryPosition(int position) const;
    Q_INVOKABLE bool loadFile(const QString &path = QString());
    Q_INVOKABLE bool saveFile(const QString &path = QString());
    Q_INVOKABLE bool reloadFile();
    Q_INVOKABLE void cancelLoad();
    Q_INVOKABLE void markClean();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void moveCursor(int line, int column);
    Q_INVOKABLE void moveCursorLeft();
    Q_INVOKABLE void moveCursorRight();
    Q_INVOKABLE void moveCursorUp();
    Q_INVOKABLE void moveCursorDown();
    Q_INVOKABLE void moveCursorLineStart();
    Q_INVOKABLE void moveCursorLineEnd();
    Q_INVOKABLE void moveCursorDocumentStart();
    Q_INVOKABLE void moveCursorDocumentEnd();
    Q_INVOKABLE void moveCursorWordLeft();
    Q_INVOKABLE void moveCursorWordRight();
    Q_INVOKABLE void insertText(const QString &value);
    Q_INVOKABLE void insertNewline();
    Q_INVOKABLE void replaceRange(int start, int end, const QString &value);
    Q_INVOKABLE bool removePreviousCharacter();
    Q_INVOKABLE bool removeNextCharacter();

signals:
    void filePathChanged();
    void textChanged();
    void lineCountChanged();
    void lineStorageChanged();
    void cursorChanged();
    void dirtyChanged();
    void lastErrorChanged();
    void loadingChanged();
    void loadProgressChanged();
    void loadChunkSizeChanged();
    void fileLoadProgress(const QString &path, qint64 loadedBytes, qint64 totalBytes);
    void fileLoaded(const QString &path, int length);
    void fileLoadFailed(const QString &path, const QString &error);
    void fileSaved(const QString &path, int length);
    void fileSaveFailed(const QString &path, const QString &error);

private:
    struct LineRecord {
        qint64 byteOffset = 0;
        qint64 byteLength = 0;
        int length = 0;
        QString text;
        bool fileBacked = false;
    };

    static QString normalizeNewlines(const QString &text);
    static QStringList splitLines(const QString &text);
    static QString decodeUtf8Line(const QByteArray &data);
    static LineRecord memoryLine(const QString &text);
    static LineRecord fileLine(qint64 byteOffset, qint64 byteLength, int length);
    static bool isWordCharacter(QChar character);
    static int previousGraphemeBoundary(const QString &text, int position);
    static int nextGraphemeBoundary(const QString &text, int position);
    QString resolvedPath(const QString &path) const;
    QString lineTextForRecord(const LineRecord &record) const;
    void replaceLineRecord(int line, const LineRecord &record);
    void insertLineRecords(int firstLine, const QVector<LineRecord> &records);
    void removeLineRecord(int line);
    void replaceText(const QString &text, bool dirty);
    void resetLoadedText(bool dirty);
    void loadNextChunk();
    void scanLoadedBytes(const QByteArray &data, qint64 chunkStartOffset);
    void finalizeLoadedLine(qint64 byteOffset, const QByteArray &lineBytes);
    void finishLoad();
    void failLoad(const QString &path, const QString &error);
    bool writeModelToFile(const QString &path, QString *error);
    int boundedCharacterCount() const;
    QPair<int, int> lineColumnForPosition(int position) const;
    void setDirty(bool dirty);
    void setLastError(const QString &error);
    void setLoading(bool loading);
    void setLoadedByteCount(qint64 count);
    void setTotalByteCount(qint64 count);
    void setCharacterCount(qint64 count);
    void updateCharacterCountFromLines();
    void clampCursor();
    int lineStartPosition(int line) const;
    void emitLineChanged(int line);

    QString m_filePath;
    QVector<LineRecord> m_lines { memoryLine(QString()) };
    qint64 m_characterCount = 0;
    int m_cursorLine = 0;
    int m_cursorColumn = 0;
    bool m_dirty = false;
    QString m_lastError;
    bool m_loading = false;
    qint64 m_loadedByteCount = 0;
    qint64 m_totalByteCount = 0;
    int m_loadChunkSize = 64 * 1024;
    QString m_loadingPath;
    std::unique_ptr<QFile> m_loadingFile;
    bool m_loadPendingCarriageReturn = false;
    QByteArray m_pendingLineBytes;
    qint64 m_pendingLineStartOffset = 0;
};
