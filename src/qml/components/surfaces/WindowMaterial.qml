import QtQuick
import QtQuick.Window as NativeWindow
import LVRS 1.0

MaterialSurface {
    // Resolve the app accent from this window, including custom backgrounds.
    // A detached surface falls back to the engine's app theme.
    primaryColor: {
        const owner = NativeWindow.Window.window
        const accent = owner ? owner["primaryColor"] : undefined
        return accent === undefined ? Theme.primaryColor : accent
    }
    density: MaterialSurface.Dense75
    color: Theme.materialWindowFill
    tintOpacity: Theme.materialWindowOpacity
    intenseOpacity: Theme.materialIntenseOpacity
    faintOpacity: Theme.materialFaintOpacity
    radius: Theme.materialWindowRadius
    shadowRadius: Theme.scaleRealMetric(48)
    shadowOffsetY: Theme.scaleRealMetric(16)
    shadowOpacity: 0.30
    implicitWidth: Theme.scaleMetric(500)
    implicitHeight: Theme.scaleMetric(272)
}

// LV.WindowMaterial { anchors.fill: parent } // Inherits its window's primaryColor.
