#pragma once
#include <iiGUI/NativeWidgets.h>
namespace lvrs {
struct ButtonEvent final {ii::gui::Widget source;std::string_view trigger="clicked";};
using ButtonMethod=std::function<void(const ButtonEvent&)>;
}
