# MVVM Example (Model → ViewModel → QML)

This example is buildable and runnable, and demonstrates the MVVM flow used in this repo:

- **Model (C++)** holds state and emits signals.
- **ViewModel (C++)** exposes model state to QML and provides commands.
- **View (QML)** binds to ViewModel and triggers commands.
- **ViewModelRegistry (C++)** acts as the bridge that hands ViewModels to QML by key.

> This folder is both a reference implementation and a runnable sample target.

## Run

From repository root:

```bash
cmake -S . -B build -DLVRS_BUILD_EXAMPLES=ON
cmake --build build --target LVRSExampleMVVM
./build/example/mvvm/bin/LVRSExampleMVVM
```

On macOS Finder, you can also click `example/run-mvvm.command`.

---

## Plain-language explanation (non-developers)

Think of the app like a restaurant:
- **Model** is the kitchen (where the real data lives).
- **ViewModel** is the waiter (it knows how to ask the kitchen and deliver answers).
- **View (QML)** is the menu/table (what the user actually sees and clicks).
- **ViewModels registry** is the host stand (it helps the table find the right waiter).

This example shows how data flows from the kitchen → waiter → table without the table
needing to walk into the kitchen.

---

## Files

### `backend/ExampleModel.h` / `backend/ExampleModel.cpp`
A minimal QObject model with a single property:
- `status` (QString)

It emits `statusChanged()` when the value updates.

### `backend/ExampleViewModel.h` / `backend/ExampleViewModel.cpp`
Wraps the model and exposes:
- `status` (READ-only from the model)
- `simulateWork()` (optional command example; view-level writes can also be used)

It forwards `statusChanged()` to QML.

### `backend/ExampleBootstrap.h` / `backend/ExampleBootstrap.cpp`
Registers the ViewModel into `ViewModels`:

- Retrieves the QML singleton instance of `ViewModels` from the engine
- Constructs `ExampleModel` and `ExampleViewModel`
- Stores the ViewModel by key: `"Example"`

### `qml/Main.qml`
Binds UI to the registered ViewModel:

- Binds a view key with ownership: `LV.ViewModels.bindView("ExampleView", "Example", true)`
- Fetches by view key: `LV.ViewModels.getForView("ExampleView")`
- Reads `vm.status`
- Updates model through permission check: `LV.ViewModels.updateProperty("ExampleView", "status", "Working")`

---

## Why a Registry?

In MVVM, QML should not construct C++ models directly. Instead, the app bootstraps the ViewModels once
and the UI simply **requests them by key**.

`ViewModels` is the bridge that keeps the View layer clean and declarative:

- **C++** controls lifetime and wiring
- **QML** only consumes

---

## How to Wire It in a Real App (Pseudo-Flow)

1) Create the QML engine
2) Register ViewModels
3) Load QML

```cpp
QQmlApplicationEngine engine;
setupExampleViewModel(&engine);
engine.load(QUrl("qrc:/qt/qml/Example/Main.qml"));
```

---

## QML Usage Pattern

```qml
import LVRS 1.0 as LV

LV.ApplicationWindow {
    property string viewId: "ExampleView"
    Component.onCompleted: LV.ViewModels.bindView(viewId, "Example", true)
    property var vm: LV.ViewModels.getForView(viewId)

    Column {
        LV.Label { text: vm ? ("Status: " + vm.status) : "No VM" }
        LV.LabelButton {
            text: "Toggle"
            tone: LV.AbstractButton.Primary
            onClicked: LV.ViewModels.updateProperty(viewId, "status",
                                                     vm.status === "Idle" ? "Working" : "Idle")
        }
    }
}
```

---

## Notes / Constraints

- This is available as a build target.
- The file paths use plain QObject classes for simplicity.
- In real code, ViewModels can expose richer state and commands, but the registration pattern is the same.

---

## Quick Checklist for New ViewModels

- [ ] Create a model with Q_PROPERTY + signals
- [ ] Wrap it in a ViewModel
- [ ] Register it via `ViewModels.set("Key", vm)`
- [ ] Bind view ownership via `ViewModels.bindView("ViewId", "Key", true|false)`
- [ ] Fetch in QML via `ViewModels.getForView("ViewId")`
