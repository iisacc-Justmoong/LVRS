#include "backend/text/textdocumentmodel.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTimer>

#include <limits>

TextDocumentModel::TextDocumentModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QString TextDocumentModel::filePath() const
{
    return m_filePath;
}

void TextDocumentModel::setFilePath(const QString &path)
{
    if (m_filePath == path)
        return;
    m_filePath = path;
    emit filePathChanged();
}

bool TextDocumentModel::hasFilePath() const
{
    return !m_filePath.trimmed().isEmpty();
}

QString TextDocumentModel::text() const
{
    QStringList lines;
    lines.reserve(m_lines.size());
    for (const LineRecord &line : m_lines)
        lines.append(lineTextForRecord(line));
    return lines.join(QLatin1Char('\n'));
}

void TextDocumentModel::setText(const QString &text)
{
    cancelLoad();
    replaceText(text, true);
}

qint64 TextDocumentModel::characterCount() const
{
    return m_characterCount;
}

int TextDocumentModel::lineCount() const
{
    return m_lines.size();
}

int TextDocumentModel::fileBackedLineCount() const
{
    int count = 0;
    for (const LineRecord &line : m_lines) {
        if (line.fileBacked)
            ++count;
    }
    return count;
}

int TextDocumentModel::memoryLineCount() const
{
    return m_lines.size() - fileBackedLineCount();
}

int TextDocumentModel::cursorLine() const
{
    return m_cursorLine;
}

void TextDocumentModel::setCursorLine(int line)
{
    const int nextLine = qBound(0, line, lineCount() - 1);
    if (m_cursorLine == nextLine)
        return;
    m_cursorLine = nextLine;
    m_cursorColumn = qBound(0, m_cursorColumn, lineLength(m_cursorLine));
    emit cursorChanged();
}

int TextDocumentModel::cursorColumn() const
{
    return m_cursorColumn;
}

void TextDocumentModel::setCursorColumn(int column)
{
    const int nextColumn = qBound(0, column, lineLength(m_cursorLine));
    if (m_cursorColumn == nextColumn)
        return;
    m_cursorColumn = nextColumn;
    emit cursorChanged();
}

int TextDocumentModel::cursorPosition() const
{
    return lineStartPosition(m_cursorLine) + m_cursorColumn;
}

void TextDocumentModel::setCursorPosition(int position)
{
    int remaining = qMax(0, position);
    int nextLine = 0;
    for (; nextLine < m_lines.size(); ++nextLine) {
        const int length = m_lines.at(nextLine).length;
        if (remaining <= length)
            break;
        remaining -= length;
        if (nextLine < m_lines.size() - 1)
            remaining -= 1;
    }

    nextLine = qBound(0, nextLine, lineCount() - 1);
    const int nextColumn = qBound(0, remaining, lineLength(nextLine));
    if (m_cursorLine == nextLine && m_cursorColumn == nextColumn)
        return;
    m_cursorLine = nextLine;
    m_cursorColumn = nextColumn;
    emit cursorChanged();
}

bool TextDocumentModel::dirty() const
{
    return m_dirty;
}

QString TextDocumentModel::lastError() const
{
    return m_lastError;
}

bool TextDocumentModel::loading() const
{
    return m_loading;
}

qint64 TextDocumentModel::loadedByteCount() const
{
    return m_loadedByteCount;
}

qint64 TextDocumentModel::totalByteCount() const
{
    return m_totalByteCount;
}

qreal TextDocumentModel::loadProgress() const
{
    if (m_totalByteCount <= 0)
        return m_loading ? 0.0 : 1.0;
    return qBound<qreal>(0.0, static_cast<qreal>(m_loadedByteCount) / static_cast<qreal>(m_totalByteCount), 1.0);
}

int TextDocumentModel::loadChunkSize() const
{
    return m_loadChunkSize;
}

