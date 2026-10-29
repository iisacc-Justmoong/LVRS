#include "PlatformInfo.h"
#include <iiSDKCore/Core/Platform/PlatformProfile.h>
#include <algorithm>
#include <array>

namespace lvrs {
namespace {
using Value=ii::core::Value;
constexpr std::array<std::string_view,6> targets{"macos","linux","windows","ios","android","wasm"};
constexpr std::array<std::string_view,4> desktops{"macos","linux","windows","wasm"};
constexpr std::array<std::string_view,2> mobiles{"ios","android"};
struct Alias final {std::string_view name,target;};
constexpr Alias aliases[]{
    {"osx","macos"},{"mac","macos"},{"darwin","macos"},{"macosx","macos"},
    {"win","windows"},{"win32","windows"},{"win64","windows"},{"mingw","windows"},
    {"gnu/linux","linux"},{"ubuntu","linux"},{"debian","linux"},{"fedora","linux"},
    {"iphoneos","ios"},{"iphonesimulator","ios"},{"ios-simulator","ios"},
    {"android-arm","android"},{"android-arm64","android"},{"android-x86","android"},{"android-x86_64","android"},
    {"emscripten","wasm"},{"webassembly","wasm"},{"qtwasm","wasm"}
};
constexpr std::array<std::string_view,18> spaces{
    "\xc2\x85","\xc2\xa0","\xe1\x9a\x80","\xe2\x80\x80","\xe2\x80\x81","\xe2\x80\x82",
    "\xe2\x80\x83","\xe2\x80\x84","\xe2\x80\x85","\xe2\x80\x86","\xe2\x80\x87","\xe2\x80\x88",
    "\xe2\x80\x89","\xe2\x80\x8a","\xe2\x80\xa8","\xe2\x80\xa9","\xe2\x80\xaf","\xe2\x81\x9f"
};
bool ascii_space(unsigned char c){return c==' '||(c>=9&&c<=13);}
std::string_view trimmed(std::string_view value){
    if(!ii::core::valid_utf8(value))throw std::invalid_argument("Platform target requires UTF-8");
    const auto remove=[&](bool front){
        if(value.empty())return false;
        if(ascii_space(static_cast<unsigned char>(front?value.front():value.back()))){if(front)value.remove_prefix(1);else value.remove_suffix(1);return true;}
        for(auto space:spaces)if(front?value.starts_with(space):value.ends_with(space)){if(front)value.remove_prefix(space.size());else value.remove_suffix(space.size());return true;}
        constexpr std::string_view ideographic="\xe3\x80\x80";
        if(front?value.starts_with(ideographic):value.ends_with(ideographic)){if(front)value.remove_prefix(ideographic.size());else value.remove_suffix(ideographic.size());return true;}
        return false;
    };
    while(remove(true)){}while(remove(false)){}return value;
}
template<std::size_t N>bool contains(const std::array<std::string_view,N>& values,std::string_view value){return std::ranges::find(values,value)!=values.end();}
std::string normalized(std::string_view value){
    value=trimmed(value);if(value.size()>16)return {};
    std::string text(value);for(auto& c:text)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
    if(contains(targets,text))return text;
    for(const auto& alias:aliases)if(alias.name==text)return std::string(alias.target);
    return {};
}
std::string backend(std::string_view target){
    if(target=="macos"||target=="ios")return "metal";
    if(target=="windows")return "d3d11";
    if(target=="android")return "vulkan";
    return "default";
}
struct Adaptive final {
    int wide=980,nav=220,drawer=240,mobile_min=1200,bottom=5,compact=900;
    double rail=.32;
    int safety=16,enter=160,exit=120;
};
Adaptive adaptive(std::string_view target){
    if(target=="ios")return {948,216,264,1180,5,860,.33,24,185,145};
    if(target=="android")return {940,208,252,1080,4,840,.34,20,170,130};
    if(target=="macos")return {1040,232,256,1320,5,960,.30,16,180,136};
    if(target=="windows")return {1000,224,248,1240,5,920,.31,16,155,115};
    if(target=="wasm")return {960,216,236,1120,4,880,.34,16,140,100};
    return {};
}
std::string cmake_name(std::string_view target){
    constexpr Alias names[]{{"macos","Darwin"},{"linux","Linux"},{"windows","Windows"},{"ios","iOS"},{"android","Android"},{"wasm","Emscripten"}};
    for(const auto& name:names)if(name.name==target)return std::string(name.target);return "Unknown";
}
}
PlatformEnvironment PlatformEnvironment::current(){
    const auto profile=ii::core::PlatformProfile::current();PlatformEnvironment result;
    using Platform=ii::runtime::Platform;
    switch(profile.os){case Platform::macos:result.canonical_os="macos";break;case Platform::linux_os:result.canonical_os="linux";break;
    case Platform::windows:result.canonical_os="windows";break;case Platform::ios:result.canonical_os="ios";break;
    case Platform::android:result.canonical_os="android";break;case Platform::wasm:result.canonical_os="wasm";break;default:break;}
    using Architecture=ii::core::Architecture;
    switch(profile.architecture){case Architecture::arm64:result.architecture="arm64";break;case Architecture::arm32:result.architecture="arm";break;
    case Architecture::x64:result.architecture="x86_64";break;case Architecture::x86:result.architecture="i386";break;
    case Architecture::wasm32:result.architecture="wasm32";break;case Architecture::wasm64:result.architecture="wasm64";break;default:break;}
#if defined(LVRS_NATIVE_HAS_METAL) && LVRS_NATIVE_HAS_METAL
    result.metal=true;
#endif
#if defined(LVRS_NATIVE_HAS_VULKAN) && LVRS_NATIVE_HAS_VULKAN
    result.vulkan=true;
#endif
    return result;
}
PlatformInfo::PlatformInfo():PlatformInfo(PlatformEnvironment::current()){}
PlatformInfo::PlatformInfo(PlatformEnvironment environment):environment_(std::move(environment)),canonical_([&]{auto name=normalized(environment_.canonical_os);return name.empty()?std::string("unknown"):name;}()){
    if(environment_.architecture.empty()||!ii::core::valid_utf8(environment_.architecture))throw std::invalid_argument("Platform architecture requires nonempty UTF-8");
}
std::string PlatformInfo::os()const{return macos()?"osx":canonical_;}
std::string PlatformInfo::graphicsBackend()const{return backend(canonical_);}
bool PlatformInfo::mobile()const{return contains(mobiles,canonical_);}
bool PlatformInfo::desktop()const{return contains(desktops,canonical_);}
std::vector<std::string> PlatformInfo::runtimeTargets()const{return {targets.begin(),targets.end()};}
std::vector<std::string> PlatformInfo::desktopTargets()const{return {desktops.begin(),desktops.end()};}
std::vector<std::string> PlatformInfo::mobileTargets()const{return {mobiles.begin(),mobiles.end()};}
std::vector<Value> PlatformInfo::runtimeProfiles()const{std::vector<Value> result;result.reserve(targets.size());for(auto target:targets)result.push_back(runtimeProfile(target));return result;}
std::string PlatformInfo::normalizeTarget(std::string_view target)const{return normalized(target);}
bool PlatformInfo::isKnownTarget(std::string_view target)const{return !normalized(target).empty();}
bool PlatformInfo::targetMatchesCurrent(std::string_view target)const{const auto value=normalized(target);return !value.empty()&&value==canonical_;}
bool PlatformInfo::targetIsMobile(std::string_view target)const{return contains(mobiles,normalized(target));}
bool PlatformInfo::targetIsDesktop(std::string_view target)const{return contains(desktops,normalized(target));}
bool PlatformInfo::supportsTargetGeneration(std::string_view target)const{return isKnownTarget(target);}
bool PlatformInfo::backendFeatureReadyFor(std::string_view target)const{
    const auto value=normalized(target);if(value.empty())return false;const auto name=backend(value);
    return name=="metal"?environment_.metal:name=="vulkan"?environment_.vulkan:true;
}
std::string PlatformInfo::graphicsBackendFor(std::string_view target)const{const auto value=trimmed(target);return backend(value.empty()?canonical_:normalized(value));}
Value PlatformInfo::runtimeProfile(std::string_view requested)const{
    requested=trimmed(requested);const auto target=normalized(requested.empty()?std::string_view(canonical_):requested);
    const bool known=!target.empty(),is_mobile=contains(mobiles,target),is_desktop=contains(desktops,target),is_android=target=="android",is_ios=target=="ios",is_wasm=target=="wasm";
    const auto graphics=backend(target);const bool ready=known&&backendFeatureReadyFor(target);const auto layout=adaptive(target);
    Value::Object result;
    const auto put=[&]<class T>(const char* name,T value){result.emplace(name,Value(std::move(value)));};
    put("requested",std::string(requested));put("target",known?target:std::string("unknown"));put("known",known);put("host",canonical_);put("current",known&&target==canonical_);
    put("desktop",is_desktop);put("mobile",is_mobile);put("backend",graphics);put("generationSupported",known);put("backendFeatureReady",ready);
    put("metalRequired",known&&graphics=="metal");put("vulkanRequired",known&&graphics=="vulkan");put("runtimeEventsAutoAttachRecommended",false);
    put("mobileSystemWindowDelegationRecommended",is_android);put("mobileSystemInsetsDelegationRecommended",is_android);
    put("mobileDisplayCoverageOverrideRecommended",is_mobile);put("mobileFullscreenVisibilityRecommended",is_mobile);put("mobileFullscreenGeometryHintRecommended",is_mobile);
    put("bootstrapMsaaSamples",is_wasm?2:4);put("bootstrapFramesInFlight",target=="macos"?3:is_wasm?1:2);
    put("bootstrapPartialUpdateRecommended",!is_wasm);put("bootstrapBatchRenderingRecommended",!is_wasm);put("bootstrapPipelineCacheRecommended",!is_wasm);
    put("bootstrapTextureAtlasEdge",is_mobile||is_wasm?1024:2048);put("adaptiveWideBreakpoint",layout.wide);put("adaptiveNavWidth",layout.nav);put("adaptiveNavDrawerWidth",layout.drawer);
    put("adaptiveMobileDesktopMinWidth",layout.mobile_min);put("adaptiveBottomNavigationMaxItems",layout.bottom);put("adaptiveCompactSpacingBreakpoint",layout.compact);put("adaptiveNavRailMaxWidthRatio",layout.rail);
    put("adaptiveDrawerMarginSafety",layout.safety);put("adaptiveDrawerEnterDuration",layout.enter);put("adaptiveDrawerExitDuration",layout.exit);put("adaptiveAnimatedTransitions",ready&&!is_wasm);
    put("android",is_android);put("ios",is_ios);put("cmakeSystemName",cmake_name(target));put("executableSuffix",target=="windows"?".exe":is_wasm?".html":"");
    put("sharedLibrarySuffix",target=="windows"?".dll":target=="macos"||is_ios?".dylib":target=="linux"||is_android?".so":"");put("directRunSupported",is_desktop&&!is_wasm);
    return Value(std::move(result));
}
}
