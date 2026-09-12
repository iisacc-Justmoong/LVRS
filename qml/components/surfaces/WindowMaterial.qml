import QtQuick
import LVRS 1.0

MaterialSurface {
    density: MaterialSurface.Dense75
    color: Theme.materialWindowFill
    tintOpacity: Theme.applicationWindowOpacity
    intenseOpacity: 0
    faintOpacity: 0
    radius: Theme.materialWindowRadius
    shadowRadius: Theme.scaleRealMetric(48)
    shadowOffsetY: Theme.scaleRealMetric(16)
    shadowOpacity: 0.30
    implicitWidth: Theme.scaleMetric(500)
    implicitHeight: Theme.scaleMetric(272)
}

// LV.WindowMaterial { anchors.fill: parent; primaryColor: LV.Theme.primary }