void TextDocumentModel::setLoadChunkSize(int size)
{
    const int nextSize = qBound(1024, size, 16 * 1024 * 1024);
    if (m_loadChunkSize == nextSize)
        return;
    m_loadChunkSize = nextSize;
    emit loadChunkSizeChanged();
}

int TextDocumentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_lines.size();
}

QVariant TextDocumentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size())
        return QVariant();

    const LineRecord &line = m_lines.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case TextRole:
        return lineTextForRecord(line);
    case LineIndexRole:
        return index.row();
    case LineNumberRole:
        return index.row() + 1;
    case LengthRole:
        return line.length;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TextDocumentModel::roleNames() const
{
    return {
        { LineIndexRole, QByteArrayLiteral("lineIndex") },
        { LineNumberRole, QByteArrayLiteral("lineNumber") },
        { TextRole, QByteArrayLiteral("text") },
        { LengthRole, QByteArrayLiteral("length") }
    };
}

QString TextDocumentModel::lineText(int line) const
{
    if (line < 0 || line >= m_lines.size())
        return QString();
    return lineTextForRecord(m_lines.at(line));
}

int TextDocumentModel::lineLength(int line) const
{
    if (line < 0 || line >= m_lines.size())
        return 0;
    return m_lines.at(line).length;
}

bool TextDocumentModel::loadFile(const QString &path)
{
    const QString targetPath = resolvedPath(path);
    if (targetPath.trimmed().isEmpty()) {
        const QString error = QStringLiteral("Empty path");
        setLastError(error);
        emit fileLoadFailed(targetPath, error);
        return false;
    }

    auto file = std::make_unique<QFile>(targetPath);
    if (!file->open(QIODevice::ReadOnly)) {
        if (!QFileInfo::exists(targetPath)) {
            cancelLoad();
            m_loadingPath = targetPath;
            setFilePath(targetPath);
            setTotalByteCount(0);
            setLoadedByteCount(0);
            setLoading(true);
            resetLoadedText(false);
            setLastError(QString());
            m_loadingPath.clear();
            setLoading(false);
            emit fileLoaded(targetPath, boundedCharacterCount());
            return true;
        }

        const QString error = file->errorString();
        setLastError(error);
        emit fileLoadFailed(targetPath, error);
        return false;
    }

    cancelLoad();
    m_loadingPath = targetPath;
    m_loadingFile = std::move(file);
    m_loadPendingCarriageReturn = false;
    setFilePath(targetPath);
    setTotalByteCount(m_loadingFile->size());
    setLoadedByteCount(0);
    setLoading(true);
    resetLoadedText(false);
    setLastError(QString());
    QTimer::singleShot(0, this, &TextDocumentModel::loadNextChunk);
    return true;
}

bool TextDocumentModel::saveFile(const QString &path)
{
    const QString targetPath = resolvedPath(path);
    if (targetPath.trimmed().isEmpty()) {
        const QString error = QStringLiteral("Empty path");
        setLastError(error);
        emit fileSaveFailed(targetPath, error);
        return false;
    }

    if (m_loading) {
        const QString error = QStringLiteral("Cannot save while file is still loading");
        setLastError(error);
        emit fileSaveFailed(targetPath, error);
        return false;
    }

    QString error;
    if (!writeModelToFile(targetPath, &error)) {
        setLastError(error);
        emit fileSaveFailed(targetPath, error);
        return false;
    }

    setDirty(false);
    setLastError(QString());
    emit fileSaved(targetPath, boundedCharacterCount());
    return true;
}

bool TextDocumentModel::reloadFile()
{
    return loadFile(m_filePath);
}

void TextDocumentModel::cancelLoad()
{
    if (!m_loading && !m_loadingFile)
        return;

    m_loadingFile.reset();
    m_loadingPath.clear();
    m_loadPendingCarriageReturn = false;
    setLoading(false);
}

void TextDocumentModel::markClean()
{
    setDirty(false);
    setLastError(QString());
}

void TextDocumentModel::clear()
{
    cancelLoad();
    replaceText(QString(), true);
}

