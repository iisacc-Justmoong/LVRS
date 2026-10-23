#pragma once
#include <LVRS/AlertButton.h>
#include <LVRS/Motion.h>
#include <functional>
#include <stdexcept>
class AlertBackend final {
public:
    inline static int created = 0, destroyed = 0;
    std::function<void(int)> on_open, on_clicked;
    std::function<void(int, const lvrs::ButtonEvent&)> on_method;
    std::function<void()> on_plain_method, on_opened, on_closed, on_dismissed, on_close;
    std::function<void()> on_motion_changed;
    std::shared_ptr<lvrs::Motion> motion=std::make_shared<lvrs::Motion>();
    AlertBackend() { ++created; }
    ~AlertBackend() { ++destroyed; }
    void one() { if (on_open) on_open(0); }
    void two() { if (on_open) on_open(1); }
    void three() { if (on_open) on_open(2); }
    void primary_method(const lvrs::ButtonEvent& event) { if (on_method) on_method(1, event); }
    void secondary_method() { if (on_plain_method) on_plain_method(); }
    void tertiary_method(const lvrs::ButtonEvent& event) { if (on_method) on_method(3, event); }
    void primary() { if (on_clicked) on_clicked(1); }
    void secondary() { if (on_clicked) on_clicked(2); }
    void tertiary() { if (on_clicked) on_clicked(3); }
    void opened() { if (on_opened) on_opened(); }
    void closed() { if (on_closed) on_closed(); }
    void dismissed() { if (on_dismissed) on_dismissed(); }
    void close() { if (on_close) on_close(); }
    void reduce_motion() {
        auto state=motion->state();if(!state)throw std::runtime_error(state.error().message);
        auto result=motion->set_reduced_motion(!state->reduced_motion);if(!result)throw std::runtime_error(result.error().message);
        if(on_motion_changed)on_motion_changed();
    }
    void motion_speed() {
        auto state=motion->state();if(!state)throw std::runtime_error(state.error().message);
        auto result=motion->set_speed(state->speed==1?2:1);if(!result)throw std::runtime_error(result.error().message);
        if(on_motion_changed)on_motion_changed();
    }
};
