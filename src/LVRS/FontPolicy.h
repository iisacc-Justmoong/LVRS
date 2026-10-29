#pragma once
#include <iiGUI/FontDatabase.h>
#include <iiSDKCore/Runtime/Query.h>
#include <memory>

namespace lvrs {
enum class FontPolicyEvent {effective_family,last_warning};
using FontPolicyChangePort=ii::runtime::QueryPort<FontPolicyEvent>;
class FontPolicy final {
    struct Impl;
public:
    FontPolicy();
    explicit FontPolicy(std::shared_ptr<ii::gui::FontDatabase>);
    ~FontPolicy();
    FontPolicy(const FontPolicy&)=delete;
    FontPolicy& operator=(const FontPolicy&)=delete;
    [[nodiscard]] std::string preferredFamily()const;
    [[nodiscard]] std::string systemFamily()const;
    [[nodiscard]] std::string effectiveFamily()const;
    [[nodiscard]] bool pretendardAvailable()const;
    [[nodiscard]] std::string lastWarning()const;
    [[nodiscard]] bool enforceApplicationFallback();
    void refresh();
    [[nodiscard]] std::string resolveFamily(std::string_view)const;
    [[nodiscard]] int weightForTextSize(int pixel_size,int fallback_weight)const;
    [[nodiscard]] std::string styleNameForTextSize(int pixel_size,std::string_view fallback_style)const;
    [[nodiscard]] bool isThemeTextStyleCompliant(int pixel_size,int weight,std::string_view style)const;
    static void loadBundledFonts();
    static void installPretendardFallbacks();
    [[nodiscard]] static bool enforcePretendardFallback();
    [[nodiscard]] ii::core::Result<void> observe(std::shared_ptr<const FontPolicyChangePort>);
private:
    std::shared_ptr<Impl> impl_;
};
}
