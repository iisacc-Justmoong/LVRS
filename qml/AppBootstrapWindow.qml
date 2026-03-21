import QtQuick
import LVRS 1.0

ApplicationWindow {
    id: root

    // Compatibility wrapper: ApplicationWindow now owns the bootstrap contract,
    // so this type only preserves the visible-root convenience default.
    visible: true
}

// API usage (external):
// import LVRS as LV
// LV.AppBootstrapWindow { title: "Workspace" }
