# Windows validation (2026-10-19)

The MSVC x64 Release build completes with Qt 6.8.3 and CMake 4.4.4 when
`LVRS_ENABLE_IPO=OFF`. The MSVC export scanner requires ordinary COFF objects.
QML-only applications retain the shared DLL through its native resource
initialization symbol. `LVRSTests_windows_qml_link` verifies this without a
backend API reference. Tooltip fixtures use an explicit QByteArray branch type.

The build contains 59 CTest suites. After using Git for Windows for `sh`, Ninja
for nested configuration, and the basic Qt render loop for deterministic
readback, 52 suites pass. These seven suites still fail and require follow-up:

- LVRSTests_import_api
- LVRSTests_list_composites
- LVRSTests_figma_parity
- LVRSTests_navigation_state
- LVRSTests_svg_manager
- LVRSTests_platform_integration
- LVRSTests_visual_regression

The Windows export and QML-only link regressions pass. The failing full suite
prevents this maintenance run from installing the framework. No checks are
disabled. Run all tests with `ctest --test-dir build --output-on-failure`.
