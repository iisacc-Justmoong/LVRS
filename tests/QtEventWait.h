#pragma once

#include "CTestSupport.h"
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QEvent>
#include <QEventLoop>
#include <QTimer>

namespace lvrs_ctest {
class ScopedDeferredDeletes final {
public:
    ~ScopedDeferredDeletes()
    {
        if (QCoreApplication::instance())
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
};

template <typename Predicate>
bool waitFor(Predicate&& predicate, int timeoutMs)
{
    QDeadlineTimer deadline(timeoutMs);
    while (!predicate()) {
        if (deadline.hasExpired()) return false;
        QEventLoop loop;
        QTimer::singleShot(10, &loop, &QEventLoop::quit);
        loop.exec();
    }
    return true;
}
} // namespace lvrs_ctest

#define CTEST_WAIT_VERIFY(expression, timeout_ms) \
    do { \
        if (!::lvrs_ctest::waitFor([&] { return bool(expression); }, (timeout_ms))) \
            ::lvrs_ctest::fail(__FILE__, __LINE__, "timed out waiting for " #expression); \
    } while (false)

#define CTEST_WAIT_DEFAULT(expression) CTEST_WAIT_VERIFY(expression, 5000)
#define CTEST_WAIT_COMPARE_TIMEOUT(actual, expected, timeout_ms) \
    CTEST_WAIT_VERIFY(((actual) == (expected)), (timeout_ms))
#define CTEST_WAIT_COMPARE(actual, expected) CTEST_WAIT_COMPARE_TIMEOUT(actual, expected, 5000)
