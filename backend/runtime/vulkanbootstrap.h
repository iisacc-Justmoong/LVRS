#pragma once

#include <QString>
#include <QStringList>

namespace lvrs {
struct GraphicsBackendBootstrapResult {
    bool available = false;
    QString backendName;
    QString loaderName;
    QString errorMessage;
    QString requestedBackendName;
    bool fallbackUsed = false;
    QString fallbackReason;
    QStringList probeCandidates;
};

GraphicsBackendBootstrapResult bootstrapPreferredGraphicsBackend(bool logDiagnostics = true);
}
