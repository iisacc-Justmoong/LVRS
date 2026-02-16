# InputMethodGuard

Location: `qml/components/control/util/InputMethodGuard.qml`

`InputMethodGuard` protects text composition state when IME locale/visibility/focus conditions change.

## Purpose

- Prevent preedit/composition residue across IME transitions.
- Provide explicit composition commit points for sensitive text controls.

## API

- `target` (text input-like object)
- `guardEnabled` (default `true`)
- `commitOnLocaleChanged` (default `false`)
- `commitOnVisibilityLost` (default `true`)
- `commitOnFocusLost` (default `true`)
- `logCommitEvents` (default `false`)

## Commit Conditions

When `target.inputMethodComposing == true`, guard commits composition on configured triggers:

- locale change
- IME visibility lost
- focus lost

## Usage

```qml
import LVRS 1.0 as LV

LV.InputMethodGuard {
    target: editor
    guardEnabled: enabled && !readOnly
}
```

## How It Works

- Listens to `Qt.inputMethod` and target focus signals.
- Executes `Qt.inputMethod.commit()` only when guard is enabled and target is composing.
- Optional debug output logs commit reason through `Debug.log`.

## Deployment Note

Keep locale-change commit disabled by default unless target user base actively switches IME locale during composition.
Enable it only when real-world corruption reports justify stricter commit timing.

## FAQ

Q. Why is locale-change commit disabled by default?  
A. Locale changes are less frequent than focus/visibility transitions, and eager commits can be intrusive in some IME workflows.

Q. Should guard be attached to read-only fields?  
A. No. Keep `guardEnabled` false for read-only controls.

## Validation Checklist

- verify composing text is committed on focus loss when enabled,
- verify no duplicate commit side effects in normal typing path,
- verify guard is disabled for non-editable controls.
