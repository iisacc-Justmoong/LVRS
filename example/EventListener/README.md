# EventListener Examples

This folder shows **multiple ways** to use `EventListener` in QML. The project is now buildable and runnable as a standalone example app.

The example window now follows the full-bleed shell contract: rendering stays full-window, and the demo content is explicitly placed inside `layoutSafeAreaBounds`.

> Note: component name is `EventListener` (spelled as in code).

## Run

From repository root:

```bash
./build.sh
./example/EventListener/bin/LVRSExampleEventListener
```

On macOS Finder, you can also click `example/run-eventlistener.command`.

---

## Plain-language explanation (non-developers)

Think of `EventListener` as a tiny “ear” you can place inside any UI element.
When the element is clicked, hovered, or a key is pressed, the ear hears it
and runs a simple action (like showing a message).

This lets designers attach behavior without changing the original component.

---

## 1) Basic click handler on Label

```qml
LV.Label {
    text: "Click me"
    LV.EventListener {
        trigger: "clicked"
        action: () => console.log("Label clicked")
    }
}
```

---

## 2) Hover state tracking

```qml
Rectangle {
    width: 120; height: 40; color: "#333"
    LV.EventListener {
        trigger: "hoverChanged"
        action: (e) => console.log("hover:", e.containsMouse)
    }
}
```

---

## 3) Press / Release

```qml
LV.Label {
    text: "Hold"
    LV.EventListener {
        trigger: "pressed"
        action: () => console.log("pressed")
    }
    LV.EventListener {
        trigger: "released"
        action: () => console.log("released")
    }
}
```

---

## 4) Custom payload

```qml
LV.Label {
    text: "Payload"
    LV.EventListener {
        trigger: "clicked"
        payload: { source: "payload-example", value: 42 }
        action: (data) => console.log(data.source, data.value)
    }
}
```

---

## 5) Right-click only

```qml
LV.Label {
    text: "Right click"
    LV.EventListener {
        trigger: "clicked"
        acceptedButtons: Qt.RightButton
        action: () => console.log("right click")
    }
}
```

---

## 6) Keyboard trigger (focusable parent)

```qml
TextInput {
    id: input
    focus: true

    LV.EventListener {
        trigger: "keyPressed"
        action: (event) => {
            if (event.key === Qt.Key_Return)
                console.log("enter")
        }
    }
}
```

---

## 7) Wrap any component with multiple listeners

```qml
Rectangle {
    width: 180; height: 60; color: "#222"

    LV.EventListener {
        trigger: "entered"
        action: () => console.log("hover enter")
    }

    LV.EventListener {
        trigger: "exited"
        action: () => console.log("hover exit")
    }

    LV.EventListener {
        trigger: "clicked"
        action: () => console.log("clicked")
    }
}
```

---

## 8) Reusable handler function

```qml
function onAction(tag) {
    console.log("action:", tag)
}

LV.Label {
    text: "Reusable"
    LV.EventListener {
        trigger: "clicked"
        action: () => onAction("label")
    }
}
```
