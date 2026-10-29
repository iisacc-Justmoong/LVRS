#pragma once
#include <iiSDKCore/Core/Value/Value.h>
#include <string>
#include <string_view>
#include <vector>

#if defined(linux)
#undef linux
#endif

namespace lvrs {
// Immutable process/build facts. Alternate target contexts are values and never
// change the machine, renderer or application. Core owns platform detection.
struct PlatformEnvironment final {
    std::string canonical_os="unknown",architecture="unknown";
    bool metal=false,vulkan=false;
    [[nodiscard]] static PlatformEnvironment current();
};
class PlatformInfo final {
public:
    PlatformInfo();
    explicit PlatformInfo(PlatformEnvironment);
    [[nodiscard]] std::string os()const;
    [[nodiscard]] const std::string& canonicalOs()const noexcept{return canonical_;}
    [[nodiscard]] const std::string& arch()const noexcept{return environment_.architecture;}
    [[nodiscard]] std::string graphicsBackend()const;
    [[nodiscard]] bool mobile()const;
    [[nodiscard]] bool desktop()const;
    [[nodiscard]] bool android()const noexcept{return canonical_=="android";}
    [[nodiscard]] bool ios()const noexcept{return canonical_=="ios";}
    [[nodiscard]] bool macos()const noexcept{return canonical_=="macos";}
    [[nodiscard]] bool windows()const noexcept{return canonical_=="windows";}
    [[nodiscard]] bool linux()const noexcept{return canonical_=="linux";}
    [[nodiscard]] bool wasm()const noexcept{return canonical_=="wasm";}
    [[nodiscard]] bool metalSupported()const noexcept{return environment_.metal;}
    [[nodiscard]] bool vulkanSupported()const noexcept{return environment_.vulkan;}
    [[nodiscard]] std::vector<std::string> runtimeTargets()const;
    [[nodiscard]] std::vector<std::string> desktopTargets()const;
    [[nodiscard]] std::vector<std::string> mobileTargets()const;
    [[nodiscard]] std::vector<ii::core::Value> runtimeProfiles()const;
    [[nodiscard]] std::string normalizeTarget(std::string_view)const;
    [[nodiscard]] bool isKnownTarget(std::string_view)const;
    [[nodiscard]] bool targetMatchesCurrent(std::string_view)const;
    [[nodiscard]] bool targetIsMobile(std::string_view)const;
    [[nodiscard]] bool targetIsDesktop(std::string_view)const;
    [[nodiscard]] bool supportsTargetGeneration(std::string_view)const;
    [[nodiscard]] bool backendFeatureReadyFor(std::string_view)const;
    [[nodiscard]] std::string graphicsBackendFor(std::string_view target={})const;
    [[nodiscard]] ii::core::Value runtimeProfile(std::string_view target={})const;
private:
    const PlatformEnvironment environment_;
    const std::string canonical_;
};
}
