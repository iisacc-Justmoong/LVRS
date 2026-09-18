#pragma once

#include "backend/runtime/vulkanbootstrap.h"

#include <QString>

class QGuiApplication;

namespace lvrs {

struct AppBootstrapOptions {
    QString applicationName;
    QString quickStyleName;
    bool configureRenderQualityDefaults = true;
    bool bootstrapGraphicsBackend = true;
    bool logBootstrapDiagnostics = true;
    bool logGraphicsBackend = true;
    bool installBundledFonts = true;
    bool installPretendardFallbacks = true;
    bool enforcePretendardFallback = true;
};

struct AppBootstrapState {
    bool ok = true;
    QString errorMessage;
    GraphicsBackendBootstrapResult graphicsBackend;
};

AppBootstrapState preApplicationBootstrap(const AppBootstrapOptions &options = {});
void postApplicationBootstrap(QGuiApplication &app, const AppBootstrapOptions &options = {});

} // namespace lvrs
