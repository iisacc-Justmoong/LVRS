use super::install;
use crate::cli::BootstrapArgs;
use anyhow::{Context, Result, bail};
use std::cmp::Ordering;
use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

const DESKTOP_MOBILE_PLATFORMS: &str = "macos;linux;windows;ios;android";
const DESKTOP_MOBILE_WASM_PLATFORMS: &str = "macos;linux;windows;ios;android;wasm";
const ENV_BOOTSTRAP_FRAMEWORK_PLATFORMS: &str = "LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS";

const MAIN_CPP_RELATIVE_PATH: &str = "main.cpp";
const MAIN_ROOT_OBJECT_MARKER: &str = "rootObject = QStringLiteral(\"Main\")";
const MAIN_BOOTSTRAP_CALL_MARKER: &str = "runBootstrappedQmlApp";

pub(crate) struct AutoBootstrapHints {
    pub(crate) injected: Vec<String>,
    pub(crate) warnings: Vec<String>,
}

pub fn run(mut args: BootstrapArgs, verbose: u8) -> Result<()> {
    let home_dir = install::resolve_home_dir()?;
    let install_prefix = install::resolve_install_prefix(args.install.prefix.clone(), &home_dir)?;
    let project_root = install::resolve_project_root(None, Some(&install_prefix))?;
    ensure_main_entrypoints_ready(&project_root)?;

    let (platforms, source) =
        resolve_bootstrap_platforms(args.install.platforms.clone(), args.with_wasm);
    args.install.platforms = Some(platforms.clone());
    let selected_platforms = parse_platform_list(&platforms);
    if selected_platforms.is_empty() {
        bail!("[LVRS] bootstrap platform list is empty. set --platforms with at least one target.");
    }

    let hints = apply_auto_bootstrap_hints(&mut args.install.cmake_args, &selected_platforms)?;

    println!(
        "[LVRS] Bootstrap profile: {}",
        if args.with_wasm {
            "desktop+mobile+wasm"
        } else {
            "desktop+mobile"
        }
    );
    println!("[LVRS] Bootstrap source : {source}");
    println!("[LVRS] Bootstrap targets: {}", selected_platforms.join(";"));
    println!(
        "[LVRS] Main entrypoint : {}",
        project_root.join(MAIN_CPP_RELATIVE_PATH).display()
    );
    println!("[LVRS] Main root object: Main");

    if !hints.injected.is_empty() {
        println!("[LVRS] Auto-injected hints:");
        for item in &hints.injected {
            println!("  - {item}");
        }
    }
    if !hints.warnings.is_empty() {
        println!("[LVRS] Auto-detect warnings:");
        for item in &hints.warnings {
            println!("  - {item}");
        }
    }

    install::run_with_project_root(args.install, verbose, project_root)
}

pub(crate) fn resolve_bootstrap_platforms(
    cli_platforms: Option<String>,
    with_wasm: bool,
) -> (String, &'static str) {
    if let Some(value) = cli_platforms {
        let trimmed = value.trim();
        if !trimmed.is_empty() {
            return (normalize_platform_list(trimmed), "cli");
        }
    }

    if let Ok(value) = env::var(ENV_BOOTSTRAP_FRAMEWORK_PLATFORMS) {
        let trimmed = value.trim();
        if !trimmed.is_empty() {
            return (
                normalize_platform_list(trimmed),
                ENV_BOOTSTRAP_FRAMEWORK_PLATFORMS,
            );
        }
    }

    (
        default_bootstrap_platforms(with_wasm).to_string(),
        "bootstrap-profile-default",
    )
}

fn default_bootstrap_platforms(with_wasm: bool) -> &'static str {
    default_bootstrap_platforms_for_host(install::detect_host_platform(), with_wasm)
}

