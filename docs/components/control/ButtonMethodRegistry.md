# ButtonMethodRegistry

Location: `qml/components/control/buttons/ButtonMethodRegistry.qml`

`ButtonMethodRegistry` is the shared internal method injection host used by the button family.

## Purpose

- Normalize the `method` and `methods` injection API across button-like controls.
- Build a consistent event payload for clicked and manual method dispatch.
- Support JavaScript functions and command objects without coupling each button to a command framework.

## API

- `owner`: component that exposes the API.
- `defaultTrigger`: trigger name used when no explicit trigger is provided.
- `method`: single injected callable.
- `methods`: array of callables or command objects.
- `hasInjectedMethods` (readonly).
- `collectMethods()`
- `createEvent(triggerName)`
- `invokeMethod(candidate, eventData)`
- `invokeMethods(eventData)`

## Callable Contract

Accepted entries are:

- JavaScript functions: `function(eventData) { ... }`
- command objects with `invoke(eventData)`
- command/action objects with `trigger(eventData)`

`method` runs before entries in `methods`.

## Usage

```qml
ButtonMethodRegistry {
    owner: control
    defaultTrigger: "clicked"
    method: function(eventData) {
        commandBus.dispatch(eventData.trigger)
    }
}
```
