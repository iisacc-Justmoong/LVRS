import QtQuick
import LVRS 1.0

// Figma 331:9283: a 1px line inset 4px inside a 145 x 3 slot.
MenuDivider {
    lineLength: Theme.scaleMetric(145)
    linePadding: Theme.gap4
    dividerColor: Theme.contextMenuDivider
}