void TextDocumentModel::moveCursor(int line, int column)
{
    const int nextLine = qBound(0, line, lineCount() - 1);
    const int nextColumn = qBound(0, column, lineLength(nextLine));
    if (m_cursorLine == nextLine && m_cursorColumn == nextColumn)
        return;
    m_cursorLine = nextLine;
    m_cursorColumn = nextColumn;
    emit cursorChanged();
}

void TextDocumentModel::moveCursorLeft()
{
    if (m_cursorColumn > 0) {
        moveCursor(m_cursorLine, m_cursorColumn - 1);
        return;
    }
    if (m_cursorLine > 0)
        moveCursor(m_cursorLine - 1, lineLength(m_cursorLine - 1));
}

void TextDocumentModel::moveCursorRight()
{
    if (m_cursorColumn < lineLength(m_cursorLine)) {
        moveCursor(m_cursorLine, m_cursorColumn + 1);
        return;
    }
    if (m_cursorLine < lineCount() - 1)
        moveCursor(m_cursorLine + 1, 0);
}

void TextDocumentModel::moveCursorUp()
{
    if (m_cursorLine > 0)
        moveCursor(m_cursorLine - 1, m_cursorColumn);
}

void TextDocumentModel::moveCursorDown()
{
    if (m_cursorLine < lineCount() - 1)
        moveCursor(m_cursorLine + 1, m_cursorColumn);
}

void TextDocumentModel::moveCursorLineStart()
{
    moveCursor(m_cursorLine, 0);
}

void TextDocumentModel::moveCursorLineEnd()
{
    moveCursor(m_cursorLine, lineLength(m_cursorLine));
}

void TextDocumentModel::insertText(const QString &value)
{
    if (value.isEmpty())
        return;

    cancelLoad();
    const QStringList insertLines = splitLines(value);
    const QString current = lineTextForRecord(m_lines.at(m_cursorLine));
    const QString before = current.left(m_cursorColumn);
    const QString after = current.mid(m_cursorColumn);

    if (insertLines.size() == 1) {
        replaceLineRecord(m_cursorLine, memoryLine(before + insertLines.first() + after));
        m_cursorColumn += insertLines.first().size();
    } else {
        const int firstLine = m_cursorLine;
        const int newLineCount = insertLines.size();
        replaceLineRecord(firstLine, memoryLine(before + insertLines.first()));
        QVector<LineRecord> insertedRecords;
        insertedRecords.reserve(newLineCount - 1);
        for (int i = 1; i < newLineCount; ++i) {
            QString line = insertLines.at(i);
            if (i == newLineCount - 1)
                line.append(after);
            insertedRecords.append(memoryLine(line));
        }
        insertLineRecords(firstLine + 1, insertedRecords);
        m_cursorLine = firstLine + newLineCount - 1;
        m_cursorColumn = insertLines.last().size();
    }

    setDirty(true);
    updateCharacterCountFromLines();
    emit textChanged();
    emit cursorChanged();
}

void TextDocumentModel::insertNewline()
{
    insertText(QStringLiteral("\n"));
}

bool TextDocumentModel::removePreviousCharacter()
{
    cancelLoad();
    if (m_cursorColumn > 0) {
        QString line = lineTextForRecord(m_lines.at(m_cursorLine));
        line.remove(m_cursorColumn - 1, 1);
        replaceLineRecord(m_cursorLine, memoryLine(line));
        m_cursorColumn -= 1;
    } else if (m_cursorLine > 0) {
        const int previousLine = m_cursorLine - 1;
        const int previousLength = lineLength(previousLine);
        const QString merged = lineTextForRecord(m_lines.at(previousLine)) + lineTextForRecord(m_lines.at(m_cursorLine));
        replaceLineRecord(previousLine, memoryLine(merged));
        removeLineRecord(m_cursorLine);
        m_cursorLine = previousLine;
        m_cursorColumn = previousLength;
    } else {
        return false;
    }

    setDirty(true);
    updateCharacterCountFromLines();
    emit textChanged();
    emit cursorChanged();
    return true;
}

