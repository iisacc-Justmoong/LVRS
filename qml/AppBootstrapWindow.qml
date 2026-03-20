import QtQuick
import LVRS 1.0

ApplicationWindow {
    id: root

    // Standard downstream bootstrap profile:
    // - visible root window
    // - mobile-safe viewport defaults
    // - runtime daemon attach
    // - internal page-stack host with seedable initial route
    visible: true
    navigationEnabled: false
    autoAttachRuntimeEvents: true
    internalRouterRegisterAsGlobalNavigator: true
    mobileOversizedHeightEnabled: false
    useInternalPageStack: true

    property string initialRoutePath: "/"

    pageInitialPath: initialRoutePath
}

// API usage (external):
// import LVRS as LV
// LV.AppBootstrapWindow { pageRoutes: [{ path: "/", component: homePage }] }
