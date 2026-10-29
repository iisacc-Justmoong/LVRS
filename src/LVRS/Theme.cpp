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
ii::gui::WidgetColor Theme::accent_role(Accent accent,AccentRole role) {
    auto color=accent_color(accent);
    const auto multiply=[&](double value){color.red*=value;color.green*=value;color.blue*=value;};
    const auto mix=[&](double base,double amount){color.red=base*(1-amount)+color.red*amount;
        color.green=base*(1-amount)+color.green*amount;color.blue=base*(1-amount)+color.blue*amount;};
    constexpr double surface=.1411764770746231;
    switch(role) {
    case AccentRole::standard: break;
    case AccentRole::hover: multiply(1./1.12);break;
    case AccentRole::pressed: multiply(1./1.2);break;
    case AccentRole::selected: mix(surface,.25);break;
    case AccentRole::disabled: color.alpha*=.3;break;
    case AccentRole::subtle: mix(surface,.16);break;
    case AccentRole::outline: mix(surface,.36);break;
    case AccentRole::overlay: color.alpha*=64./255.;break;
    case AccentRole::text:
        if(accent==Accent::blue) {color.red=.3529411852359772;color.green=.6666666865348816;color.blue=1;}
        else {const double white=accent==Accent::brown?.45:.3;mix(1,1-white);}
        break;
    default: throw std::invalid_argument("Unknown LVRS accent role");
    }
    return color;
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
    result.hover=accent_role(accent_,AccentRole::hover);
    result.pressed=accent_role(accent_,AccentRole::pressed);
    return result;
}
} // namespace lvrs
