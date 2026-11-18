#pragma once

// QtGui owns this touch-event injection API; it does not link QtTest.
#include <QtGui/qtestsupport_gui.h>

namespace lvrs_ctest {
inline QPointingDevice *createTouchDevice()
{
    return QTest::createTouchDevice();
}

class TouchSequence final : public QTest::QTouchEventSequence {
public:
    explicit TouchSequence(QWindow *window, QPointingDevice *device, bool autoCommit = true)
        : QTest::QTouchEventSequence(window, device, autoCommit) {}

    [[nodiscard]] bool hasPoint(int id) const { return points.contains(id); }
    [[nodiscard]] QEventPoint pendingPoint(int id) const { return points.value(id); }
};
} // namespace lvrs_ctest
