# Dynamic linking

The native LVRS library defaults to a shared target, like the legacy Qt build.
Set `LVRS_BUILD_SHARED_LIBS=OFF` for an explicit static build. Shared iOS and
WebAssembly configurations select a framework and side module respectively.
Applications must package the matching iiGUI and other selected runtime libraries.
Native render-monitor tests clear inherited dynamic-loader overrides so they
exercise the library selected by the build rather than an older installation.
The iOS and WebAssembly bootstraps forward the requested shared-library mode;
support in each Qt toolchain must be validated separately.
The Emscripten package configuration permits shared side-module imports;
applications supply the main-module flags when compiling and linking.
The legacy text model retains its active provider file device across chunked
loads and releases it when loading finishes or is cancelled.