fn default_bootstrap_platforms_for_host(host_platform: &str, with_wasm: bool) -> &'static str {
    match host_platform {
        "linux" => {
            if with_wasm {
                "linux;wasm"
            } else {
                "linux"
            }
        }
        "macos" => {
            if with_wasm {
                "macos;ios;android;wasm"
            } else {
                "macos;ios;android"
            }
        }
        "windows" => {
            if with_wasm {
                "windows;android;wasm"
            } else {
                "windows;android"
            }
        }
        _ => {
            if with_wasm {
                DESKTOP_MOBILE_WASM_PLATFORMS
            } else {
                DESKTOP_MOBILE_PLATFORMS
            }
        }
    }
}

fn normalize_platform_list(input: &str) -> String {
    input.replace(',', ";")
}

pub(crate) fn parse_platform_list(input: &str) -> Vec<String> {
    let mut platforms = Vec::new();
    for item in input.split(';') {
        let token = item.trim().to_lowercase();
        if token.is_empty() {
            continue;
        }
        if !platforms.contains(&token) {
            platforms.push(token);
        }
    }
    platforms
}

pub(crate) fn ensure_main_entrypoints_ready(project_root: &Path) -> Result<()> {
    let main_cpp = project_root.join(MAIN_CPP_RELATIVE_PATH);
    if !main_cpp.is_file() {
        bail!(
            "[LVRS] bootstrap requires Main entry asset: missing {}",
            main_cpp.display()
        );
    }

    let main_cpp_content = fs::read_to_string(&main_cpp)
        .with_context(|| format!("failed to read {}", main_cpp.display()))?;
    let mut missing_settings = Vec::new();
    if !main_cpp_content.contains(MAIN_ROOT_OBJECT_MARKER) {
        missing_settings.push("rootObject default(Main)");
    }
    if !main_cpp_content.contains(MAIN_BOOTSTRAP_CALL_MARKER) {
        missing_settings.push("runBootstrappedQmlApp call");
    }
    if missing_settings.is_empty() {
        return Ok(());
    }

    bail!(
        "[LVRS] bootstrap requires Main entry settings in {}: missing {}",
        main_cpp.display(),
        missing_settings.join(", ")
    )
}

pub(crate) fn apply_auto_bootstrap_hints(
    cmake_args: &mut Vec<String>,
    selected_platforms: &[String],
) -> Result<AutoBootstrapHints> {
    let mut injected = Vec::new();
    let mut warnings = Vec::new();

    let home_dir = resolve_home_dir().ok();
    let qt_version_root = detect_qt_version_root();

    for platform in selected_platforms {
        if let Some(cmake_var) = platform_to_qt_prefix_var(platform) {
            if should_inject_var(cmake_args, cmake_var) {
                if let Some(prefix) =
                    detect_qt_prefix_for_platform(platform, qt_version_root.as_deref())
                {
                    inject_cmake_var(cmake_args, cmake_var, &prefix);
                    injected.push(format!("{}={}", cmake_var, prefix.display()));
                } else {
                    warnings.push(format!(
                        "Qt prefix for '{}' was not auto-detected. set {} manually.",
                        platform, cmake_var
                    ));
                }
            }
        }
    }

    if selected_platforms.iter().any(|p| p == "ios") {
        if let Some(warning) = detect_ios_bootstrap_toolchain_warning() {
            warnings.push(warning);
        }
        if should_inject_var(cmake_args, "LVRS_BOOTSTRAP_IOS_ARCHITECTURES") {
            inject_cmake_var(cmake_args, "LVRS_BOOTSTRAP_IOS_ARCHITECTURES", "arm64");
            injected.push("LVRS_BOOTSTRAP_IOS_ARCHITECTURES=arm64".to_string());
        }
    }

    if selected_platforms.iter().any(|p| p == "android") {
        let android_sdk = detect_android_sdk_root(home_dir.as_deref());
        if should_inject_var(cmake_args, "LVRS_BOOTSTRAP_ANDROID_SDK_ROOT") {
            if let Some(path) = &android_sdk {
                inject_cmake_var(cmake_args, "LVRS_BOOTSTRAP_ANDROID_SDK_ROOT", path);
                injected.push(format!(
                    "LVRS_BOOTSTRAP_ANDROID_SDK_ROOT={}",
                    path.display()
                ));
            } else {
                warnings.push("Android SDK root was not auto-detected.".to_string());
            }
        }

        if should_inject_var(cmake_args, "LVRS_BOOTSTRAP_ANDROID_NDK") {
            if let Some(path) = detect_android_ndk_root(android_sdk.as_deref()) {
                inject_cmake_var(cmake_args, "LVRS_BOOTSTRAP_ANDROID_NDK", &path);
                injected.push(format!("LVRS_BOOTSTRAP_ANDROID_NDK={}", path.display()));
            } else {
                warnings.push("Android NDK root was not auto-detected.".to_string());
            }
        }
    }

    if selected_platforms.iter().any(|p| p == "wasm") {
        let emsdk_root = detect_emsdk_root(home_dir.as_deref());
        if should_inject_var(cmake_args, "LVRS_BOOTSTRAP_EMSDK_ROOT") {
            if let Some(path) = &emsdk_root {
                inject_cmake_var(cmake_args, "LVRS_BOOTSTRAP_EMSDK_ROOT", path);
                injected.push(format!("LVRS_BOOTSTRAP_EMSDK_ROOT={}", path.display()));
            } else {
                warnings.push("emsdk root was not auto-detected.".to_string());
            }
        }

        if should_inject_var(cmake_args, "LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE") {
            if let Some(path) = detect_emscripten_toolchain(emsdk_root.as_deref()) {
                inject_cmake_var(
                    cmake_args,
                    "LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE",
                    &path,
                );
                injected.push(format!(
                    "LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE={}",
                    path.display()
                ));
            } else {
                warnings.push(
                    "Emscripten toolchain file was not auto-detected. set LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE manually."
                        .to_string(),
                );
            }
        }
    }

    Ok(AutoBootstrapHints { injected, warnings })
}

