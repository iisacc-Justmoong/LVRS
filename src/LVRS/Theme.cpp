#include "Theme.h"
#include "FontPolicy.h"
#include <iiSDKCore/Core/Property/ObservableReadScope.h>
#include <cmath>
#include <stdexcept>

namespace lvrs {
ii::gui::WidgetColor Theme::accent_color(Accent accent) {
    using C=ii::gui::WidgetColor;
    constexpr std::array<C,12> palette{{
        {.08120492845773697,.5099760293960571,1,1},
        {.5275065302848816,.291259765625,1,1},
        {.975510835647583,.26653972268104553,.4437825083732605,1},
        {1,.2760854959487915,.2760854959487915,1},
        {1,.47672274708747864,.15144230425357819,1},
        {1,.8303598165512085,.15179912745952606,1},
        {0,.9058823585510254,.12156862765550613,1},
        {140.0/255,140.0/255,140.0/255,1},
        {.2616060674190521,.13824503123760223,.09279626607894897,1},
        {.12926307320594788,.8258525729179382,1,1},
        {0,.7790151238441467,.8655723929405212,1},
        {1,1,1,1}}};
    const auto index=static_cast<unsigned>(accent);
    if(index>=palette.size()) throw std::invalid_argument("Unknown LVRS accent");
    return palette[index];
}
ii::gui::WidgetAppearance Theme::appearance() const {
    static thread_local const bool fonts_ready=ii::core::ObservableReadScope::pause([]{FontPolicy policy;return true;});
    (void)fonts_ready;
    auto result=ii::gui::lvrs_appearance(); result.primary=accent_color(accent_);
    const auto fonts=ii::gui::FontDatabase::application();
    if(fonts->family_available("Pretendard"))result.font_family="Pretendard";
    else {
        bool found=false;for(const auto& family:fonts->families())if(ii::gui::FontDatabase::fold_name(family).find("pretendard")!=std::string::npos){result.font_family=family;found=true;break;}
        if(!found){const auto application=fonts->application_font();result.font_family=application?application->family:fonts->system_family();}
    }
    const auto c=result.primary;
    const auto mix=[](double v) { return std::round((20.0*.98+v*255*.02))/255; };
    result.window={mix(c.red),mix(c.green),mix(c.blue),1};
    result.hover={c.red/1.12,c.green/1.12,c.blue/1.12,1};
    result.pressed={c.red/1.2,c.green/1.2,c.blue/1.2,1};
    return result;
}
} // namespace lvrs
