#pragma once
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <timeapi.h>
#endif

// Timing fixtures use 1–2 ms synthetic sleeps; release the request after each test.
class MillisecondTimerResolution {
public:
    MillisecondTimerResolution() {
#ifdef Q_OS_WIN
        m_available = timeBeginPeriod(1) == TIMERR_NOERROR;
#endif
    }
    ~MillisecondTimerResolution() {
#ifdef Q_OS_WIN
        if (m_available) timeEndPeriod(1);
#endif
    }
    bool available() const { return m_available; }
    MillisecondTimerResolution(const MillisecondTimerResolution &) = delete;
    MillisecondTimerResolution &operator=(const MillisecondTimerResolution &) = delete;
private:
    bool m_available = true;
};