fn platform_to_qt_prefix_var(platform: &str) -> Option<&'static str> {
    match platform {
        "macos" => Some("LVRS_BOOTSTRAP_QT_PREFIX_MACOS"),
        "linux" => Some("LVRS_BOOTSTRAP_QT_PREFIX_LINUX"),
        "windows" => Some("LVRS_BOOTSTRAP_QT_PREFIX_WINDOWS"),
        "ios" => Some("LVRS_BOOTSTRAP_QT_PREFIX_IOS"),
        "android" => Some("LVRS_BOOTSTRAP_QT_PREFIX_ANDROID"),
        "wasm" => Some("LVRS_BOOTSTRAP_QT_PREFIX_WASM"),
        _ => None,
    }
}

fn platform_qt_env_hints(platform: &str) -> &'static [&'static str] {
    match platform {
        "macos" => &["QT_MACOS_PREFIX", "QT_HOST_PREFIX"],
        "linux" => &["QT_LINUX_PREFIX", "QT_HOST_PREFIX"],
        "windows" => &["QT_WINDOWS_PREFIX", "QT_HOST_PREFIX"],
        "ios" => &["QT_IOS_PREFIX"],
        "android" => &["QT_ANDROID_PREFIX"],
        "wasm" => &["QT_WASM_PREFIX"],
        _ => &[],
    }
}

fn should_inject_var(cmake_args: &[String], key: &str) -> bool {
    !has_cmake_definition(cmake_args, key) && !is_env_var_set(key)
}

fn has_cmake_definition(cmake_args: &[String], key: &str) -> bool {
    let prefix = format!("-D{}=", key);
    cmake_args.iter().any(|arg| arg.starts_with(&prefix))
}

fn is_env_var_set(name: &str) -> bool {
    match env::var(name) {
        Ok(value) => !value.trim().is_empty(),
        Err(_) => false,
    }
}

fn inject_cmake_var<V: AsRef<Path>>(cmake_args: &mut Vec<String>, key: &str, value: V) {
    cmake_args.push(format!("-D{}={}", key, value.as_ref().display()));
}

