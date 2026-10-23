# Native Page Router Gallery

Two independently compiled LVUI pages share one self-rendered iiGUI surface.
LVRS PageRouter commits typed route history only after a new page is constructed.
An old page scope retires on replacement; back navigation constructs a fresh scope.
iiSDKCore owns the executable, worker result delivery, frame loop and shutdown.
The pages and toolbar share an explicit Navigator domain. PageRouter registration
follows the widget scope, even if the application retains the controller.

Configure with the native LVRS, iiGUI, LVUI and iiSDKCore installation prefixes.
`install.sh [destination] [CMake options...]` only configures, builds and installs.
It never launches the application or invokes tests. `--self-test` is registered
for the final runtime verification batch; compilation is not execution evidence.

Home now includes text and rich-child native Links with Core Observable inputs.
The Target 43 toolbar action changes the live href before navigation.
Navigator synchronizes its logical history to ViewStateTracker. A separate
PageMonitor records committed visits; the toolbar's Undo visit changes that log
without navigating. Both model summaries are drawn by the same native LVUI view.
Inactive-page caches, interactive transitions and full inherited Link contracts remain pending.
See [the usage contract](docs/Usage.md).
