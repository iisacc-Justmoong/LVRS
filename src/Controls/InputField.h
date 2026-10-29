#pragma once
#include <iiGUI/NativeWidgets.h>

namespace lvrs {
enum class InputFieldShape { inline_rectangle, rounded, cylinder };
enum class InputFieldMode { text, search, password };
namespace detail {
[[nodiscard]] ii::gui::InputStyle input_field_style(InputFieldShape,InputFieldMode,bool enabled);
}

struct InputFieldOptions final {
    ii::gui::InputOptions input;
    InputFieldShape shape=InputFieldShape::rounded;
    InputFieldMode mode=InputFieldMode::text;
    bool clear_button_visible=true;
};
struct InputFieldEvents final {
    std::function<void(std::string)> edited;
    ii::runtime::Task submitted;
};

// The editor remains an iiGUI text input, so platform text services and
// selection are not replaced by a parallel text implementation.
class InputField final {
public:
    InputField()=default;
    [[nodiscard]] static ii::core::Result<InputField> create(ii::gui::WidgetBuilder&,ii::gui::Widget,
        std::string_view,InputFieldOptions={},InputFieldEvents={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget editor()const noexcept{return editor_;}
    [[nodiscard]] ii::core::Result<void> set_options(InputFieldOptions)const;
    [[nodiscard]] ii::core::Result<void> set_text(std::string_view)const;
private:
    struct Impl;
    ii::gui::Widget root_,editor_;
    std::weak_ptr<Impl> state_;
};
}
