#include <LVRS/Theme.h>
#include <LVRS/FontPolicy.h>
#include "FigmaColors.h"
#include <iiSDKCore/Core/Property/ObservableReadScope.h>
#include <cmath>
#include <stdexcept>

namespace lvrs {
ii::gui::WidgetColor Theme::accent_color(Accent accent) {
    return accent_role(accent, AccentRole::standard);
}
ii::gui::WidgetColor Theme::accent_role(Accent accent,AccentRole role) {
    constexpr std::array<int,12> rows{0,4,5,1,6,3,2,-1,7,8,9,-1};
    const auto index=static_cast<unsigned>(accent), state=static_cast<unsigned>(role);
    if(index>=rows.size()) throw std::invalid_argument("Unknown LVRS accent");
    if(state>=figma_colors::accents.front().size()) throw std::invalid_argument("Unknown LVRS accent role");
    if(rows[index]>=0) return figma_colors::accents[std::size_t(rows[index])][state];
    // Gray and white remain legacy neutral choices; Figma defines no Accent roles for them.
    ii::gui::WidgetColor color=accent==Accent::gray
        ? ii::gui::WidgetColor{140.0/255,140.0/255,140.0/255,1}
        : ii::gui::WidgetColor{1,1,1,1};
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
    case AccentRole::text: mix(1,.7);break;
    default: throw std::invalid_argument("Unknown LVRS accent role");
    }
    return color;
}
ii::gui::WidgetColor Theme::icon_accent_role(Accent accent,AccentRole source) {
    if(source==AccentRole::standard) return accent_role(accent,AccentRole::hover);
    if(source==AccentRole::hover) return accent_role(accent,AccentRole::pressed);
    auto color=accent_role(accent,source);
    constexpr std::array<int,12> rows{0,4,5,1,6,3,2,-1,7,8,9,-1};
    const auto index=rows[static_cast<unsigned>(accent)];
    for(const auto& token:figma_colors::icon_accents)
        if(index>=0 && token.color==std::size_t(index) && token.role==static_cast<unsigned>(source)) return token.value;
    color.red*=.9;color.green*=.9;color.blue*=.9;
    return color;
}
ii::gui::WidgetColor Theme::background(BackgroundRole role) {
    const auto index=static_cast<unsigned>(role);
    if(index>=figma_colors::backgrounds.size()) throw std::invalid_argument("Unknown LVRS background role");
    return figma_colors::backgrounds[index];
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
    const auto base=background(BackgroundRole::window);
    const auto mix=[](double b,double v) { return std::round((b*.98+v*.02)*255)/255; };
    result.window={mix(base.red,c.red),mix(base.green,c.green),mix(base.blue,c.blue),1};
    result.hover=accent_role(accent_,AccentRole::hover);
    result.pressed=accent_role(accent_,AccentRole::pressed);
    return result;
}
} // namespace lvrs
