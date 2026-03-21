#include <QtTest>

#include <QDir>
#include <QGuiApplication>
#include <QTemporaryDir>

#include "backend/runtime/appbootstrap.h"
#include "backend/runtime/appentry.h"

namespace {

class MessageCapture
{
public:
    MessageCapture()
        : m_previousHandler(qInstallMessageHandler(MessageCapture::handler))
    {
        s_activeCapture = this;
    }

    ~MessageCapture()
    {
        s_activeCapture = nullptr;
        qInstallMessageHandler(m_previousHandler);
    }

    QString joinedMessages() const
    {
        return m_messages.join(QLatin1Char('\n'));
    }

private:
    static void handler(QtMsgType type, const QMessageLogContext &, const QString &message)
    {
        if (!s_activeCapture)
            return;
        if (type == QtInfoMsg || type == QtWarningMsg || type == QtCriticalMsg)
            s_activeCapture->m_messages.append(message);
    }

    static MessageCapture *s_activeCapture;

    QtMessageHandler m_previousHandler = nullptr;
    QStringList m_messages;
};

MessageCapture *MessageCapture::s_activeCapture = nullptr;

} // namespace

class AppEntryTests : public QObject
{
    Q_OBJECT

private slots:
    void default_runtime_qml_import_paths_include_staged_bundle_dirs();
    void default_runtime_qml_import_paths_include_snapshot_platform_dirs();
    void bootstrap_diagnostics_are_emitted();
};

void AppEntryTests::default_runtime_qml_import_paths_include_staged_bundle_dirs()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path();
    const QString appDir = QDir(rootDir).filePath(QStringLiteral("bin"));
    const QString bundledQmlDir = QDir(appDir).filePath(QStringLiteral("lvrs-runtime/qml/LVRS"));
    const QString installedQmlDir = QDir(appDir).filePath(QStringLiteral("../lib/qt6/qml/LVRS"));

    QVERIFY(QDir().mkpath(bundledQmlDir));
    QVERIFY(QDir().mkpath(installedQmlDir));

    const QStringList importPaths = lvrs::defaultRuntimeQmlImportPaths(appDir);
    QCOMPARE(importPaths,
             QStringList({
                 QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("lvrs-runtime/qml"))),
                 QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../lib/qt6/qml")))
             }));
}

void AppEntryTests::default_runtime_qml_import_paths_include_snapshot_platform_dirs()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString installRoot = QDir(tempDir.path()).filePath(QStringLiteral("prefix"));
    const QString appDir = QDir(installRoot).filePath(QStringLiteral("src/LVRS/example/VisualCatalog/bin"));
    const QString snapshotQmlDir =
        QDir(installRoot).filePath(QStringLiteral("platforms/linux/lib/qt6/qml/LVRS"));

    QVERIFY(QDir().mkpath(appDir));
    QVERIFY(QDir().mkpath(snapshotQmlDir));

    const QStringList importPaths = lvrs::defaultRuntimeQmlImportPaths(appDir);
    QCOMPARE(importPaths,
             QStringList({
                 QDir::cleanPath(
                     QDir(appDir).filePath(QStringLiteral("../../../../../platforms/linux/lib/qt6/qml")))
             }));
}

void AppEntryTests::bootstrap_diagnostics_are_emitted()
{
    const QString previousApplicationName = qGuiApp ? qGuiApp->applicationName() : QString();

    lvrs::AppBootstrapOptions options;
    options.applicationName = QStringLiteral("BootstrapDiagnosticsTest");
    options.bootstrapGraphicsBackend = false;
    options.installBundledFonts = false;
    options.installPretendardFallbacks = false;
    options.enforcePretendardFallback = false;

    MessageCapture capture;

    const lvrs::AppBootstrapState state = lvrs::preApplicationBootstrap(options);
    QVERIFY(state.ok);
    lvrs::postApplicationBootstrap(*qGuiApp, options);

    const QString logs = capture.joinedMessages();
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.pre.options")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.pre.render-quality")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.pre.graphics-backend")));
    QVERIFY(logs.contains(QStringLiteral("\"bootstrapGraphicsBackend\":false")));
    QVERIFY(logs.contains(QStringLiteral("\"QSG_RHI_PIPELINE_CACHE_LOAD\"")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.post.application")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.post.font-policy")));
    QVERIFY(logs.contains(QStringLiteral("\"pretendardFallbackEnforced\":true")));

    if (qGuiApp)
        qGuiApp->setApplicationName(previousApplicationName);
}

QTEST_MAIN(AppEntryTests)
#include "tst_app_entry.moc"
