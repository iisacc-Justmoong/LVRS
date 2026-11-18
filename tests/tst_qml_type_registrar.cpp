#include "QtEventWait.h"
#include <QGuiApplication>

#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtPlugin>

#include "backend/runtime/qmltyperegistrar.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class RegistrarCreatableObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit RegistrarCreatableObject(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    int value() const
    {
        return m_value;
    }

    void setValue(int value)
    {
        if (m_value == value)
            return;
        m_value = value;
        emit valueChanged();
    }

signals:
    void valueChanged();

private:
    int m_value = 0;
};

class RegistrarUncreatableObject : public QObject
{
    Q_OBJECT

public:
    explicit RegistrarUncreatableObject(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
};

class QmlTypeRegistrarTests : public QObject
{
public:
    void manifest_registers_types_and_qml_can_create_manifest_type();
    void manifest_reports_required_failures_optional_skips_and_duplicates();
};

void QmlTypeRegistrarTests::manifest_registers_types_and_qml_can_create_manifest_type()
{
    const QString uri = QStringLiteral("LVRS.TestRegistrar.Manifest");

    const lvrs::QmlTypeRegistrationReport report = lvrs::registerQmlTypes({
        lvrs::qmlCreatableType<RegistrarCreatableObject>(uri,
                                                         1,
                                                         0,
                                                         QStringLiteral("RegistrarCreatable"),
                                                         QStringLiteral("RegistrarCreatableObject")),
        lvrs::qmlUncreatableType<RegistrarUncreatableObject>(uri,
                                                             1,
                                                             0,
                                                             QStringLiteral("RegistrarUncreatable"),
                                                             QStringLiteral("Constructed only from C++"),
                                                             QStringLiteral("RegistrarUncreatableObject"))
    });

    CTEST_CHECK_MESSAGE(report.ok, qPrintable(report.errorMessage()));
    CTEST_COMPARE(report.results.size(), 2);
    CTEST_CHECK(report.results.at(0).typeId >= 0);
    CTEST_CHECK(report.results.at(1).typeId >= 0);
    CTEST_COMPARE(report.results.at(0).kind, lvrs::QmlTypeRegistrationKind::Creatable);
    CTEST_COMPARE(report.results.at(1).kind, lvrs::QmlTypeRegistrationKind::Uncreatable);
    CTEST_COMPARE(report.diagnostics().size(), 2);
    CTEST_COMPARE(report.results.at(0).toVariantMap().value(QStringLiteral("kind")).toString(),
             QStringLiteral("creatable"));

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import LVRS.TestRegistrar.Manifest 1.0

RegistrarCreatable {
    value: 42
}
)",
                      QUrl(QStringLiteral("qrc:/tests/qml-type-registrar/Creatable.qml")));

    const QStringList errors = [&component]() {
        QStringList messages;
        for (const QQmlError &error : component.errors())
            messages.append(error.toString());
        return messages;
    }();
    CTEST_CHECK_MESSAGE(!component.isError(), qPrintable(errors.join(QStringLiteral("\n"))));

    QPointer<QObject> object = component.create();
    CTEST_CHECK(!object.isNull());
    CTEST_COMPARE(object->property("value").toInt(), 42);
    delete object;
}

void QmlTypeRegistrarTests::manifest_reports_required_failures_optional_skips_and_duplicates()
{
    const QString duplicateUri = QStringLiteral("LVRS.TestRegistrar.Diagnostics");

    const lvrs::QmlTypeRegistrationReport report = lvrs::registerQmlTypes({
        lvrs::qmlCustomTypeRegistration(duplicateUri,
                                        1,
                                        0,
                                        QStringLiteral("DuplicateThing"),
                                        lvrs::QmlTypeRegistrationKind::Custom,
                                        []() { return 9001; },
                                        QStringLiteral("FirstDuplicateThing")),
        lvrs::qmlCustomTypeRegistration(duplicateUri,
                                        1,
                                        0,
                                        QStringLiteral("DuplicateThing"),
                                        lvrs::QmlTypeRegistrationKind::Custom,
                                        []() { return 9002; },
                                        QStringLiteral("SecondDuplicateThing")),
        lvrs::qmlCustomTypeRegistration(QStringLiteral("LVRS.TestRegistrar.RequiredFailure"),
                                        1,
                                        0,
                                        QStringLiteral("BrokenRequired"),
                                        lvrs::QmlTypeRegistrationKind::Custom,
                                        []() { return -1; },
                                        QStringLiteral("BrokenRequired")),
        lvrs::qmlCustomTypeRegistration(QString(),
                                        1,
                                        0,
                                        QStringLiteral("OptionalMissingUri"),
                                        lvrs::QmlTypeRegistrationKind::Custom,
                                        []() { return -1; },
                                        QStringLiteral("OptionalMissingUri"),
                                        false)
    });

    CTEST_CHECK(!report.ok);
    CTEST_COMPARE(report.results.size(), 4);
    CTEST_CHECK(report.results.at(0).ok);
    CTEST_COMPARE(report.results.at(0).typeId, 9001);
    CTEST_CHECK(!report.results.at(1).ok);
    CTEST_CHECK(report.results.at(1).error.contains(QStringLiteral("declared more than once")));
    CTEST_CHECK(!report.results.at(2).ok);
    CTEST_CHECK(report.results.at(2).error.contains(QStringLiteral("registration failed")));
    CTEST_CHECK(report.results.at(3).ok);
    CTEST_CHECK(report.results.at(3).skipped);
    CTEST_CHECK(report.results.at(3).error.contains(QStringLiteral("empty URI")));
    CTEST_COMPARE(report.errors.size(), 2);
    CTEST_CHECK(report.errorMessage().contains(QStringLiteral("SecondDuplicateThing")));
    CTEST_COMPARE(report.diagnostics().size(), 4);
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    QmlTypeRegistrarTests tests;
    runner.run("manifest_registers_types_and_qml_can_create_manifest_type", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.manifest_registers_types_and_qml_can_create_manifest_type(); });
    runner.run("manifest_reports_required_failures_optional_skips_and_duplicates", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.manifest_reports_required_failures_optional_skips_and_duplicates(); });
    return runner.result();
}
#include "tst_qml_type_registrar.moc"
