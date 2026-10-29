#pragma once
#include <LVRS/ToolbarButton.h>
#include <iiSDKCore/Core/Value/Value.h>
namespace lvrs::toolbar_detail {
[[nodiscard]] bool id_valid(const ToolbarId&);
[[nodiscard]] ii::core::Result<ii::gui::ButtonOptions> prepare(const ToolbarButtonOptions&,bool active=false);
[[nodiscard]] ii::core::Result<std::size_t> value_cost(const ii::core::Value&);
}
