import QtQuick
import LVRS 1.0

// Figma 110:857: regular 24px rows and 4px vertical container padding.
ContextMenu {
    compactItems: false
    itemWidth: Theme.scaleMetric(145)
    topPadding: Theme.gap4
    bottomPadding: Theme.gap4
    dividerColor: Theme.menuDivider
}
