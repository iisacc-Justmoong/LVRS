# Native navigation model gallery

This C++23 example compiles LVUI through AOT and connects native route matching,
resolution and presentation history to iiGUI labels and buttons. The application
entry, background work, frame loop and shutdown belong to iiSDKCore.

Application sources live in `App/`. This is a navigation-model integration
example, not a completed routed-page host. Per-page creation, scope retirement,
page transitions and the full PageRouter/Navigator/Link surface remain pending.

Build with the installed LVRS native CMake package and its native dependencies.
Use a `build/` output directory. `--self-test` is registered for the final
verification batch; it must not be launched during the current implementation
phase, in accordance with the user's execution policy.
