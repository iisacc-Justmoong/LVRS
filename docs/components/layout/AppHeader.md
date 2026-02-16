# AppHeader

Location: `qml/components/layout/AppHeader.qml`

Top toolbar with title, subtitle, and action slot.

## Usage
```qml
LV.AppHeader { title: "Dashboard"; subtitle: "Overview" }
```

## Practical Examples

### Example 1: Standard title header
```qml
import QtQuick
import LVRS 1.0 as LV

LV.AppHeader {
    title: "Deployments"
    subtitle: "Production Cluster"
}
```

### Example 2: Drawer menu opener
```qml
import QtQuick
import LVRS 1.0 as LV

LV.AppHeader {
    title: "Monitoring"
    menuVisible: true
    onMenuClicked: console.log("Open drawer")
}
```

### Example 3: Header action buttons
```qml
import QtQuick
import LVRS 1.0 as LV

LV.AppHeader {
    title: "Runs"

    LV.IconButton {
        tone: LV.AbstractButton.Borderless
        iconName: "add"
        onClicked: console.log("More actions")
    }

    LV.LabelButton {
        text: "New Run"
        tone: LV.AbstractButton.Primary
    }
}
```