fn detect_qt_prefix_for_platform(
    platform: &str,
    qt_version_root: Option<&Path>,
) -> Option<PathBuf> {
    for env_name in platform_qt_env_hints(platform) {
        if let Ok(value) = env::var(env_name) {
            let candidate = PathBuf::from(value.trim());
            if is_qt_prefix_dir(&candidate) {
                return Some(candidate);
            }
        }
    }

    let root = qt_version_root?;
    for candidate in default_qt_prefix_candidates(platform, root) {
        if is_qt_prefix_dir(&candidate) {
            return Some(candidate);
        }
    }

    None
}

fn default_qt_prefix_candidates(platform: &str, qt_version_root: &Path) -> Vec<PathBuf> {
    match platform {
        "macos" => vec![qt_version_root.join("macos")],
        "linux" => vec![qt_version_root.join("gcc_64")],
        "windows" => vec![
            qt_version_root.join("msvc2022_64"),
            qt_version_root.join("msvc2019_64"),
            qt_version_root.join("mingw_64"),
        ],
        "ios" => vec![qt_version_root.join("ios")],
        "android" => vec![
            qt_version_root.join("android_arm64_v8a"),
            qt_version_root.join("android_x86_64"),
            qt_version_root.join("android"),
        ],
        "wasm" => vec![
            qt_version_root.join("wasm_singlethread"),
            qt_version_root.join("wasm_multithread"),
            qt_version_root.join("wasm_32"),
            qt_version_root.join("wasm"),
        ],
        _ => Vec::new(),
    }
}

fn is_qt_prefix_dir(path: &Path) -> bool {
    path.is_dir()
        && (path
            .join("lib")
            .join("cmake")
            .join("Qt6")
            .join("Qt6Config.cmake")
            .is_file()
            || path.join("Qt6Config.cmake").is_file()
            || path.join("bin").join("qtpaths").is_file()
            || path.join("bin").join("qmake").is_file())
}

fn detect_qt_version_root() -> Option<PathBuf> {
    if let Ok(value) = env::var("QT_VERSION_ROOT") {
        let candidate = PathBuf::from(value.trim());
        if candidate.is_dir() {
            return Some(candidate);
        }
    }

    let qt_home = PathBuf::from("/Volumes/Storage/Qt");
    if !qt_home.is_dir() {
        return None;
    }

    latest_version_dir(&qt_home).or(Some(qt_home))
}

fn latest_version_dir(parent: &Path) -> Option<PathBuf> {
    let mut candidates = Vec::new();
    let read_dir = fs::read_dir(parent).ok()?;
    for entry in read_dir.flatten() {
        let path = entry.path();
        if !path.is_dir() {
            continue;
        }
        let name = entry.file_name().to_string_lossy().to_string();
        if is_version_text(&name) {
            candidates.push(path);
        }
    }

    candidates.sort_by(|a, b| compare_version_paths(a, b));
    candidates.pop()
}

fn is_version_text(text: &str) -> bool {
    if text.is_empty() {
        return false;
    }
    text.chars().all(|ch| ch.is_ascii_digit() || ch == '.')
}

fn compare_version_paths(left: &Path, right: &Path) -> Ordering {
    let left_name = left
        .file_name()
        .map(|name| name.to_string_lossy().to_string())
        .unwrap_or_default();
    let right_name = right
        .file_name()
        .map(|name| name.to_string_lossy().to_string())
        .unwrap_or_default();
    compare_version_text(&left_name, &right_name)
}

fn compare_version_text(left: &str, right: &str) -> Ordering {
    let left_parts = parse_version_parts(left);
    let right_parts = parse_version_parts(right);

    let max_len = left_parts.len().max(right_parts.len());
    for index in 0..max_len {
        let l = *left_parts.get(index).unwrap_or(&0);
        let r = *right_parts.get(index).unwrap_or(&0);
        if l != r {
            return l.cmp(&r);
        }
    }
    Ordering::Equal
}

