#include <QtTest>

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
    Q_OBJECT

private slots:
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

    QVERIFY2(report.ok, qPrintable(report.errorMessage()));
    QCOMPARE(report.results.size(), 2);
    QVERIFY(report.results.at(0).typeId >= 0);
    QVERIFY(report.results.at(1).typeId >= 0);
    QCOMPARE(report.results.at(0).kind, lvrs::QmlTypeRegistrationKind::Creatable);
    QCOMPARE(report.results.at(1).kind, lvrs::QmlTypeRegistrationKind::Uncreatable);
    QCOMPARE(report.diagnostics().size(), 2);
    QCOMPARE(report.results.at(0).toVariantMap().value(QStringLiteral("kind")).toString(),
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
    QVERIFY2(!component.isError(), qPrintable(errors.join(QStringLiteral("\n"))));

    QPointer<QObject> object = component.create();
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("value").toInt(), 42);
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

    QVERIFY(!report.ok);
    QCOMPARE(report.results.size(), 4);
    QVERIFY(report.results.at(0).ok);
    QCOMPARE(report.results.at(0).typeId, 9001);
    QVERIFY(!report.results.at(1).ok);
    QVERIFY(report.results.at(1).error.contains(QStringLiteral("declared more than once")));
    QVERIFY(!report.results.at(2).ok);
    QVERIFY(report.results.at(2).error.contains(QStringLiteral("registration failed")));
    QVERIFY(report.results.at(3).ok);
    QVERIFY(report.results.at(3).skipped);
    QVERIFY(report.results.at(3).error.contains(QStringLiteral("empty URI")));
    QCOMPARE(report.errors.size(), 2);
    QVERIFY(report.errorMessage().contains(QStringLiteral("SecondDuplicateThing")));
    QCOMPARE(report.diagnostics().size(), 4);
}

QTEST_MAIN(QmlTypeRegistrarTests)
#include "tst_qml_type_registrar.moc"
