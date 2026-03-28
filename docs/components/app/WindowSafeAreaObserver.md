# WindowSafeAreaObserver

Location: `backend/platform/windowsafeareaobserver.h`

`WindowSafeAreaObserver` exposes the platform window safe-area insets for a bound `QWindow`/`QQuickWindow`.

## Purpose

- Let downstream apps read the real system safe-area insets without forcing layout into a safe-area container.
- Support full-bleed roots where status-bar, notch, and home-indicator regions are intentionally app-controlled.
- Refresh inset values when the bound window is shown, resized, moved, re-screened, or its platform surface changes.

## API

- `window`: target `QWindow`/`QQuickWindow` object.
- `leftInset`
- `topInset`
- `rightInset`
- `bottomInset`
- `resolved`: `true` once the observer has a platform window and the current safe-area margins were queried.
- `refresh()`: force a re-query.

## Usage

```qml
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root
    visible: true

    LV.WindowSafeAreaObserver {
        id: safeArea
        window: root
    }

    Rectangle {
        anchors.fill: parent
        color: "#111827"
    }

    Item {
        x: safeArea.leftInset
        y: safeArea.topInset
        width: parent.width - safeArea.leftInset - safeArea.rightInset
        height: 56
    }
}
```

## How It Works

- Internally the observer queries Qt's platform window safe-area margins instead of inferring them from `ApplicationWindow` padding.
- This matters because LVRS intentionally keeps root content full-bleed on mobile; the app decides when to honor system insets.
- Before the platform window exists, `resolved` stays `false` and the inset values stay `0`.
- On platforms that do not report safe-area margins, the observer still resolves successfully after attach, but all inset values remain `0`.