fn parse_version_parts(text: &str) -> Vec<u32> {
    text.split('.')
        .map(|part| part.parse::<u32>().unwrap_or(0))
        .collect()
}

fn detect_android_sdk_root(home_dir: Option<&Path>) -> Option<PathBuf> {
    for env_name in [
        "LVRS_BOOTSTRAP_ANDROID_SDK_ROOT",
        "ANDROID_SDK_ROOT",
        "ANDROID_HOME",
    ] {
        if let Ok(value) = env::var(env_name) {
            let candidate = PathBuf::from(value.trim());
            if candidate.is_dir() {
                return Some(candidate);
            }
        }
    }

    let home = home_dir?;
    let candidates = [
        home.join("Library").join("Android").join("sdk"),
        home.join("Android").join("Sdk"),
    ];
    candidates.into_iter().find(|candidate| candidate.is_dir())
}

fn detect_android_ndk_root(android_sdk_root: Option<&Path>) -> Option<PathBuf> {
    for env_name in [
        "LVRS_BOOTSTRAP_ANDROID_NDK",
        "CMAKE_ANDROID_NDK",
        "ANDROID_NDK_ROOT",
        "ANDROID_NDK_HOME",
    ] {
        if let Ok(value) = env::var(env_name) {
            let candidate = PathBuf::from(value.trim());
            if candidate.is_dir() {
                return Some(candidate);
            }
        }
    }

    let sdk_root = android_sdk_root?;
    let ndk_parent = sdk_root.join("ndk");
    if !ndk_parent.is_dir() {
        return None;
    }

    let mut candidates = fs::read_dir(&ndk_parent)
        .ok()?
        .flatten()
        .map(|entry| entry.path())
        .filter(|path| path.is_dir())
        .collect::<Vec<_>>();
    candidates.sort_by(|a, b| b.file_name().cmp(&a.file_name()));
    candidates.into_iter().next()
}

fn detect_emsdk_root(home_dir: Option<&Path>) -> Option<PathBuf> {
    for env_name in ["LVRS_BOOTSTRAP_EMSDK_ROOT", "EMSDK"] {
        if let Ok(value) = env::var(env_name) {
            let candidate = PathBuf::from(value.trim());
            if candidate.is_dir() {
                return Some(candidate);
            }
        }
    }

    let mut candidates = Vec::new();
    if let Some(home) = home_dir {
        candidates.push(home.join("emsdk"));
        candidates.push(home.join(".emsdk"));
    }
    candidates.push(PathBuf::from("/opt/emsdk"));
    candidates.into_iter().find(|candidate| candidate.is_dir())
}

fn detect_emscripten_toolchain(emsdk_root: Option<&Path>) -> Option<PathBuf> {
    if let Ok(value) = env::var("LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE") {
        let candidate = PathBuf::from(value.trim());
        if candidate.is_file() {
            return Some(candidate);
        }
    }

    let root = emsdk_root?;
    let candidates = [
        root.join("upstream")
            .join("emscripten")
            .join("cmake")
            .join("Modules")
            .join("Platform")
            .join("Emscripten.cmake"),
        root.join("fastcomp")
            .join("emscripten")
            .join("cmake")
            .join("Modules")
            .join("Platform")
            .join("Emscripten.cmake"),
        root.join("emscripten")
            .join("cmake")
            .join("Modules")
            .join("Platform")
            .join("Emscripten.cmake"),
    ];
    candidates.into_iter().find(|candidate| candidate.is_file())
}