bool TextDocumentModel::removeNextCharacter()
{
    cancelLoad();
    if (m_cursorColumn < lineLength(m_cursorLine)) {
        QString line = lineTextForRecord(m_lines.at(m_cursorLine));
        line.remove(m_cursorColumn, 1);
        replaceLineRecord(m_cursorLine, memoryLine(line));
    } else if (m_cursorLine < lineCount() - 1) {
        const QString merged = lineTextForRecord(m_lines.at(m_cursorLine)) + lineTextForRecord(m_lines.at(m_cursorLine + 1));
        replaceLineRecord(m_cursorLine, memoryLine(merged));
        removeLineRecord(m_cursorLine + 1);
    } else {
        return false;
    }

    setDirty(true);
    updateCharacterCountFromLines();
    emit textChanged();
    emit cursorChanged();
    return true;
}

QString TextDocumentModel::normalizeNewlines(const QString &text)
{
    QString normalized = text;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QChar('\r'), QChar('\n'));
    return normalized;
}

QStringList TextDocumentModel::splitLines(const QString &text)
{
    QStringList lines = normalizeNewlines(text).split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    if (lines.isEmpty())
        lines.append(QString());
    return lines;
}

QString TextDocumentModel::decodeUtf8Line(const QByteArray &data)
{
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString decoded = decoder(QByteArrayView(data));
    if (decoder.hasError())
        return QString::fromUtf8(data);
    return decoded;
}

TextDocumentModel::LineRecord TextDocumentModel::memoryLine(const QString &text)
{
    LineRecord record;
    record.length = text.size();
    record.text = text;
    record.fileBacked = false;
    return record;
}

TextDocumentModel::LineRecord TextDocumentModel::fileLine(qint64 byteOffset, qint64 byteLength, int length)
{
    LineRecord record;
    record.byteOffset = qMax<qint64>(0, byteOffset);
    record.byteLength = qMax<qint64>(0, byteLength);
    record.length = qMax(0, length);
    record.fileBacked = true;
    return record;
}

QString TextDocumentModel::resolvedPath(const QString &path) const
{
    if (path.isNull() || path.isEmpty())
        return m_filePath;
    return path;
}

QString TextDocumentModel::lineTextForRecord(const LineRecord &record) const
{
    if (!record.fileBacked)
        return record.text;

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    if (!file.seek(record.byteOffset))
        return QString();

    return decodeUtf8Line(file.read(record.byteLength));
}

void TextDocumentModel::replaceLineRecord(int line, const LineRecord &record)
{
    if (line < 0 || line >= m_lines.size())
        return;
    m_lines[line] = record;
    emitLineChanged(line);
    emit lineStorageChanged();
}

void TextDocumentModel::insertLineRecords(int firstLine, const QVector<LineRecord> &records)
{
    if (records.isEmpty())
        return;

    const int boundedFirstLine = qBound(0, firstLine, m_lines.size());
    beginInsertRows(QModelIndex(), boundedFirstLine, boundedFirstLine + records.size() - 1);
    for (int i = 0; i < records.size(); ++i)
        m_lines.insert(boundedFirstLine + i, records.at(i));
    endInsertRows();
    emit lineCountChanged();
    emit lineStorageChanged();
}

void TextDocumentModel::removeLineRecord(int line)
{
    if (line < 0 || line >= m_lines.size())
        return;
    beginRemoveRows(QModelIndex(), line, line);
    m_lines.removeAt(line);
    endRemoveRows();
    if (m_lines.isEmpty())
        m_lines.append(memoryLine(QString()));
    emit lineCountChanged();
    emit lineStorageChanged();
}

void TextDocumentModel::replaceText(const QString &text, bool dirty)
{
    const QStringList lines = splitLines(text);
    QVector<LineRecord> records;
    records.reserve(lines.size());
    for (const QString &line : lines)
        records.append(memoryLine(line));

    beginResetModel();
    m_lines = records;
    m_cursorLine = 0;
    m_cursorColumn = 0;
    endResetModel();
    updateCharacterCountFromLines();
    setDirty(dirty);
    emit textChanged();
    emit lineCountChanged();
    emit cursorChanged();
}

