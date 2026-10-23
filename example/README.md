# LVRS native examples

The runnable contracts and examples are maintained in [`examples/`](../examples/). Build them in the repository `build/` directory. Use the installed native SDK kit documented in `docs/`; the Congregation integration uses C++23 and contains no Qt runtime.

`NativeNavigationGallery` demonstrates route/history models.
`NativePageRouterGallery` constructs two independent AOT LVUI pages in one
iiGUI surface. New application sources live in each example's `App/` directory.
Installers build and package without launching applications or tests; runtime
validation remains deferred to the final whole-goal batch.
