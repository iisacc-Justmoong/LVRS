#pragma once
#include <iiGUI/NativeWidgets.h>
#include <chrono>
namespace lvrs {
// An explicitly shared native model. A View without one uses its host's model.
using Motion=ii::gui::MotionPreferences;
struct MotionTokens final {
    static constexpr auto press=std::chrono::milliseconds(90),hover=std::chrono::milliseconds(160),
        release=std::chrono::milliseconds(360),button_release=std::chrono::milliseconds(180),
        surface=std::chrono::milliseconds(420),exit=std::chrono::milliseconds(150),color=std::chrono::milliseconds(130);
    static constexpr double overshoot=1.45;
};
[[nodiscard]] ii::gui::PopoverTransition alert_transition(std::shared_ptr<Motion> preferences={},bool enabled=true);
[[nodiscard]] ii::gui::PopoverTransition modal_transition(std::shared_ptr<Motion> preferences={},bool enabled=true);
}
