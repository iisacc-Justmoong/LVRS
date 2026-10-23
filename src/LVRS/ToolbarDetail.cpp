#include "ToolbarDetail.h"
#include "Icons.h"
#include "SurfaceDetail.h"
#include <cmath>
namespace lvrs::toolbar_detail {
using namespace ii;
bool id_valid(const ToolbarId& id){auto text=std::get_if<std::string>(&id);return !text||(text->size()<=1024&&surface_detail::text(*text));}
core::Result<gui::ButtonOptions> prepare(const ToolbarButtonOptions& o,bool active){
    if(!id_valid(o.button_id)||!surface_detail::text(o.icon_name)||!surface_detail::text(o.accessibility_name)||!surface_detail::text(o.text)||unsigned(o.tone)>4)return core::fail(core::ErrorCode::invalid_argument,"Invalid toolbar button identity or text");
    for(auto metric:{o.width,o.height,o.icon_size,o.left_padding,o.right_padding,o.corner_radius})if(!surface_detail::metric(metric))return core::fail(core::ErrorCode::invalid_argument,"Invalid toolbar button metric");
    if(o.height<1||o.icon_size<1||o.icon_size>256||!std::isfinite(o.content_spacing)||o.content_spacing< -256||o.content_spacing>8192)return core::fail(core::ErrorCode::invalid_argument,"Invalid toolbar button size or spacing");
    if(o.style){for(auto color:{o.style->fill,o.style->hover_fill,o.style->pressed_fill,o.style->disabled_fill,o.style->text,o.style->disabled_text,o.style->border})if(color&&!surface_detail::color(*color))return core::fail(core::ErrorCode::invalid_argument,"Invalid toolbar button color");if(!surface_detail::metric(o.style->border_width)||!surface_detail::metric(o.style->line_height)||(o.style->font_size&&(!std::isfinite(*o.style->font_size)||*o.style->font_size<1||*o.style->font_size>256))||(o.style->text_role&&unsigned(*o.style->text_role)>7))return core::fail(core::ErrorCode::invalid_argument,"Invalid toolbar typography");}
    auto icon=o.icon;auto name=Icons::normalize(o.icon_name);
    if(!icon&&name.empty()&&o.fallback_icon)name=Icons::normalize("projectStructure");
    if(!icon&&!name.empty()){auto decoded=Icons::image(name);if(!decoded)return std::unexpected(decoded.error());icon=*decoded;}
    gui::ButtonOptions result{.text=o.text,.enabled=o.enabled,.tone=o.tone==gui::ButtonTone::disabled?o.tone:active?gui::ButtonTone::normal:o.tone,.icon=std::move(icon),.icon_name=std::move(name),.accessibility_name=o.accessibility_name,.icon_size=o.icon_size,.icon_mode=bool(icon),.height=o.height,.corner_radius=o.corner_radius,.style=o.style,.left_padding=o.left_padding,.right_padding=o.right_padding,.content_spacing=o.content_spacing,.indicator_size=o.icon_size,.width=o.width?std::optional{o.width}:std::nullopt};
    // Moving the image above must not change the decision to draw an icon.
    result.icon_mode=bool(result.icon);
    if(o.menu){auto indicator=Icons::image(result.tone==gui::ButtonTone::borderless?"generalchevronDownBorderless":result.tone==gui::ButtonTone::primary||result.tone==gui::ButtonTone::destructive?"generalchevronDownAccent":"generalchevronDown");if(!indicator)return std::unexpected(indicator.error());result.indicator=*indicator;indicator=Icons::image("generalchevronDownDisabled");if(!indicator)return std::unexpected(indicator.error());result.disabled_indicator=*indicator;}
    return result;
}
core::Result<std::size_t> value_cost(const core::Value& value){
    struct Pending{const core::Value* value;std::size_t depth;};std::vector<Pending> stack{{&value,0}};std::size_t bytes=0,nodes=0;
    const auto charge=[&](std::size_t n){if(n>1024*1024-bytes)return false;bytes+=n;return true;};
    while(!stack.empty()){
        auto [item,depth]=stack.back();stack.pop_back();if(depth>32||++nodes>8192||!charge(32))return core::fail(core::ErrorCode::full,"Toolbar payload exceeds its structure budget");
        if(auto s=std::get_if<std::string>(&item->storage());s&&!charge(s->size()))return core::fail(core::ErrorCode::full,"Toolbar payload exceeds its byte budget");
        if(auto b=std::get_if<core::Bytes>(&item->storage());b&&!charge(b->size()))return core::fail(core::ErrorCode::full,"Toolbar payload exceeds its byte budget");
        if(auto a=std::get_if<core::Value::Array>(&item->storage())){if(a->size()>8192-nodes-stack.size())return core::fail(core::ErrorCode::full,"Toolbar array exceeds its node budget");for(const auto& child:*a)stack.push_back({&child,depth+1});}
        if(auto o=std::get_if<core::Value::Object>(&item->storage())){if(o->size()>8192-nodes-stack.size())return core::fail(core::ErrorCode::full,"Toolbar object exceeds its node budget");for(const auto& [key,child]:*o){if(!charge(key.size()))return core::fail(core::ErrorCode::full,"Toolbar payload exceeds its byte budget");stack.push_back({&child,depth+1});}}
    }return bytes;
}
}