void TextDocumentModel::resetLoadedText(bool dirty)
{
    beginResetModel();
    m_lines = { memoryLine(QString()) };
    m_cursorLine = 0;
    m_cursorColumn = 0;
    m_characterCount = 0;
    m_pendingLineBytes.clear();
    m_pendingLineStartOffset = 0;
    endResetModel();
    setDirty(dirty);
    emit textChanged();
    emit lineCountChanged();
    emit cursorChanged();
}

void TextDocumentModel::loadNextChunk()
{
    if (!m_loading || !m_loadingFile)
        return;

    const qint64 chunkStartOffset = m_loadingFile->pos();
    const QByteArray chunk = m_loadingFile->read(m_loadChunkSize);
    if (chunk.isEmpty() && m_loadingFile->error() != QFileDevice::NoError) {
        failLoad(m_loadingPath, m_loadingFile->errorString());
        return;
    }

    if (!chunk.isEmpty())
        scanLoadedBytes(chunk, chunkStartOffset);

    setLoadedByteCount(m_loadingFile->pos());
    emit fileLoadProgress(m_loadingPath, m_loadedByteCount, m_totalByteCount);

    if (m_loadingFile->atEnd()) {
        finalizeLoadedLine(m_pendingLineStartOffset, m_pendingLineBytes);
        m_pendingLineBytes.clear();
        finishLoad();
        return;
    }

    QTimer::singleShot(0, this, &TextDocumentModel::loadNextChunk);
}

void TextDocumentModel::scanLoadedBytes(const QByteArray &data, qint64 chunkStartOffset)
{
    for (int i = 0; i < data.size(); ++i) {
        const char byte = data.at(i);
        const qint64 absoluteOffset = chunkStartOffset + i;

        if (m_loadPendingCarriageReturn) {
            m_loadPendingCarriageReturn = false;
            if (byte == '\n') {
                m_pendingLineStartOffset = absoluteOffset + 1;
                continue;
            }
        }

        if (byte == '\r') {
            finalizeLoadedLine(m_pendingLineStartOffset, m_pendingLineBytes);
            m_pendingLineBytes.clear();
            m_pendingLineStartOffset = absoluteOffset + 1;
            m_loadPendingCarriageReturn = true;
            continue;
        }

        if (byte == '\n') {
            finalizeLoadedLine(m_pendingLineStartOffset, m_pendingLineBytes);
            m_pendingLineBytes.clear();
            m_pendingLineStartOffset = absoluteOffset + 1;
            continue;
        }

        m_pendingLineBytes.append(byte);
    }
}

void TextDocumentModel::finalizeLoadedLine(qint64 byteOffset, const QByteArray &lineBytes)
{
    const QString decoded = decodeUtf8Line(lineBytes);
    const LineRecord record = fileLine(byteOffset, lineBytes.size(), decoded.size());

    if (m_lines.size() == 1
            && !m_lines.first().fileBacked
            && m_lines.first().text.isEmpty()
            && m_characterCount == 0) {
        m_lines[0] = record;
        emitLineChanged(0);
        emit lineStorageChanged();
    } else {
        const int firstRow = m_lines.size();
        beginInsertRows(QModelIndex(), firstRow, firstRow);
        m_lines.append(record);
        endInsertRows();
        emit lineCountChanged();
        emit lineStorageChanged();
    }

    setCharacterCount(m_characterCount + decoded.size() + (m_lines.size() > 1 ? 1 : 0));
    emit textChanged();
}

void TextDocumentModel::finishLoad()
{
    const QString loadedPath = m_loadingPath;
    m_loadingFile.reset();
    m_loadingPath.clear();
    m_loadPendingCarriageReturn = false;
    setLoadedByteCount(m_totalByteCount);
    setDirty(false);
    setLastError(QString());
    setLoading(false);
    emit fileLoaded(loadedPath, boundedCharacterCount());
}

