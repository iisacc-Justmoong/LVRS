#include <LVRS/Widgets.h>
#include <iiGUI/iiGUI.h>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ii;
namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class T> T take(core::Result<T> value) {
    if (!value) throw std::runtime_error(value.error().message);
    return std::move(*value);
}
void good(core::Result<void> value) {
    if (!value) {
        if (value.error().cause) std::rethrow_exception(value.error().cause);
        throw std::runtime_error(value.error().message);
    }
}
void geometry() {
    gui::Host host;
    good(host.run({.opened = [&] {
        auto mount = take(host.mount_widgets([&](gui::WidgetBuilder& builder) -> core::Result<void> {
            auto root = take(builder.vstack({}, "root"));
            take(lvrs::AlertButton::create(builder, root, "compact"));
            take(lvrs::AlertButton::create(builder, root, "dialog", {.dialog_style = true}));
            take(lvrs::AlertButton::create(builder, root, "action", {.dialog_style = true, .height = 56}));
            take(lvrs::AlertButton::create(builder, root, "disabled", {.tone = gui::ButtonTone::disabled}));
            check(!lvrs::AlertButton::create(builder, root, "invalid", {.tone = gui::ButtonTone(99)}), "Invalid tone accepted");
            check(!lvrs::AlertButton::create(builder, root, "color", {.text_color = gui::WidgetColor{2, 0, 0, 1}}), "Invalid color accepted");
            check(!lvrs::AlertButton::create(builder, root, "nan", {.height = std::numeric_limits<double>::quiet_NaN()}), "Invalid metric accepted");
            return {};
        }, lvrs::Theme().appearance()));
        auto compact = take(take(mount.find("compact")).snapshot());
        auto dialog = take(take(mount.find("dialog")).snapshot());
        check(compact.size.height == 20 && dialog.size.height == 20 && dialog.size.width == compact.size.width + 16,
              "Authored compact/dialog padding or height differs");
        check(take(take(mount.find("action")).snapshot()).size.height == 56, "Explicit action height was ignored");
        check(!take(take(mount.find("disabled")).snapshot()).enabled, "Disabled tone remained enabled");
        good(host.request_close());
    }}));
}
void input() {
    gui::Host host;
    lvrs::AlertButton button;
    std::string order;
    good(host.run({.opened = [&] {
        auto mount = take(host.mount_widgets([&](gui::WidgetBuilder& builder) -> core::Result<void> {
            auto root = take(builder.vstack({}, "root"));
            button = take(lvrs::AlertButton::create(builder, root, "button", {.text = "Native action"}, {
                .method = [&](const lvrs::ButtonEvent& event) {
                    check(event.trigger == "clicked" && take(event.source.snapshot()).id == "button", "Button event payload differs");
                    order += 'm';
                }, .clicked = [&] { order += 'c'; }, .pressed = [&] { order += 'p'; },
                .released = [&] { order += 'r'; }, .canceled = [&] { order += 'x'; }
            }));
            return {};
        }, lvrs::Theme().appearance()));
        auto bounds = take(button.root().snapshot()).bounds;
        const double x = bounds.x + bounds.width / 2, y = bounds.y + bounds.height / 2;
        good(mount.dispatch_input({.kind = gui::WidgetInputKind::pointer_down, .x = x, .y = y}));
        good(mount.dispatch_input({.kind = gui::WidgetInputKind::pointer_up, .x = x, .y = y}));
        check(order == "prmc", "Pointer method/click event order differs");
        order.clear();
        good(button.root().focus());
        good(mount.dispatch_input({.kind = gui::WidgetInputKind::key_down, .key = gui::WidgetKey::space}));
        good(mount.dispatch_input({.kind = gui::WidgetInputKind::key_up, .key = gui::WidgetKey::space}));
        check(order == "prmc", "Keyboard action differs from pointer action");
        order.clear();
        good(mount.dispatch_input({.kind = gui::WidgetInputKind::pointer_down, .x = x, .y = y}));
        good(button.set_enabled(false));
        good(mount.dispatch_input({.kind = gui::WidgetInputKind::pointer_up, .x = x, .y = y}));
        check(order == "px", "Disabling pressed action did not cancel exactly once");
        good(button.root().activate());
        check(order == "px", "Disabled action invoked its method");
        good(host.request_close());
    }}));
}
void lifetime() {
    gui::Host host;
    lvrs::AlertButton button;
    std::weak_ptr<int> owner;
    std::string order;
    good(host.run({.opened = [&] {
        auto mount = take(host.mount_widgets([&](gui::WidgetBuilder& builder) -> core::Result<void> {
            auto root = take(builder.vstack({}, "root"));
            auto capture = std::make_shared<int>(42); owner = capture;
            button = take(lvrs::AlertButton::create(builder, root, "button", {}, {
                .method = [&, capture](const lvrs::ButtonEvent&) {
                    check(*capture == 42, "Active method capture was released");
                    order += 'a';
                    good(button.set_method([&](const lvrs::ButtonEvent&) { order += 'b'; }));
                    check(!owner.expired(), "Replacement retired an active method");
                }, .clicked = [&] { order += 'c'; }
            }));
            return {};
        }, lvrs::Theme().appearance()));
        good(button.root().activate());
        check(order == "ac" && owner.expired(), "Old method capture survived its invocation");
        good(button.root().activate());
        check(order == "acbc", "Replacement method did not run");
        good(button.set_method([&](const lvrs::ButtonEvent&) { order += 'u'; good(host.unmount_widgets()); }));
        good(button.root().activate());
        check(order == "acbcu" && !button.root().snapshot(), "Owner removal delivered a stale clicked callback");
        check(!button.set_text("retired") && !button.set_enabled(true), "Retired handle remained writable");
        good(host.request_close());
    }}));
}
}
int main(int argc, char** argv) {
    try {
        const std::string name = argc > 1 ? argv[1] : "geometry";
        if (name == "geometry") geometry(); else if (name == "input") input(); else if (name == "lifetime") lifetime();
        else throw std::runtime_error("Unknown AlertButton test");
        std::cout << "LVRS_NATIVE_ALERT_BUTTON_OK " << name << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
