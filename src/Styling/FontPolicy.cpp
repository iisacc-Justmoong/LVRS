#include <LVRS/FontPolicy.h>
#include <LVRS/PresentationDetail.h>
#include <iiGUI/Appearance.h>
#include <array>

namespace lvrs {
namespace pd=presentation_detail;
using Database=ii::gui::FontDatabase;
namespace {
constexpr std::array<std::string_view,8> targets{"Arial","Helvetica","Sans Serif","Noto Sans","SF Pro Text","SF Pro Display","system-ui","sans-serif"};
template<class T>T take(ii::core::Result<T> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}return std::move(*value);}
bool looks(std::string_view family){return Database::fold_name(family).find("pretendard")!=std::string::npos;}
bool equal(std::string_view a,std::string_view b){return Database::fold_name(a)==Database::fold_name(b);}
bool application_looks(const Database& database){const auto current=database.application_font();return current&&looks(current->family);}
std::string style(int weight){return weight>=700?"Bold":weight>=600?"SemiBold":weight>=500?"Medium":"Regular";}
std::string pretendard(const Database& database){
    const auto families=database.families();for(const auto& family:families)if(equal(family,"Pretendard"))return family;
    for(const auto& family:families)if(looks(family))return family;return {};
}
void install(Database& database){(void)take(database.load_bundled_fonts());for(auto target:targets)(void)take(database.install_substitution(std::string(target),"Pretendard"));}
bool enforce(Database& database){
    (void)take(database.load_bundled_fonts());auto current=database.application_font();if(!current)return false;
    std::string family;for(const auto& candidate:database.families())if(looks(candidate)){family=candidate;break;}
    if(family.empty())return false;if(equal(current->family,family))return true;
    current->family=family;current->style=style(current->weight);(void)take(database.set_application_font(*current));return equal(database.resolve(*current),family);
}
}
struct FontPolicy::Impl final:pd::Signals<FontPolicyEvent> {
    std::shared_ptr<Database> database;
    std::string effective="Pretendard",warning;
    bool available=false;
    ii::core::Observable<std::uint64_t> effective_epoch{0},warning_epoch{0};
    void publish(FontPolicyEvent event){
        pd::Flag guard{notifying};std::optional<ii::core::Error> failure;
        tick(event==FontPolicyEvent::effective_family?effective_epoch:warning_epoch,failure);if(!closed)send(event,failure);
        if(failure){if(failure->cause)std::rethrow_exception(failure->cause);throw std::runtime_error(failure->message);}
    }
    void require_write(){if(auto ready=writable();!ready)throw std::logic_error(ready.error().message);}
    void refresh(){
        if(closed)return;(void)take(database->refresh());auto family=pretendard(*database);const bool next_available=!family.empty();
        if(family.empty()){const auto current=database->application_font();family=current?current->family:database->system_family();}
        if(effective==family&&available==next_available)return;
        effective=std::move(family);available=next_available;publish(FontPolicyEvent::effective_family);
    }
    void warning_value(std::string value){if(closed||warning==value)return;warning=std::move(value);publish(FontPolicyEvent::last_warning);}
};
FontPolicy::FontPolicy():FontPolicy(Database::application()){}
FontPolicy::FontPolicy(std::shared_ptr<Database> database):impl_(std::make_shared<Impl>()){
    if(!database)throw std::invalid_argument("Font policy requires an application font database");impl_->database=std::move(database);
    install(*impl_->database);if(!application_looks(*impl_->database))(void)enforce(*impl_->database);impl_->refresh();
}
FontPolicy::~FontPolicy(){auto p=impl_;p->closed=true;p->receivers.clear();}
std::string FontPolicy::preferredFamily()const{impl_->readable();return "Pretendard";}
std::string FontPolicy::systemFamily()const{impl_->readable();return impl_->database->system_family();}
std::string FontPolicy::effectiveFamily()const{auto p=impl_;p->readable();(void)p->effective_epoch.get();return p->effective;}
bool FontPolicy::pretendardAvailable()const{auto p=impl_;p->readable();(void)p->effective_epoch.get();return p->available;}
std::string FontPolicy::lastWarning()const{auto p=impl_;p->readable();(void)p->warning_epoch.get();return p->warning;}
void FontPolicy::refresh(){auto p=impl_;p->require_write();p->refresh();}
bool FontPolicy::enforceApplicationFallback(){
    auto p=impl_;p->require_write();
    if(application_looks(*p->database)){p->refresh();p->warning_value({});return true;}
    const bool applied=enforce(*p->database);p->refresh();
    p->warning_value(!applied&&!p->available?"Pretendard is unavailable. Falling back to application font.":"");return applied;
}
std::string FontPolicy::resolveFamily(std::string_view requested)const{auto p=impl_;p->readable();auto clean=pd::normalized_text(requested);return !clean.empty()&&p->database->family_available(clean)?clean:effectiveFamily();}
int FontPolicy::weightForTextSize(int size,int fallback)const{impl_->readable();for(const auto& token:ii::gui::lvrs_text_tokens())if(token.pixels==size)return token.weight;return fallback;}
std::string FontPolicy::styleNameForTextSize(int size,std::string_view fallback)const{impl_->readable();for(const auto& token:ii::gui::lvrs_text_tokens())if(token.pixels==size)return std::string(token.style);return std::string(fallback);}
bool FontPolicy::isThemeTextStyleCompliant(int size,int weight,std::string_view name)const{impl_->readable();auto clean=pd::normalized_text(name);for(const auto& token:ii::gui::lvrs_text_tokens())if(token.pixels==size&&token.weight==weight&&equal(clean,token.style))return true;return false;}
void FontPolicy::loadBundledFonts(){(void)take(Database::application()->load_bundled_fonts());}
void FontPolicy::installPretendardFallbacks(){install(*Database::application());}
bool FontPolicy::enforcePretendardFallback(){return enforce(*Database::application());}
ii::core::Result<void> FontPolicy::observe(std::shared_ptr<const FontPolicyChangePort> receiver){return impl_->observe(std::move(receiver));}
}