void TextDocumentModel::failLoad(const QString &path, const QString &error)
{
    m_loadingFile.reset();
    m_loadingPath.clear();
    m_loadPendingCarriageReturn = false;
    setLoading(false);
    setLastError(error);
    emit fileLoadFailed(path, error);
}

bool TextDocumentModel::writeModelToFile(const QString &path, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    QVector<LineRecord> savedRecords;
    savedRecords.reserve(m_lines.size());
    qint64 writtenBytes = 0;

    for (int i = 0; i < m_lines.size(); ++i) {
        if (i > 0) {
            if (file.write("\n", 1) != 1) {
                if (error)
                    *error = file.errorString();
                file.cancelWriting();
                return false;
            }
            writtenBytes += 1;
        }

        const QString line = lineTextForRecord(m_lines.at(i));
        const QByteArray data = line.toUtf8();
        const qint64 lineOffset = writtenBytes;
        if (file.write(data) != data.size()) {
            if (error)
                *error = file.errorString();
            file.cancelWriting();
            return false;
        }

        savedRecords.append(fileLine(lineOffset, data.size(), line.size()));
        writtenBytes += data.size();
    }

    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }

    setFilePath(path);
    beginResetModel();
    m_lines = savedRecords.isEmpty() ? QVector<LineRecord> { memoryLine(QString()) } : savedRecords;
    endResetModel();
    emit lineStorageChanged();
    return true;
}

int TextDocumentModel::boundedCharacterCount() const
{
    return static_cast<int>(qMin<qint64>(m_characterCount, std::numeric_limits<int>::max()));
}

void TextDocumentModel::setDirty(bool dirty)
{
    if (m_dirty == dirty)
        return;
    m_dirty = dirty;
    emit dirtyChanged();
}

void TextDocumentModel::setLastError(const QString &error)
{
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}

void TextDocumentModel::setLoading(bool loading)
{
    if (m_loading == loading)
        return;
    m_loading = loading;
    emit loadingChanged();
}

void TextDocumentModel::setLoadedByteCount(qint64 count)
{
    const qint64 nextCount = qMax<qint64>(0, count);
    if (m_loadedByteCount == nextCount)
        return;
    m_loadedByteCount = nextCount;
    emit loadProgressChanged();
}

void TextDocumentModel::setTotalByteCount(qint64 count)
{
    const qint64 nextCount = qMax<qint64>(0, count);
    if (m_totalByteCount == nextCount)
        return;
    m_totalByteCount = nextCount;
    emit loadProgressChanged();
}

void TextDocumentModel::setCharacterCount(qint64 count)
{
    const qint64 nextCount = qMax<qint64>(0, count);
    if (m_characterCount == nextCount)
        return;
    m_characterCount = nextCount;
}

void TextDocumentModel::updateCharacterCountFromLines()
{
    qint64 count = 0;
    for (const LineRecord &line : m_lines)
        count += line.length;
    if (m_lines.size() > 1)
        count += m_lines.size() - 1;
    setCharacterCount(count);
}

void TextDocumentModel::clampCursor()
{
    m_cursorLine = qBound(0, m_cursorLine, lineCount() - 1);
    m_cursorColumn = qBound(0, m_cursorColumn, lineLength(m_cursorLine));
}

int TextDocumentModel::lineStartPosition(int line) const
{
    int position = 0;
    const int boundedLine = qBound(0, line, lineCount() - 1);
    for (int i = 0; i < boundedLine; ++i)
        position += m_lines.at(i).length + 1;
    return position;
}

void TextDocumentModel::emitLineChanged(int line)
{
    if (line < 0 || line >= m_lines.size())
        return;
    const QModelIndex changed = index(line, 0);
    emit dataChanged(changed, changed, { TextRole, LengthRole, Qt::DisplayRole, Qt::EditRole });
}
