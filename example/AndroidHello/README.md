# Android Hello Example

Minimal LVRS Android bootstrap example with:

- standard `QmlAppLaunchSpec` initial property seeding
- a single-route `LV.ApplicationWindow` host
- edge-to-edge rendering in the full Android window
- explicit content reservation through `mobileSystemSafeAreaBounds` instead of automatic shell padding

## Build Android bootstrap target

From repository root:

```bash
cmake -S . -B build-proof -DLVRS_BUILD_EXAMPLES=ON
cmake --build build-proof --target bootstrap_LVRSExampleAndroidHello_android
```

If Android SDK/NDK and adb are configured, LVRS will build and deploy to an attached emulator or device.
