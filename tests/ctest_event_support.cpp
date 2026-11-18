#include "QtEventWait.h"
#include <QCoreApplication>
#include <QPointer>
#include <QTimer>

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    runner.run("immediate_predicate", [] {
        CTEST_CHECK(lvrs_ctest::waitFor([] { return true; }, 0));
    });
    runner.run("queued_callback", [] {
        int value = 0;
        QTimer::singleShot(0, [&] { value = 42; });
        CTEST_WAIT_COMPARE(value, 42);
    });
    runner.run("timeout_is_reported", [] {
        CTEST_CHECK(!lvrs_ctest::waitFor([] { return false; }, 5));
    });
    runner.run("deferred_deletion_is_flushed", [] {
        QPointer<QObject> object = new QObject;
        {
            lvrs_ctest::ScopedDeferredDeletes cleanup;
            object->deleteLater();
        }
        CTEST_CHECK(object.isNull());
    });
    return runner.result();
}
