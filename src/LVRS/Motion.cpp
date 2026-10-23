#include "Motion.h"
namespace lvrs {
namespace {
ii::gui::PopoverTransition surface_transition(std::shared_ptr<Motion> preferences,bool enabled){
    return {.enter_duration=MotionTokens::surface,.exit_duration=MotionTokens::exit,
        .enter_easing=ii::animation::Easing::out_back,.exit_easing=ii::animation::Easing::in_cubic,
        .overshoot=MotionTokens::overshoot,.collapsed_scale=.92,.enabled=enabled,.preferences=std::move(preferences)};
}
}
ii::gui::PopoverTransition alert_transition(std::shared_ptr<Motion> preferences,bool enabled){return surface_transition(std::move(preferences),enabled);}
ii::gui::PopoverTransition modal_transition(std::shared_ptr<Motion> preferences,bool enabled){
    return surface_transition(std::move(preferences),enabled);
}
}