fn detect_ios_bootstrap_toolchain_warning() -> Option<String> {
    if install::detect_host_platform() != "macos" {
        return Some("iOS bootstrap requires a macOS host with full Xcode.".to_string());
    }

    let xcodebuild_output = match Command::new("xcodebuild").arg("-version").output() {
        Ok(output) => output,
        Err(error) => {
            return Some(format!("iOS bootstrap cannot run xcodebuild: {error}"));
        }
    };
    let xcodebuild_text = command_output_text(&xcodebuild_output.stdout, &xcodebuild_output.stderr);
    if !xcodebuild_output.status.success() {
        return Some(format!(
            "iOS bootstrap requires a selected full Xcode: {}",
            nonempty_or_default(&xcodebuild_text, "xcodebuild -version failed")
        ));
    }
    if !xcode_version_output_is_supported(&xcodebuild_text) {
        return Some(
            "iOS bootstrap requires xcodebuild to report a supported Xcode version.".to_string(),
        );
    }

    let xcrun_output = match Command::new("xcrun")
        .args(["--sdk", "iphoneos", "--show-sdk-path"])
        .output()
    {
        Ok(output) => output,
        Err(error) => {
            return Some(format!("iOS bootstrap cannot run xcrun: {error}"));
        }
    };
    let xcrun_text = command_output_text(&xcrun_output.stdout, &xcrun_output.stderr);
    if !xcrun_output.status.success() {
        return Some(format!(
            "iOS bootstrap requires an iPhoneOS SDK: {}",
            nonempty_or_default(&xcrun_text, "xcrun could not resolve the iphoneos SDK")
        ));
    }

    let sdk_path = String::from_utf8_lossy(&xcrun_output.stdout)
        .trim()
        .to_string();
    if !Path::new(&sdk_path).is_dir() {
        return Some(format!(
            "iOS bootstrap requires an existing iPhoneOS SDK path; xcrun returned '{}'.",
            sdk_path
        ));
    }

    None
}

fn command_output_text(stdout: &[u8], stderr: &[u8]) -> String {
    let mut text = String::new();
    text.push_str(String::from_utf8_lossy(stdout).trim());
    if !stderr.is_empty() {
        if !text.is_empty() {
            text.push(' ');
        }
        text.push_str(String::from_utf8_lossy(stderr).trim());
    }
    text.trim().to_string()
}

fn nonempty_or_default<'a>(value: &'a str, fallback: &'a str) -> &'a str {
    if value.trim().is_empty() {
        fallback
    } else {
        value.trim()
    }
}

fn xcode_version_output_is_supported(output: &str) -> bool {
    output.lines().any(|line| {
        let Some(version) = line.trim().strip_prefix("Xcode ") else {
            return false;
        };
        version
            .chars()
            .next()
            .map(|ch| ch.is_ascii_digit())
            .unwrap_or(false)
    })
}

fn resolve_home_dir() -> Result<PathBuf> {
    if let Ok(home) = env::var("HOME") {
        if !home.is_empty() {
            return Ok(PathBuf::from(home));
        }
    }
    if let Ok(user_profile) = env::var("USERPROFILE") {
        if !user_profile.is_empty() {
            return Ok(PathBuf::from(user_profile));
        }
    }
    bail!("HOME/USERPROFILE environment variable is required.")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn host_aware_bootstrap_defaults_include_linux_only_on_linux_hosts() {
        assert_eq!(
            default_bootstrap_platforms_for_host("linux", false),
            "linux"
        );
        assert_eq!(
            default_bootstrap_platforms_for_host("linux", true),
            "linux;wasm"
        );
        assert_eq!(
            default_bootstrap_platforms_for_host("macos", false),
            "macos;ios;android"
        );
        assert_eq!(
            default_bootstrap_platforms_for_host("macos", true),
            "macos;ios;android;wasm"
        );
        assert_eq!(
            default_bootstrap_platforms_for_host("windows", false),
            "windows;android"
        );
        assert_eq!(
            default_bootstrap_platforms_for_host("windows", true),
            "windows;android;wasm"
        );
    }

    #[test]
    fn xcode_version_output_requires_full_xcode_version_line() {
        assert!(xcode_version_output_is_supported(
            "Xcode 16.4\nBuild version 16F6\n"
        ));
        assert!(!xcode_version_output_is_supported(
            "CommandLineTools 16.4\nBuild version 16F6\n"
        ));
        assert!(!xcode_version_output_is_supported(
            "xcode-select: error: tool 'xcodebuild' requires Xcode"
        ));
    }
}
