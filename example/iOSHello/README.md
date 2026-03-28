# iOS Hello Example

Minimal LVRS iOS bootstrap example with:

- standard `QmlAppLaunchSpec` initial property seeding
- a single-route `LV.ApplicationWindow` host
- full-bleed rendering across the entire iOS window, including the status-bar and notch region
- explicit content reservation through `mobileSystemSafeAreaBounds` instead of automatic scaffold padding

## Build iOS bootstrap target

From repository root:

```bash
cmake -S . -B build-proof -DLVRS_BUILD_EXAMPLES=ON
cmake --build build-proof --target bootstrap_LVRSExampleIOSHello_ios
```

If successful, LVRS configures/builds an iOS app bundle and installs it to a booted simulator.
