#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QTextStream>

// Run from the repository root. Only SVG root attributes are rewritten;
// paths, colors, embedded images, masks, and clip paths stay byte-identical.
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QString root = app.arguments().value(1, QStringLiteral("."));
    QDirIterator files(root + "/resources", {"*.svg"}, QDir::Files, QDirIterator::Subdirectories);
    const QRegularExpression svgTag(QStringLiteral("<svg\\b[^>]*>"));
    auto attribute = [](QString tag, const QString &name, const QString &value) {
        const QRegularExpression pattern("\\s" + name + "=\"[^\"]*\"");
        tag.remove(pattern);
        tag.insert(tag.size() - 1, " " + name + "=\"" + value + "\"");
        return tag;
    };
    auto boxText = [](const QRectF &box) {
        return QStringLiteral("%1 %2 %3 %4").arg(box.x(), 0, 'g', 12)
            .arg(box.y(), 0, 'g', 12).arg(box.width(), 0, 'g', 12).arg(box.height(), 0, 'g', 12);
    };
    QHash<QString, QString> oldHashes;
    int count = 0;
    while (files.hasNext()) {
        QFile file(files.next());
        if (!file.open(QIODevice::ReadOnly)) return 1;
        const QByteArray original = file.readAll();
        file.close();
        QString svg = QString::fromUtf8(original);
        const auto match = svgTag.match(svg);
        QSvgRenderer initial(original);
        if (!match.hasMatch() || !initial.isValid()) return 2;
        const QRectF originalBox = initial.viewBoxF();
        // Probe outside the exported viewport as some Figma strokes overflow it.
        const QRectF probeBox = originalBox.adjusted(-originalBox.width(), -originalBox.height(),
                                                     originalBox.width(), originalBox.height());
        QString probe = svg;
        probe.replace(match.capturedStart(), match.capturedLength(),
                      attribute(match.captured(), "viewBox", boxText(probeBox)));
        QSvgRenderer renderer(probe.toUtf8());
        constexpr double resolution = 8.0;
        QImage image(qCeil(probeBox.width() * resolution), qCeil(probeBox.height() * resolution),
                     QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        renderer.render(&painter, QRectF(image.rect()));
        painter.end();
        QRect bounds;
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
                if (qAlpha(image.pixel(x, y)) > 0) bounds |= QRect(x, y, 1, 1);
        if (bounds.isEmpty()) return 3;
        const QPointF center(probeBox.x() + (bounds.x() + bounds.width() / 2.0) / resolution,
                             probeBox.y() + (bounds.y() + bounds.height() / 2.0) / resolution);
        const double side = qMax(qMax(originalBox.width(), originalBox.height()),
                                qMax(bounds.width(), bounds.height()) / resolution);
        // Keep the sampling origin on the same grid across repeated runs.
        // Otherwise half-pixel antialiasing can move the measured edge each time.
        const QRectF centered(qRound((center.x() - side / 2) * resolution) / resolution,
                              qRound((center.y() - side / 2) * resolution) / resolution, side, side);
        QString tag = attribute(match.captured(), "width", "100%");
        tag = attribute(tag, "height", "100%");
        tag = attribute(tag, "viewBox", boxText(centered));
        tag = attribute(tag, "preserveAspectRatio", "xMidYMid meet");
        svg.replace(match.capturedStart(), match.capturedLength(), tag);
        if (svg.toUtf8() != original) {
            oldHashes.insert(QFileInfo(file).absoluteFilePath(),
                QString::fromLatin1(QCryptographicHash::hash(original, QCryptographicHash::Sha256).toHex()));
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return 4;
            if (file.write(svg.toUtf8()) != svg.toUtf8().size()) return 5;
        }
        ++count;
    }
    QFile manifest(root + "/docs/figma-iconset-manifest.json");
    if (!manifest.open(QIODevice::ReadOnly)) return 6;
    auto inventory = QJsonDocument::fromJson(manifest.readAll()).object();
    manifest.close();
    auto icons = inventory["icons"].toArray();
    for (qsizetype i = 0; i < icons.size(); ++i) {
        auto icon = icons[i].toObject();
        const QString filename = icon["filename"].toString();
        QFile file(root + "/resources/iconset/" + filename);
        const QString absolutePath = QFileInfo(file).absoluteFilePath();
        if (!icon.contains("preResponsiveSha256") && oldHashes.contains(absolutePath))
            icon["preResponsiveSha256"] = oldHashes[absolutePath];
        if (!file.open(QIODevice::ReadOnly)) return 7;
        icon["sha256"] = QString::fromLatin1(
            QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex());
        icons[i] = icon;
    }
    inventory["icons"] = icons;
    if (!manifest.open(QIODevice::WriteOnly | QIODevice::Truncate)) return 8;
    manifest.write(QJsonDocument(inventory).toJson(QJsonDocument::Indented));
    QFile fixture(root + "/tests/fixtures/figma-contract.json");
    if (!fixture.open(QIODevice::ReadOnly)) return 9;
    QByteArray fixtureBytes = fixture.readAll();
    const auto contract = QJsonDocument::fromJson(fixtureBytes).object();
    fixture.close();
    auto assets = contract["assets"].toObject();
    for (auto it = assets.begin(); it != assets.end(); ++it) {
        if (!it.key().endsWith(".svg")) continue;
        QFile asset(root + "/resources/" + it.key());
        if (!asset.open(QIODevice::ReadOnly)) return 10;
        fixtureBytes.replace(it.value().toString().toLatin1(),
            QCryptographicHash::hash(asset.readAll(), QCryptographicHash::Sha256).toHex());
    }
    if (!fixture.open(QIODevice::WriteOnly | QIODevice::Truncate)) return 11;
    fixture.write(fixtureBytes);
    QTextStream(stdout) << "Checked " << count << " SVGs; updated " << oldHashes.size() << " roots.\n";
}
