.pragma library

// Every shipped QML type has an explicit interaction recipe. Nonvisual types
// document the visible consumer rather than pretending to own a hit target.
var recipes = {
    "menu": ["surface", "Open Menu, choose an action, then reopen and dismiss with Escape.", "The 24px row menu enters with the shared surface rebound and retains its closing layer until the fade ends."],
    "context-menu-divider": ["display", "Compare the compact divider with the regular divider and open their menu surfaces.", "The separator follows surface motion and opacity without intercepting pointer or keyboard input."],
    "context-menu-item": ["action", "Press the compact row, then open ContextMenu and choose a modeled entry.", "The compact row shares button feedback and immediate activation while its 18px layout stays fixed."],
    "color-picker-button": ["action", "Press each size, then Tab to inspect the 2px focus border and disabled example.", "The ring and well rebound together while fill states blend; changing color leaves the trigger bounds fixed."],
    "help-button": ["action", "Press the question mark or focus it with Tab and press Space.", "The circular surface and glyph compress together then rebound through AbstractButton."],
    "app-bootstrap-window": ["shell", "Resize the application-shell example and switch its navigation destination.", "The compatibility root inherits ApplicationWindow's drawer and page motion."],
    "application-window": ["shell", "Resize between sidebar and drawer layouts; open navigation and choose a page.", "The drawer enters with a bounded rebound; navigation controls use the shared press response."],
    "app-shell": ["shell", "Use the shell navigation and accent controls.", "This compatibility wrapper delegates window and navigation feedback to ApplicationWindow."],
    "window": ["shell", "Open the sample tool window, move it, and resize its edges.", "Native window movement follows the operating system; content controls retain LVRS feedback."],
    "window-safe-area-observer": ["utility", "Resize the safe-area example and inspect the inset readout.", "The observer reports native geometry synchronously so the surrounding window can lay out correctly."],
    "window-chrome-interaction": ["utility", "Drag empty title-bar space or a window resize edge in the window example.", "The helper forwards native move and resize gestures without animating pointer coordinates."],
    "app-header": ["action", "Press the menu affordance and the action buttons in the header.", "Each header action compresses then rebounds through the AbstractButton foundation."],
    "v-stack": ["layout", "Change the vertical spacing in the layout playground.", "The ColumnLayout spacing settles with the common rebound; child sizes remain layout-owned."],
    "h-stack": ["layout", "Change the horizontal spacing in the layout playground.", "The RowLayout spacing settles with the same timing as vertical stacks."],
    "z-stack": ["layout", "Toggle the overlay layer in the layout playground.", "Layer opacity dissolves while alignment continues to be managed by anchors."],
    "spacer": ["layout", "Change the spacer minimum length in the layout playground.", "The minimum length settles elastically along the stack axis; it has no clickable surface."],
    "abstract-button": ["action", "Hold the button, release, then click again before its rebound finishes.", "Press deformation is shared by every button subclass, with a focus ring for Tab navigation."],
    "push-button": ["action", "Press the label and icon push-button variants, including their disabled examples.", "The label and icon share one compact compression and elastic release."],
    "dropdown-button": ["action", "Press the dropdown trigger and inspect its label, indicator and menu.", "The complete trigger rebounds as one surface; any ContextMenu animates independently."],
    "label-button": ["action", "Press a label button using pointer, touch or Space after Tab focus.", "The label remains centered as the visual transform compresses and returns."],
    "icon-button": ["action", "Hold an icon button and release; repeat rapidly.", "The icon uses the same press rhythm as text buttons while its authored frame stays fixed."],
    "label-menu-button": ["action", "Press the label-and-chevron menu trigger.", "The trigger rebounds without delaying its clicked or injected method callback."],
    "icon-menu-button": ["action", "Press the icon-and-chevron menu trigger.", "Both symbols share the parent button's motion and one activation event."],
    "label-segmented-control": ["selection", "Select Original and Edited, then use Tab and Space.", "Child buttons respond individually; the row also animates explicit spacing changes."],
    "icon-segmented-control": ["selection", "Switch between the two icon segments.", "The chosen icon button receives the same rebound and keyboard focus feedback as label segments."],
    "combo-box": ["action", "Press Primary and Borderless selectors and watch the label and arrow together.", "The compact composite transforms as one unit; the clicked signal remains immediate."],
    "stepper": ["action", "Press the upper and lower halves of the UpDown stepper.", "Each press rebounds; stepped delivers +1 or -1 according to the actual click half."],
    "check-box": ["selection", "Toggle the checkbox by pointer or Space.", "The checked asset or drawn mark grows into place; the fill blends and the button rebounds."],
    "radio-button": ["selection", "Select the radio indicator and compare its disabled example.", "The inner dot grows into place with the common overshoot while the checked fill blends."],
    "toggle-switch": ["selection", "Hold to squash the knob, drag it, then release or reverse it mid-flight.", "The knob stretches during travel and rebounds into the selected endpoint. Slow and immediate variants are provided."],
    "label": ["display", "Change the opacity of the label in the motion playground and compare typography styles.", "Opacity transitions smoothly. Text values, accessibility content and measurement update synchronously."],
    "progress-bar": ["value", "Change the progress value repeatedly, including 0 and 100.", "The displayed fill settles elastically; the numeric progress remains current and the rendered fill stays inside its track."],
    "table": ["table", "Select cells, edit text and drag row or column resize handles.", "Selection tint blends through TableCellItem; active input uses a focus ring. Resize tracking remains direct."],
    "table-header": ["table", "Edit an inputable header cell or change the selected column in the table example.", "Header cells inherit text-entry focus and selection feedback from their delegates."],
    "table-row": ["table", "Edit a row cell and move focus across its inputs.", "Each cell handles its own focus and state animation; the row preserves column geometry."],
    "table-cell-item": ["table", "Select a cell and begin editing a typed value.", "The selection fill dissolves and the editor shows elastic focus feedback. Validation is immediate."],
    "slider": ["value", "Drag the thumb, release at a tick, then use arrow keys or set a value programmatically.", "Pointer tracking is direct. The thumb deforms on hold; discrete position changes settle with rebound."],
    "color-picker": ["value", "Drag the color plane or channel sliders; edit channel values and press an action.", "Slider thumbs, input focus and action buttons share LVRS motion. Color sampling remains immediate."],
    "abstract-input-bar": ["input", "Focus the entry by Tab or click, type, select text and move focus away.", "A spring-scaled focus ring appears around the field while glyphs and the caret stay still."],
    "input-field": ["input", "Type text, clear it and compare enabled, disabled and read-only states.", "The focus outline rebounds; inherited accessories use their own button feedback."],
    "text-editor": ["input", "Type multiple lines, select a range and scroll within the editor.", "Only the focus outline animates; cursor, text selection, IME and scrolling remain native."],
    "code-editor": ["input", "Focus the code example and edit or select its text.", "The editor frame receives the same focus treatment; syntax and text updates are immediate."],
    "event-listener": ["utility", "Use the event-listener playground to press, release, hover and type.", "The listener forwards events; the real control underneath supplies visible motion."],
    "input-method-guard": ["utility", "Focus and blur the guarded input, including an IME composition.", "The guard coordinates input-method state while the LVRS input supplies the focus ring."],
    "wheel-scroll-guard": ["utility", "Scroll over the guarded text or control area.", "Wheel routing is synchronous; no animation delays the event or changes its recipient."],
    "button-method-registry": ["utility", "Activate the button example and observe its callback counter.", "Methods execute at activation time while the owning button independently completes its rebound."],
    "navigator": ["navigation", "Use the route buttons, then go back.", "Navigator forwards to the active PageRouter, which owns the visible page transition."],
    "page-router": ["navigation", "Push a page, replace it, pop it and try the interactive transition example.", "New pages settle from a modest scale change. Interactive gestures track directly and settle on release."],
    "page-transition-controller": ["navigation", "Begin, update and commit or cancel a transition in the router example.", "This controller exposes the router's gesture state and shares its visual transition."],
    "page-router-transition-driver": ["navigation", "Drag a preview partway and cancel, then complete the same transition.", "The internal driver moves participant pages together, preserving the commit/cancel contract."],
    "link": ["action", "Activate a link using pointer or keyboard in the routing example.", "The link rebounds at activation and its router handles the destination transition."],
    "toolbar-button": ["action", "Press a toolbar action, then move keyboard focus onto it.", "Toolbar actions inherit compact button deformation and the shared focus ring."],
    "hierarchy-toolbar": ["action", "Use the hierarchy overview, expand and collapse actions.", "Each ToolbarButton responds locally without resizing or shifting the toolbar."],
    "hierarchy": ["navigation", "Select rows, expand branches, scroll beyond the edge and release.", "Rows rebound, disclosure arrows rotate, and overscrolled content returns with shared timing."],
    "hierarchy-list": ["navigation", "Expand a branch and drag a row to reorder its parent or position.", "Row state and disclosure motion are local; drag geometry and model changes remain direct."],
    "hierarchy-item": ["navigation", "Press a row and toggle its disclosure chevron.", "The row compresses subtly and its chevron rotates into the new expanded direction."],
    "list": ["navigation", "Select a list row and use controls inside the composite variants.", "Rows and embedded buttons, switches and fields inherit their own feedback."],
    "list-item": ["navigation", "Select the row, edit an inline value and use its trailing action.", "The row rebounds without changing authored height. Each embedded control keeps its native interaction."],
    "list-item-composite": ["navigation", "Try the task, media, resource and form row variants.", "This internal renderer composes animated LVRS controls with stable row geometry."],
    "list-item-selector": ["selection", "Open a select or unit selector inside a composite list row.", "The selector inherits its trigger response and ContextMenu presentation."],
    "list-toolbar": ["action", "Activate the toolbar actions in the list example.", "Action buttons animate locally while the toolbar's placement remains fixed."],
    "list-footer": ["action", "Press each enabled footer action and compare the disabled action.", "The three compact actions share button feedback and independent callback payloads."],
    "context-menu": ["surface", "Open the menu near different edges; select a row or dismiss with Escape.", "The menu scales in from its resolved origin with rebound, then fades on dismissal."],
    "menu-item": ["action", "Hover and press menu rows; expand a submenu row.", "The fill blends, the row rebounds and disclosure rotation follows expansion."],
    "menu-divider": ["display", "Open the menu and toggle a divider in the motion playground.", "A divider participates in surface presentation and supports smooth opacity changes; it has no action."],
    "window-material": ["material", "Switch the app accent and resize the window; the near-black fill should remain uniform.", "Tint and opacity blend through MaterialSurface; the host window keeps native move and resize behavior."],
    "panel-material": ["material", "Change the panel's accent and compare its Glass 25 gradients with the uniform 50% Window fill.", "Material tint transitions use the shared short color timing; panel controls animate independently."],
    "material-surface": ["material", "Change tint and density while content remains visible behind the surface.", "The material blends color and tint opacity without animating blur capture geometry."],
    "popover": ["surface", "Open the material popover, interact inside it and close using Escape or outside click.", "The panel scales from 92% with rebound and fades away on dismissal."],
    "app-card": ["material", "Change the card surface color and use its embedded controls.", "The card color blends while each child owns its interaction; this container has no implicit click action."],
    "card": ["action", "Select a file or resource card, then use its menu and primary action.", "Card deformation is capped in pixels, selection fill blends, and nested actions remain independent."],
    "card-image-content": ["display", "Use File and FilePreview in the card gallery, then change the image or selection.", "This private renderer participates in Card's transform and keeps its crop and source geometry stable."],
    "card-information-content": ["display", "Use the Project, Device and Model cards and their progress and actions.", "The private renderer composes animated progress, buttons and the parent Card response."],
    "alert": ["surface", "Open a two- or three-action alert, then dismiss with one of its actions.", "The centered card grows from 92% with rebound while the backdrop fades. The exit remains visible until settled."],
    "alert-button": ["action", "Press the primary, secondary and cancel actions inside the alert.", "All dialog actions share button deformation, including full-width action rows."],
    "modal": ["surface", "Open the modal and use Continue, Cancel or its permitted backdrop dismissal.", "The frame and backdrop enter together; the closing frame remains visible while fading."],
    "sheet": ["surface", "Open mobile or desktop presentation. Drag the mobile grabber a little, release, then dismiss.", "Mobile slides from the bottom and rebounds to its detent; desktop gently scales in. Dragging remains direct."],
    "tooltip": ["surface", "Hover, focus or hold a target; move the origin and interact with a content tooltip.", "The bubble grows from 94% and rebounds while placement and the tail continue tracking the origin."],
    "theme": ["utility", "Change the app accent in the shell or material gallery.", "Theme supplies shared colors and metrics; rendered component surfaces decide which changes animate."],
    "motion": ["utility", "Use Motion speed and Reduce motion at the top, then try the playground.", "This singleton controls timing and accessibility policy across the current QML engine."],
    "spring-behavior": ["utility", "Change a value in the motion playground, then reverse it before settling.", "The reusable Behavior retargets from its current visual value using the shared OutBack curve."],
    "state-color-behavior": ["utility", "Hover the button or change the material color in the playground.", "The reusable color Behavior blends semantic fills over the shared color duration."],
    "interaction-motion": ["utility", "Hold and release the playground button, then repeat using Space.", "The reusable transform combines press and hover progress with displacement capped by target size."],
    "focus-ring": ["input", "Tab through the motion playground and click its input field.", "The noninteractive outline scales and fades into place without intercepting input." ]
}

function guide(key) {
    var recipe = recipes[key]
    if (!recipe) return null
    var family = recipe[0]
    return {
        family: family,
        trigger: recipe[1],
        response: recipe[2],
        observe: family === "utility"
            ? "Observe the named consumer's live preview. This helper does not create an independent pointer target."
            : family === "input" || family === "table"
                ? "Compare focus entry and exit at 0.25× speed. Editing and selection must remain immediate and readable."
                : family === "layout" || family === "display" || family === "material"
                    ? "Change the demonstrated property at 0.5× speed. The final layout and content should remain stable."
                    : "Hold, release, then interrupt the return with another action. Look for one soft overshoot and a clean settle.",
        contract: "Actions and model values update immediately. Motion only changes presentation. Reduce motion removes interpolation; disabled controls retain their input contract."
    }
}
