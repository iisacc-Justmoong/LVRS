#include "Gallery.generated.h"
#include <iiGUI/Application.h>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>

using namespace ii;
using namespace std::chrono_literals;
namespace {
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
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
using View = lvui_generated::Gallery;
struct State final {
    std::shared_ptr<View> view = std::make_shared<View>();
    std::shared_ptr<gui::Host> host;
    std::array<gui::Widget, 3> alerts, buttons;
    gui::Widget status, worker, frames, motion_label;
    std::shared_ptr<lvrs::Motion> motion;
    int motion_phase=0;
    bool test = false, closed = false, background = false, resized = false, closing = false;
    int active = 1, opened = 0, retired = 0, dismissed = 0, typed = 0, plain = 0;
    std::uint64_t frame = 0;
    std::string order;
    void refresh() {
        if (status.snapshot()) good(status.set_text("Opened: " + std::to_string(opened) +
            " · closed: " + std::to_string(retired) + " · dismissed: " + std::to_string(dismissed) +
            " · method → clicked: " + order));
    }
};
}

core::Result<void> runtime::configure_application(Application& app) {
    auto state = std::make_shared<State>();
    state->test = app.arguments().size() == 2 && app.arguments()[1] == "--self-test";
    if (app.arguments().size() > 1 && !state->test)
        return core::fail(core::ErrorCode::invalid_argument, "Use --self-test or no arguments");
    std::weak_ptr<State> weak = state;
    auto options = View::options();
    options.frame_loop = &app.frame_loop();
    auto sender = app.event_loop().sender();
    state->host = take(gui::attach_application(app, options, {
        .opened = [weak, sender, &app] {
            auto state = weak.lock(); if (!state) return;
            auto mount = state->view->widgets();
            const std::array<const char*, 3> alert_ids{"one", "two", "three"};
            const std::array<const char*, 3> button_ids{"oneOpen", "twoOpen", "threeOpen"};
            for (std::size_t index = 0; index < 3; ++index) {
                state->alerts[index] = take(mount.find(alert_ids[index]));
                state->buttons[index] = take(mount.find(button_ids[index]));
            }
            state->status = take(mount.find("status"));
            state->worker = take(mount.find("worker"));
            state->frames = take(mount.find("frames"));
            state->motion_label = take(mount.find("motion"));
            state->refresh();
            check(bool(app.post([weak, sender] {
                (void)sender.post([weak] {
                    if (auto state = weak.lock(); state && !state->closed) {
                        state->background = true;
                        good(state->worker.set_text("Background result: 42 · iiSDKCore"));
                    }
                });
            })), "Background task rejected");
            (void)take(app.frame_loop().add([state](const runtime::FrameContext& frame) {
                state->frame = frame.frame;
                if (frame.frame % 15 == 0)
                    good(state->frames.set_text("Core display frames: " + std::to_string(frame.frame)));
                if (state->test && state->background && state->resized && !state->closing) {
                    if(state->motion_phase==1){
                        auto entry=take(state->alerts[1].snapshot());
                        if(!entry.transitioning&&entry.reveal_progress==1){
                            auto gpu=take(state->host->graphics_info());check(gpu.shadow_blurs>0&&gpu.backdrop_blurs>0,"Installed Alert effects absent");
                            good(state->alerts[1].close_popup());check(take(state->alerts[1].snapshot()).exiting,"Installed Alert did not retain exit pixels");
                            good(state->motion->set_reduced_motion(true));check(!take(state->alerts[1].snapshot()).visible,"Installed AOT model did not finish live exit");
                            good(state->buttons[0].activate());check(take(state->alerts[0].snapshot()).reveal_progress==1&&!take(state->alerts[0].snapshot()).transitioning,"AOT shared reduced-motion model was not used");
                            good(state->motion->set_reduced_motion(false));good(state->alerts[0].close_popup());state->motion_phase=2;
                        }
                    }else if(state->motion_phase==2&&!take(state->alerts[0].snapshot()).visible){
                        state->motion_phase=3;state->closing=true;good(take(state->view->widgets().find("close")).activate());
                    }
                }
            }, {.phase = runtime::FramePhase::end, .clock = runtime::TickClock::unscaled}));
            if (!state->test) return;
            check(AlertBackend::created == 1 && state->opened == 1 && take(state->alerts[1].snapshot()).opened,
                  "Initial Alert did not open after scoped backend activation");
            check(take(state->alerts[1].snapshot()).bounds.height == 379, "Two-action authored height differs");
            const auto node_count = take(mount.snapshot()).size();
            auto horizontal = take(mount.find("two.horizontal.1"));
            auto stable_id = take(horizontal.snapshot()).id;
            good(horizontal.activate());
            good(take(mount.find("two.horizontal.2")).activate());
            good(mount.dispatch_input({.kind = gui::WidgetInputKind::pointer_down, .x = 2, .y = 2}));
            good(mount.dispatch_input({.kind = gui::WidgetInputKind::pointer_up, .x = 2, .y = 2}));
            good(mount.dispatch_input({.kind = gui::WidgetInputKind::key_down, .key = gui::WidgetKey::escape}));
            check(state->order == "1a2b" && take(state->alerts[1].snapshot()).opened && state->dismissed == 0,
                  "Default Alert auto-closed or changed method/signal order");
            good(state->alerts[1].close_popup());
            check(bool(horizontal.snapshot()) && !take(horizontal.snapshot()).visible, "Closed Alert destroyed static action widgets");
            good(state->buttons[0].activate());
            good(take(mount.find("one.single.1")).activate());
            good(mount.dispatch_input({.kind = gui::WidgetInputKind::key_down, .key = gui::WidgetKey::escape}));
            check(!take(state->alerts[0].snapshot()).opened && state->dismissed == 1, "Explicit Escape policy did not dismiss once");
            good(state->buttons[2].activate());
            check(take(state->alerts[2].snapshot()).bounds.height == 479, "Three-action authored height differs");
            for (int role = 1; role <= 3; ++role)
                good(take(mount.find("three.vertical." + std::to_string(role))).activate());
            check(state->order == "1a2b1a1a2b3c", "Six AOT action positions did not preserve method/click order");
            good(state->alerts[2].close_popup());
            good(state->buttons[1].activate());
            good(state->host->set_content_size({300, 700}));
            check(take(state->alerts[1].snapshot()).bounds.width == 252, "Narrow Alert did not shrink below source minWidth");
            good(state->host->set_content_size({680, 720}));
            auto bounds = take(state->alerts[1].snapshot()).bounds;
            check(bounds.width == 500 && bounds.x == 90 && take(horizontal.snapshot()).id == stable_id &&
                  take(mount.snapshot()).size() == node_count, "Resize or reopening reconstructed Alert widgets");
            check(!take(state->host->capture_canvas()).rgba.empty() && take(state->host->graphics_info()).backdrop_blurs > 0,
                  "AOT Alert did not use real native GPU glass");
            good(state->alerts[1].close_popup());
            good(take(mount.find("compact")).activate());
            check(state->opened == 4 && state->retired == 4 && state->dismissed == 1 && state->typed == 5 &&
                  state->plain == 2 && state->order == "1a2b1a1a2b3c1a" && AlertBackend::created == 1,
                  "AOT typed/parameterless methods or retained parent scope differ");
            good(state->buttons[1].activate());state->motion_phase=1;
            check(bool(sender.after(5s, [] { throw std::runtime_error("Alert gallery completion watchdog"); })), "Watchdog rejected");
        },
        .resized = [weak](gui::Size size) {
            if (auto state = weak.lock()) state->resized = state->resized || size == gui::Size{680, 720};
        },
        .closed = [weak] {
            if (auto state = weak.lock()) {
                state->closed = true;
                check(AlertBackend::destroyed == 0, "Backend retired before Core view detach");
            }
        }
    }));
    auto verify = app.attach({"verification", [](Application&) -> core::Result<void> { return {}; },
        [state]() -> core::Result<void> {
            check(AlertBackend::created == 1 && AlertBackend::destroyed == 1, "Parent backend did not retire exactly once");
            if (state->test) {
                check(state->closed && state->background && state->resized && state->frame >= 3 && state->motion_phase==3 && state->opened==6 && state->retired==6, "Alert gallery milestones incomplete");
                std::cout << "LVRS_NATIVE_ALERT_GALLERY_OK actions=6 standalone=1 opens=6 closed=6 dismissed=1"
                    " typed=5 plain=2 parent=1/1 retained_widgets=true gpu_glass=true gpu_shadow=true native_motion=true live_reduced_motion=true background=42 frames=" << state->frame << '\n';
            }
            return {};
        }});
    if (!verify) return verify;
    return app.attach(gui::view_attachment("alerts", state->host, state->view,
        std::function<void(View::Scope&)>{[weak, &app](View::Scope& scope) {
            auto& backend = scope.backend_0;
            if(auto state=weak.lock())state->motion=backend.motion;
            backend.on_motion_changed=[weak]{if(auto state=weak.lock();state&&state->motion_label.snapshot()){
                auto prefs=take(state->motion->state());good(state->motion_label.set_text("Motion: native display frames · "+std::to_string(int(prefs.speed))+"× · reduced "+(prefs.reduced_motion?"on":"off")));
            }};
            backend.on_open = [weak](int index) {
                if (auto state = weak.lock()) {
                    state->active = index;
                    good(state->buttons[std::size_t(index)].focus());
                    good(state->alerts[std::size_t(index)].open_at(0, 0));
                }
            };
            backend.on_method = [weak](int role, const lvrs::ButtonEvent& event) {
                if (auto state = weak.lock()) {
                    check(event.trigger == "clicked" && bool(event.source.snapshot()), "AOT method lost native source event");
                    ++state->typed; state->order += char('0' + role); state->refresh();
                }
            };
            backend.on_plain_method = [weak] {
                if (auto state = weak.lock()) { ++state->plain; state->order += '2'; state->refresh(); }
            };
            backend.on_clicked = [weak](int role) {
                if (auto state = weak.lock()) {
                    state->order += char('a' + role - 1); state->refresh();
                    if (!state->test && role >= 2) good(state->alerts[std::size_t(state->active)].close_popup());
                }
            };
            backend.on_opened = [weak] { if (auto state = weak.lock()) { ++state->opened; state->refresh(); } };
            backend.on_closed = [weak] { if (auto state = weak.lock()) { ++state->retired; state->refresh(); } };
            backend.on_dismissed = [weak] { if (auto state = weak.lock()) { ++state->dismissed; state->refresh(); } };
            backend.on_close = [&app] { good(app.quit()); };
        }}));
}
