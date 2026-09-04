use crate::cli::InstallArgs;
use anyhow::{Context, Result, bail};
use std::env;
use std::ffi::{OsStr, OsString};
use std::fs;
use std::io::Read;
use std::path::{Component, Path, PathBuf};
use std::process::{Command, Stdio};
use std::time::{SystemTime, UNIX_EPOCH};

const ENV_BOOTSTRAP_FRAMEWORK_PLATFORMS: &str = "LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS";
const ENV_CMAKE_PREFIX_PATH: &str = "CMAKE_PREFIX_PATH";
const ENV_LVRS_INSTALL_PREFIX: &str = "LVRS_INSTALL_PREFIX";
const ENV_LVRS_ROOT: &str = "LVRS_ROOT";
const ENV_LVRS_PROJECT_ROOT: &str = "LVRS_PROJECT_ROOT";
const ENV_QT6_DIR: &str = "Qt6_DIR";
const INSTALL_SOURCE_INFO_FILE: &str = "INSTALL_SOURCE_INFO.txt";

#[derive(Debug, Clone)]
pub(crate) struct LinuxQtAutoConfig {
    pub(crate) prefix: PathBuf,
    pub(crate) qt6_dir: PathBuf,
    pub(crate) source: String,
    pub(crate) injected: Vec<String>,
}

#[derive(Debug)]
struct CommandCapture {
    success: bool,
    output: String,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum LinuxPackageManager {
    Apt,
    Dnf,
    Pacman,
    Zypper,
    Apk,
}

#[derive(Debug, Clone)]
struct LinuxPackageInstallPlan {
    manager: LinuxPackageManager,
    display_commands: Vec<String>,
    exec_commands: Vec<(String, Vec<String>)>,
}

#[derive(Debug, Clone)]
struct LinuxOsRelease {
    id: String,
    id_like: Vec<String>,
}

pub fn run(args: InstallArgs, _verbose: u8) -> Result<()> {
    run_internal(args, _verbose, None)
}

pub fn run_with_project_root(args: InstallArgs, _verbose: u8, project_root: PathBuf) -> Result<()> {
    run_internal(args, _verbose, Some(project_root))
}

fn run_internal(
    args: InstallArgs,
    _verbose: u8,
    project_root_override: Option<PathBuf>,
) -> Result<()> {
    if args.force_x86_qt_tools {
        bail!("[LVRS] --force-x86-qt-tools is unsupported. Apple x86 paths are disabled.");
    }

    let home_dir = resolve_home_dir()?;
    let install_prefix = resolve_install_prefix(args.prefix.clone(), &home_dir)?;
    let project_root = resolve_project_root(project_root_override, Some(&install_prefix))?;

    let build_dir = project_root.join("build");
    validate_deprecated_build_dir(args.build_dir.as_deref(), &project_root)?;

    let platform_install_root = install_prefix.join("platforms");
    let host_platform = detect_host_platform();
    let host_install_prefix = platform_install_root.join(&host_platform);
    let bootstrap_framework_platforms =
        resolve_bootstrap_framework_platforms(args.platforms.as_deref(), host_platform);

    let build_type = args
        .build_type
        .or_else(|| env::var("CMAKE_BUILD_TYPE").ok())
        .unwrap_or_else(|| "Release".to_string());
    let source_snapshot = !args.no_source_snapshot;
    let register_cmake_registry = !args.no_registry;
    let build_examples = !args.without_examples;
    let build_tests = !args.without_tests;

    let source_install_dir = install_prefix.join("src").join("LVRS");
    let project_root_is_installed_snapshot =
        paths_refer_to_same_location(&project_root, &source_install_dir);
    let package_config_dir = host_install_prefix.join("lib").join("cmake").join("LVRS");

    let lvrs_build_examples_value = if build_examples { "ON" } else { "OFF" };
    let lvrs_build_tests_value = if build_tests { "ON" } else { "OFF" };

    let mut configure_args = vec![
        "-S".to_string(),
        project_root.display().to_string(),
        "-B".to_string(),
        build_dir.display().to_string(),
        format!("-DCMAKE_INSTALL_PREFIX={}", install_prefix.display()),
        format!("-DCMAKE_BUILD_TYPE={build_type}"),
        "-DLVRS_BUILD_SHARED_LIBS=ON".to_string(),
        format!("-DLVRS_BUILD_EXAMPLES={lvrs_build_examples_value}"),
        format!("-DLVRS_BUILD_TESTS={lvrs_build_tests_value}"),
        format!(
            "-DLVRS_BOOTSTRAP_INSTALL_ROOT={}",
            platform_install_root.display()
        ),
        format!("-DLVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS={bootstrap_framework_platforms}"),
        "-DLVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES=OFF".to_string(),
        "-DLVRS_BOOTSTRAP_LVRS_BUILD_TESTS=OFF".to_string(),
        "-DLVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS=ON".to_string(),
        "-DLVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE=ON".to_string(),
    ];
    configure_args.extend(args.cmake_args.clone());

    ensure_cmake_available()?;
    let linux_qt_auto_config = ensure_linux_host_prerequisites(
        host_platform,
        &home_dir,
        &mut configure_args,
        &args.cmake_args,
        args.install_linux_deps,
    )?;

    println!("[LVRS] Project root : {}", project_root.display());
    println!("[LVRS] Build dir    : {}", build_dir.display());
    println!("[LVRS] Install dir  : {}", install_prefix.display());
    println!("[LVRS] Platforms dir: {}", platform_install_root.display());
    println!("[LVRS] Host platform: {host_platform}");
    println!("[LVRS] Bootstrap targets: {bootstrap_framework_platforms}");
    println!("[LVRS] Build type   : {build_type}");
    println!(
        "[LVRS] Registry     : {}",
        flag_as_number(register_cmake_registry)
    );
    println!("[LVRS] Snapshot     : {}", flag_as_number(source_snapshot));
    println!("[LVRS] Examples     : {}", flag_as_number(build_examples));
    println!("[LVRS] Tests        : {}", flag_as_number(build_tests));
    println!("[LVRS] Clean mode   : forced reinstall");
    if let Some(config) = &linux_qt_auto_config {
        println!("[LVRS] Linux Qt     : {}", config.prefix.display());
        println!("[LVRS] Linux Qt6_DIR: {}", config.qt6_dir.display());
        println!("[LVRS] Qt detect    : {}", config.source);
    }
    if args.clean {
        println!("[LVRS] --clean is deprecated. Forced reinstall mode is always enabled.");
    }

    println!("[LVRS] Cleaning build directory...");
    clean_recreate_dir(&build_dir, "build")?;

    println!("[LVRS] Cleaning previous LVRS install artifacts...");
    for path in [
        install_prefix.join("platforms"),
        install_prefix.join("include").join("LVRS"),
        install_prefix.join("lib").join("cmake").join("LVRS"),
        install_prefix
            .join("lib")
            .join("qt6")
            .join("qml")
            .join("LVRS"),
        source_install_dir.clone(),
    ] {
        if project_root_is_installed_snapshot && path == source_install_dir {
            continue;
        }
        remove_path_with_stale_fallback(&path, &path.display().to_string())?;
    }

    for binary_path in [
        install_prefix.join("lib").join("libLVRS.dylib"),
        install_prefix.join("lib").join("libLVRS.so"),
        install_prefix.join("lib").join("libLVRS.a"),
        install_prefix.join("lib").join("LVRS.lib"),
        install_prefix.join("bin").join("LVRS.dll"),
    ] {
        remove_path_with_stale_fallback(&binary_path, &binary_path.display().to_string())?;
    }

    let configure_status = run_command("cmake", &configure_args);
    if !configure_status {
        eprintln!("[LVRS] Configure failed.");
        eprintln!(
            "[LVRS] If Qt is not auto-detected, pass Qt6_DIR or LVRS_BOOTSTRAP_QT_PREFIX_LINUX, e.g.:"
        );
        eprintln!("       Qt6_DIR=/path/to/Qt/lib/cmake/Qt6 lvrs install");
        bail!("configure step failed");
    }

    let build_status = run_command(
        "cmake",
        &[
            "--build".to_string(),
            build_dir.display().to_string(),
            "--config".to_string(),
            build_type.clone(),
            "--target".to_string(),
            "bootstrap_lvrs_all".to_string(),
        ],
    );
    if !build_status {
        eprintln!("[LVRS] Build failed.");
        eprintln!(
            "[LVRS] Check requested platform prerequisites: Apple targets use arm64 Qt kits; Android needs a valid SDK+NDK; WASM needs emsdk or LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE."
        );
        bail!("build step failed");
    }

    println!("[LVRS] Multi-platform framework install completed.");

    if build_examples && source_snapshot {
        build_host_examples(&build_dir, &build_type)?;
    }

    if source_snapshot {
        install_source_snapshot(
            &project_root,
            &source_install_dir,
            &build_dir,
            build_examples,
        )?;
    }

    let cli_binary = install_cli_binary(&env::current_exe()?, &install_prefix)?;

    if register_cmake_registry {
        register_cmake_package(&home_dir, &install_prefix, &package_config_dir)?;
    }

    let env_file = install_prefix.join("env.sh");
    write_env_helper(
        &env_file,
        &platform_install_root,
        &host_platform,
        &host_install_prefix,
        &install_prefix,
    )?;

    println!("[LVRS] Install completed.");
    println!(
        "[LVRS] CMake package dir : {}",
        package_config_dir.display()
    );
    println!(
        "[LVRS] Platforms root    : {}",
        platform_install_root.display()
    );
    println!("[LVRS] Env helper        : {}", env_file.display());
    println!("[LVRS] CLI binary        : {}", cli_binary.display());
    println!("[LVRS] Downstream CMake  : find_package(LVRS CONFIG REQUIRED)");

    Ok(())
}

fn flag_as_number(value: bool) -> u8 {
    if value { 1 } else { 0 }
}

pub(crate) fn resolve_home_dir() -> Result<PathBuf> {
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

pub(crate) fn resolve_install_prefix(
    cli_prefix: Option<PathBuf>,
    home_dir: &Path,
) -> Result<PathBuf> {
    resolve_install_prefix_with_env(cli_prefix, env::var(ENV_LVRS_INSTALL_PREFIX).ok(), home_dir)
}

fn resolve_install_prefix_with_env(
    cli_prefix: Option<PathBuf>,
    env_prefix: Option<String>,
    home_dir: &Path,
) -> Result<PathBuf> {
    if let Some(prefix) = cli_prefix {
        return normalize_user_path(prefix, home_dir);
    }

    if let Some(value) = env_prefix {
        let trimmed = value.trim();
        if !trimmed.is_empty() {
            return normalize_user_path(PathBuf::from(trimmed), home_dir);
        }
    }

    Ok(home_dir.join(".local").join("SDK").join("LVRS"))
}

fn normalize_user_path(path: PathBuf, home_dir: &Path) -> Result<PathBuf> {
    let expanded = expand_tilde_path(&path, home_dir);
    if expanded.is_absolute() {
        Ok(expanded)
    } else {
        Ok(env::current_dir()
            .context("failed to read current working directory")?
            .join(expanded))
    }
}

fn expand_tilde_path(path: &Path, home_dir: &Path) -> PathBuf {
    let raw = path.to_string_lossy();
    if raw == "~" {
        return home_dir.to_path_buf();
    }
    if let Some(stripped) = raw.strip_prefix("~/") {
        return home_dir.join(stripped);
    }
    path.to_path_buf()
}

pub(crate) fn ensure_linux_host_prerequisites(
    host_platform: &str,
    home_dir: &Path,
    configure_args: &mut Vec<String>,
    user_cmake_args: &[String],
    install_linux_deps: bool,
) -> Result<Option<LinuxQtAutoConfig>> {
    if host_platform != "linux" {
        ensure_cmake_available()?;
        return Ok(None);
    }

    let explicit_qt_hint = has_explicit_linux_qt_hint(user_cmake_args);
    let package_install_plan = detect_linux_package_install_plan();
    let mut package_install_attempted = false;
    let missing_tools = collect_missing_linux_host_tools();
    if !missing_tools.is_empty() {
        if install_linux_deps {
            let Some(plan) = package_install_plan.as_ref() else {
                eprintln!(
                    "[LVRS] Missing Linux host tools: {}",
                    missing_tools.join(", ")
                );
                bail!("linux host dependencies missing");
            };
            println!(
                "[LVRS] Installing Linux host dependencies: {}",
                missing_tools.join(", ")
            );
            run_linux_package_install(plan)?;
            package_install_attempted = true;
        } else {
            eprintln!(
                "[LVRS] Missing Linux host tools: {}",
                missing_tools.join(", ")
            );
            print_linux_dependency_install_hint(package_install_plan.as_ref());
            bail!("linux host dependencies missing");
        }
    }

    ensure_cmake_available()?;

    let preflight_args = collect_linux_preflight_cmake_args(user_cmake_args);
    let mut initial_preflight = run_linux_dependency_preflight(&preflight_args)?;
    if initial_preflight.success {
        return Ok(None);
    }

    if explicit_qt_hint {
        eprintln!("[LVRS] Linux dependency preflight failed.");
        eprintln!(
            "[LVRS] Explicit Qt hints are set. Fix Qt6_DIR/LVRS_BOOTSTRAP_QT_PREFIX_LINUX and retry."
        );
        eprintln!(
            "[LVRS] Preflight tail:\n{}",
            tail_lines(&initial_preflight.output, 12)
        );
        print_linux_dependency_install_hint(package_install_plan.as_ref());
        bail!("linux dependency preflight failed");
    }

    let mut detected_qt_install = detect_linux_qt_install(home_dir);
    if detected_qt_install.is_none() && install_linux_deps && !package_install_attempted {
        if let Some(plan) = package_install_plan.as_ref() {
            println!(
                "[LVRS] Qt development packages were not detected. Installing Linux distro dependencies..."
            );
            run_linux_package_install(plan)?;
            ensure_cmake_available()?;
            initial_preflight = run_linux_dependency_preflight(&preflight_args)?;
            if initial_preflight.success {
                return Ok(None);
            }
            detected_qt_install = detect_linux_qt_install(home_dir);
        }
    }

    let Some(mut qt_install) = detected_qt_install else {
        eprintln!("[LVRS] Linux dependency preflight failed.");
        eprintln!(
            "[LVRS] Required dependency: Qt 6.5+ development package with Quick, QuickControls2, Qml, Svg, Network, and Qt host tools."
        );
        eprintln!(
            "[LVRS] Searched Qt hints: Qt6_DIR, LVRS_BOOTSTRAP_QT_PREFIX_LINUX, LVRS_BOOTSTRAP_QT_PREFIX, QT_LINUX_PREFIX, QT_HOST_PREFIX, QTDIR, CMAKE_PREFIX_PATH, qtpaths6/qmake6, ~/Qt, /opt/Qt, distro Qt locations."
        );
        eprintln!(
            "[LVRS] Preflight tail:\n{}",
            tail_lines(&initial_preflight.output, 12)
        );
        print_linux_dependency_install_hint(package_install_plan.as_ref());
        bail!("linux dependency preflight failed");
    };

    let mut retry_preflight_args = preflight_args;
    let mut injected = Vec::new();
    if inject_cmake_definition(configure_args, ENV_QT6_DIR, &qt_install.qt6_dir) {
        injected.push(format!(
            "-D{}={}",
            ENV_QT6_DIR,
            qt_install.qt6_dir.display()
        ));
    }
    if inject_cmake_definition(
        configure_args,
        "LVRS_BOOTSTRAP_QT_PREFIX_LINUX",
        &qt_install.prefix,
    ) {
        injected.push(format!(
            "-DLVRS_BOOTSTRAP_QT_PREFIX_LINUX={}",
            qt_install.prefix.display()
        ));
    }
    inject_cmake_definition(&mut retry_preflight_args, ENV_QT6_DIR, &qt_install.qt6_dir);

    let retry_preflight = run_linux_dependency_preflight(&retry_preflight_args)?;
    if !retry_preflight.success {
        eprintln!("[LVRS] Linux dependency preflight failed after Qt auto-detect.");
        eprintln!("[LVRS] Auto-detected Qt source : {}", qt_install.source);
        eprintln!(
            "[LVRS] Auto-detected Qt prefix : {}",
            qt_install.prefix.display()
        );
        eprintln!(
            "[LVRS] Auto-detected Qt6_DIR : {}",
            qt_install.qt6_dir.display()
        );
        eprintln!(
            "[LVRS] Preflight tail:\n{}",
            tail_lines(&retry_preflight.output, 12)
        );
        print_linux_dependency_install_hint(package_install_plan.as_ref());
        bail!("linux dependency preflight failed");
    }

    qt_install.injected = injected;
    Ok(Some(qt_install))
}

fn collect_missing_linux_host_tools() -> Vec<&'static str> {
    let mut missing = Vec::new();
    if !program_exists("cmake") {
        missing.push("cmake");
    }
    if !(program_exists("c++") || program_exists("g++") || program_exists("clang++")) {
        missing.push("C++ compiler");
    }
    if !(program_exists("make")
        || program_exists("gmake")
        || program_exists("ninja")
        || program_exists("ninja-build"))
    {
        missing.push("build tool (make or ninja)");
    }
    missing
}

fn program_exists(program: &str) -> bool {
    find_program_in_path(program).is_some()
}

fn find_program_in_path(program: &str) -> Option<PathBuf> {
    let path_value = env::var_os("PATH")?;
    for entry in env::split_paths(&path_value) {
        let candidate = entry.join(program);
        if candidate.is_file() {
            return Some(candidate);
        }
    }
    None
}

fn detect_linux_package_install_plan() -> Option<LinuxPackageInstallPlan> {
    let os_release = read_linux_os_release().unwrap_or_else(|| LinuxOsRelease {
        id: String::new(),
        id_like: Vec::new(),
    });
    let manager = detect_linux_package_manager(&os_release)?;
    Some(build_linux_package_install_plan(manager))
}

fn read_linux_os_release() -> Option<LinuxOsRelease> {
    let content = fs::read_to_string("/etc/os-release").ok()?;
    let mut id = String::new();
    let mut id_like = Vec::new();

    for line in content.lines() {
        let Some((raw_key, raw_value)) = line.split_once('=') else {
            continue;
        };
        let key = raw_key.trim();
        let value = raw_value.trim().trim_matches('"');
        match key {
            "ID" => id = value.to_lowercase(),
            "ID_LIKE" => {
                id_like = value
                    .split_whitespace()
                    .map(|item| item.to_lowercase())
                    .collect()
            }
            _ => {}
        }
    }

    Some(LinuxOsRelease { id, id_like })
}

fn detect_linux_package_manager(os_release: &LinuxOsRelease) -> Option<LinuxPackageManager> {
    let distro_tokens = {
        let mut tokens = Vec::new();
        if !os_release.id.is_empty() {
            tokens.push(os_release.id.as_str());
        }
        for item in &os_release.id_like {
            tokens.push(item.as_str());
        }
        tokens
    };

    if program_exists("apt-get")
        && distro_tokens
            .iter()
            .any(|token| matches!(*token, "debian" | "ubuntu" | "linuxmint" | "pop"))
    {
        return Some(LinuxPackageManager::Apt);
    }
    if program_exists("dnf")
        && distro_tokens
            .iter()
            .any(|token| matches!(*token, "fedora" | "rhel" | "centos"))
    {
        return Some(LinuxPackageManager::Dnf);
    }
    if program_exists("pacman")
        && distro_tokens
            .iter()
            .any(|token| matches!(*token, "arch" | "manjaro"))
    {
        return Some(LinuxPackageManager::Pacman);
    }
    if program_exists("zypper")
        && distro_tokens
            .iter()
            .any(|token| matches!(*token, "opensuse" | "suse" | "sles"))
    {
        return Some(LinuxPackageManager::Zypper);
    }
    if program_exists("apk") && distro_tokens.iter().any(|token| matches!(*token, "alpine")) {
        return Some(LinuxPackageManager::Apk);
    }

    if program_exists("apt-get") {
        Some(LinuxPackageManager::Apt)
    } else if program_exists("dnf") {
        Some(LinuxPackageManager::Dnf)
    } else if program_exists("pacman") {
        Some(LinuxPackageManager::Pacman)
    } else if program_exists("zypper") {
        Some(LinuxPackageManager::Zypper)
    } else if program_exists("apk") {
        Some(LinuxPackageManager::Apk)
    } else {
        None
    }
}

fn build_linux_package_install_plan(manager: LinuxPackageManager) -> LinuxPackageInstallPlan {
    let is_root = linux_user_is_root();
    let sudo_available = program_exists("sudo");
    let prefix = if is_root {
        None
    } else if sudo_available {
        Some("sudo")
    } else {
        None
    };

    match manager {
        LinuxPackageManager::Apt => LinuxPackageInstallPlan {
            manager,
            display_commands: vec![
                render_prefixed_command(prefix, "apt-get", &["update"]),
                render_prefixed_command(
                    prefix,
                    "apt-get",
                    &[
                        "install",
                        "-y",
                        "build-essential",
                        "cmake",
                        "ninja-build",
                        "pkg-config",
                        "qt6-base-dev",
                        "qt6-base-dev-tools",
                        "qt6-declarative-dev",
                        "qt6-declarative-dev-tools",
                        "qt6-svg-dev",
                    ],
                ),
            ],
            exec_commands: vec![
                prefixed_command(prefix, "apt-get", &["update"]),
                prefixed_command(
                    prefix,
                    "apt-get",
                    &[
                        "install",
                        "-y",
                        "build-essential",
                        "cmake",
                        "ninja-build",
                        "pkg-config",
                        "qt6-base-dev",
                        "qt6-base-dev-tools",
                        "qt6-declarative-dev",
                        "qt6-declarative-dev-tools",
                        "qt6-svg-dev",
                    ],
                ),
            ],
        },
        LinuxPackageManager::Dnf => LinuxPackageInstallPlan {
            manager,
            display_commands: vec![render_prefixed_command(
                prefix,
                "dnf",
                &[
                    "install",
                    "-y",
                    "gcc-c++",
                    "cmake",
                    "ninja-build",
                    "pkgconf-pkg-config",
                    "qt6-qtbase-devel",
                    "qt6-qtdeclarative-devel",
                    "qt6-qtsvg-devel",
                ],
            )],
            exec_commands: vec![prefixed_command(
                prefix,
                "dnf",
                &[
                    "install",
                    "-y",
                    "gcc-c++",
                    "cmake",
                    "ninja-build",
                    "pkgconf-pkg-config",
                    "qt6-qtbase-devel",
                    "qt6-qtdeclarative-devel",
                    "qt6-qtsvg-devel",
                ],
            )],
        },
        LinuxPackageManager::Pacman => LinuxPackageInstallPlan {
            manager,
            display_commands: vec![render_prefixed_command(
                prefix,
                "pacman",
                &[
                    "-S",
                    "--needed",
                    "base-devel",
                    "cmake",
                    "ninja",
                    "pkgconf",
                    "qt6-base",
                    "qt6-declarative",
                    "qt6-svg",
                ],
            )],
            exec_commands: vec![prefixed_command(
                prefix,
                "pacman",
                &[
                    "-S",
                    "--needed",
                    "base-devel",
                    "cmake",
                    "ninja",
                    "pkgconf",
                    "qt6-base",
                    "qt6-declarative",
                    "qt6-svg",
                ],
            )],
        },
        LinuxPackageManager::Zypper => LinuxPackageInstallPlan {
            manager,
            display_commands: vec![render_prefixed_command(
                prefix,
                "zypper",
                &[
                    "install",
                    "-y",
                    "gcc-c++",
                    "cmake",
                    "ninja",
                    "pkg-config",
                    "qt6-base-devel",
                    "qt6-declarative-devel",
                    "qt6-svg-devel",
                ],
            )],
            exec_commands: vec![prefixed_command(
                prefix,
                "zypper",
                &[
                    "install",
                    "-y",
                    "gcc-c++",
                    "cmake",
                    "ninja",
                    "pkg-config",
                    "qt6-base-devel",
                    "qt6-declarative-devel",
                    "qt6-svg-devel",
                ],
            )],
        },
        LinuxPackageManager::Apk => LinuxPackageInstallPlan {
            manager,
            display_commands: vec![render_prefixed_command(
                prefix,
                "apk",
                &[
                    "add",
                    "build-base",
                    "cmake",
                    "ninja",
                    "pkgconf",
                    "qt6-qtbase-dev",
                    "qt6-qtdeclarative-dev",
                    "qt6-qtsvg-dev",
                ],
            )],
            exec_commands: vec![prefixed_command(
                prefix,
                "apk",
                &[
                    "add",
                    "build-base",
                    "cmake",
                    "ninja",
                    "pkgconf",
                    "qt6-qtbase-dev",
                    "qt6-qtdeclarative-dev",
                    "qt6-qtsvg-dev",
                ],
            )],
        },
    }
}

fn linux_user_is_root() -> bool {
    let output = Command::new("id")
        .arg("-u")
        .stdin(Stdio::null())
        .stderr(Stdio::null())
        .output();
    match output {
        Ok(value) if value.status.success() => String::from_utf8_lossy(&value.stdout).trim() == "0",
        _ => false,
    }
}

fn render_prefixed_command(prefix: Option<&str>, program: &str, args: &[&str]) -> String {
    let (exec_program, exec_args) = prefixed_command(prefix, program, args);
    std::iter::once(exec_program)
        .chain(exec_args)
        .collect::<Vec<_>>()
        .join(" ")
}

fn prefixed_command(prefix: Option<&str>, program: &str, args: &[&str]) -> (String, Vec<String>) {
    if let Some(prefix_program) = prefix {
        let mut exec_args = vec![program.to_string()];
        exec_args.extend(args.iter().map(|value| (*value).to_string()));
        (prefix_program.to_string(), exec_args)
    } else {
        (
            program.to_string(),
            args.iter().map(|value| (*value).to_string()).collect(),
        )
    }
}

fn run_linux_package_install(plan: &LinuxPackageInstallPlan) -> Result<()> {
    if !linux_user_is_root() && !program_exists("sudo") {
        bail!(
            "[LVRS] automatic Linux dependency install requires root or sudo. run the suggested package-manager command manually."
        );
    }

    for (program, args) in &plan.exec_commands {
        println!("[LVRS] Running: {}", render_command_line(program, args));
        let status = Command::new(program)
            .args(args)
            .stdin(Stdio::inherit())
            .stdout(Stdio::inherit())
            .stderr(Stdio::inherit())
            .status()
            .with_context(|| {
                format!("failed to run Linux dependency install command: {program}")
            })?;
        if !status.success() {
            bail!(
                "[LVRS] Linux dependency install command failed: {}",
                render_command_line(program, args)
            );
        }
    }

    Ok(())
}

fn render_command_line(program: &str, args: &[String]) -> String {
    std::iter::once(program.to_string())
        .chain(args.iter().cloned())
        .collect::<Vec<_>>()
        .join(" ")
}

fn print_linux_dependency_install_hint(plan: Option<&LinuxPackageInstallPlan>) {
    let Some(plan) = plan else {
        return;
    };

    eprintln!(
        "[LVRS] Suggested Linux dependency install command(s) via {}:",
        linux_package_manager_label(plan.manager)
    );
    for command in &plan.display_commands {
        eprintln!("       {}", command);
    }
    if !linux_user_is_root() && !program_exists("sudo") {
        eprintln!("       run the command(s) above as root.");
    } else {
        eprintln!("       or rerun with: lvrs install --install-linux-deps");
    }
}

fn linux_package_manager_label(manager: LinuxPackageManager) -> &'static str {
    match manager {
        LinuxPackageManager::Apt => "apt",
        LinuxPackageManager::Dnf => "dnf",
        LinuxPackageManager::Pacman => "pacman",
        LinuxPackageManager::Zypper => "zypper",
        LinuxPackageManager::Apk => "apk",
    }
}

fn collect_linux_preflight_cmake_args(cmake_args: &[String]) -> Vec<String> {
    let mut collected = Vec::new();
    let mut index = 0usize;
    while index < cmake_args.len() {
        let arg = &cmake_args[index];
        match arg.as_str() {
            "-G" | "-A" | "-T" => {
                if let Some(value) = cmake_args.get(index + 1) {
                    collected.push(arg.clone());
                    collected.push(value.clone());
                    index += 2;
                    continue;
                }
            }
            _ => {}
        }

        if matches_inline_generator_arg(arg) {
            collected.push(arg.clone());
            index += 1;
            continue;
        }

        if let Some(key) = cmake_definition_key(arg) {
            if matches!(
                key,
                ENV_QT6_DIR
                    | ENV_CMAKE_PREFIX_PATH
                    | "CMAKE_C_COMPILER"
                    | "CMAKE_CXX_COMPILER"
                    | "CMAKE_MAKE_PROGRAM"
                    | "CMAKE_TOOLCHAIN_FILE"
            ) {
                collected.push(arg.clone());
            }
        }

        index += 1;
    }

    collected
}

fn matches_inline_generator_arg(arg: &str) -> bool {
    arg.starts_with("-G") || arg.starts_with("-A") || arg.starts_with("-T")
}

fn cmake_definition_key(arg: &str) -> Option<&str> {
    if !arg.starts_with("-D") {
        return None;
    }

    let definition = &arg[2..];
    let key = definition.split_once('=').map(|(value, _)| value)?;
    Some(key.split(':').next().unwrap_or(key))
}

fn has_explicit_linux_qt_hint(cmake_args: &[String]) -> bool {
    if cmake_args.iter().any(|arg| {
        matches!(
            cmake_definition_key(arg),
            Some(ENV_QT6_DIR | "LVRS_BOOTSTRAP_QT_PREFIX_LINUX" | "LVRS_BOOTSTRAP_QT_PREFIX")
        )
    }) {
        return true;
    }

    for env_name in [
        ENV_QT6_DIR,
        "LVRS_BOOTSTRAP_QT_PREFIX_LINUX",
        "LVRS_BOOTSTRAP_QT_PREFIX",
        "QT_LINUX_PREFIX",
        "QT_HOST_PREFIX",
        "QTDIR",
    ] {
        if let Ok(value) = env::var(env_name) {
            if !value.trim().is_empty() {
                return true;
            }
        }
    }

    false
}

fn inject_cmake_definition(args: &mut Vec<String>, key: &str, value: &Path) -> bool {
    if args
        .iter()
        .any(|arg| cmake_definition_key(arg).is_some_and(|definition| definition == key))
    {
        return false;
    }

    args.push(format!("-D{}={}", key, value.display()));
    true
}

fn detect_linux_qt_install(home_dir: &Path) -> Option<LinuxQtAutoConfig> {
    for env_name in [
        ENV_QT6_DIR,
        "LVRS_BOOTSTRAP_QT_PREFIX_LINUX",
        "LVRS_BOOTSTRAP_QT_PREFIX",
        "QT_LINUX_PREFIX",
        "QT_HOST_PREFIX",
        "QTDIR",
    ] {
        if let Ok(value) = env::var(env_name) {
            for candidate in split_search_paths(&value) {
                if let Some((prefix, qt6_dir)) = resolve_qt_install_candidate(&candidate) {
                    return Some(LinuxQtAutoConfig {
                        prefix,
                        qt6_dir,
                        source: format!("env:{env_name}"),
                        injected: Vec::new(),
                    });
                }
            }
        }
    }

    if let Ok(value) = env::var(ENV_CMAKE_PREFIX_PATH) {
        for candidate in split_search_paths(&value) {
            if let Some((prefix, qt6_dir)) = resolve_qt_install_candidate(&candidate) {
                return Some(LinuxQtAutoConfig {
                    prefix,
                    qt6_dir,
                    source: format!("env:{ENV_CMAKE_PREFIX_PATH}"),
                    injected: Vec::new(),
                });
            }
        }
    }

    for tool in ["qtpaths6", "qtpaths", "qmake6", "qmake"] {
        for query_key in ["QT_INSTALL_LIBS", "QT_INSTALL_PREFIX"] {
            if let Some(candidate) = query_qt_install_path(tool, query_key) {
                if let Some((prefix, qt6_dir)) = resolve_qt_install_candidate(&candidate) {
                    return Some(LinuxQtAutoConfig {
                        prefix,
                        qt6_dir,
                        source: format!("{tool} -query {query_key}"),
                        injected: Vec::new(),
                    });
                }
            }
        }
    }

    for root in detect_qt_version_roots(Some(home_dir)) {
        for candidate in linux_qt_prefix_candidates(&root) {
            if let Some((prefix, qt6_dir)) = resolve_qt_install_candidate(&candidate) {
                return Some(LinuxQtAutoConfig {
                    prefix,
                    qt6_dir,
                    source: format!("search:{}", candidate.display()),
                    injected: Vec::new(),
                });
            }
        }
    }

    for candidate in [
        PathBuf::from("/usr/lib/x86_64-linux-gnu"),
        PathBuf::from("/usr/lib64"),
        PathBuf::from("/usr/lib"),
        PathBuf::from("/usr/local/lib64"),
        PathBuf::from("/usr/local/lib"),
        PathBuf::from("/usr/lib/qt6"),
        PathBuf::from("/usr/lib64/qt6"),
        PathBuf::from("/usr/local/lib/qt6"),
    ] {
        if let Some((prefix, qt6_dir)) = resolve_qt_install_candidate(&candidate) {
            return Some(LinuxQtAutoConfig {
                prefix,
                qt6_dir,
                source: format!("search:{}", candidate.display()),
                injected: Vec::new(),
            });
        }
    }

    None
}

fn split_search_paths(raw: &str) -> Vec<PathBuf> {
    raw.split([';', ':'])
        .filter_map(|value| {
            let trimmed = value.trim();
            if trimmed.is_empty() {
                None
            } else {
                Some(PathBuf::from(trimmed))
            }
        })
        .collect()
}

fn query_qt_install_path(tool: &str, query_key: &str) -> Option<PathBuf> {
    let output = Command::new(tool)
        .args(["-query", query_key])
        .stdin(Stdio::null())
        .stderr(Stdio::null())
        .output()
        .ok()?;
    if !output.status.success() {
        return None;
    }

    let value = String::from_utf8_lossy(&output.stdout).trim().to_string();
    if value.is_empty() {
        None
    } else {
        Some(PathBuf::from(value))
    }
}

fn resolve_qt_install_candidate(candidate: &Path) -> Option<(PathBuf, PathBuf)> {
    if candidate.is_file()
        && candidate
            .file_name()
            .is_some_and(|name| name == "Qt6Config.cmake")
    {
        let qt6_dir = candidate.parent()?.to_path_buf();
        let prefix = qt_prefix_from_qt6_dir(&qt6_dir)?;
        return Some((prefix, qt6_dir));
    }

    if !candidate.is_dir() {
        return None;
    }

    for relative in ["lib/cmake/Qt6", "lib64/cmake/Qt6", "cmake/Qt6"] {
        let qt6_dir = candidate.join(relative);
        if qt6_dir.join("Qt6Config.cmake").is_file() {
            let prefix = if relative == "cmake/Qt6" {
                qt_prefix_from_qt6_dir(&qt6_dir)?
            } else {
                candidate.to_path_buf()
            };
            return Some((prefix, qt6_dir));
        }
    }

    if candidate.join("Qt6Config.cmake").is_file() {
        let qt6_dir = candidate.to_path_buf();
        let prefix = qt_prefix_from_qt6_dir(&qt6_dir)?;
        return Some((prefix, qt6_dir));
    }

    None
}

fn qt_prefix_from_qt6_dir(qt6_dir: &Path) -> Option<PathBuf> {
    if !qt6_dir.join("Qt6Config.cmake").is_file() {
        return None;
    }

    let qt6_name = qt6_dir.file_name()?.to_string_lossy();
    if qt6_name != "Qt6" {
        return None;
    }

    let cmake_dir = qt6_dir.parent()?;
    if cmake_dir.file_name()?.to_string_lossy() != "cmake" {
        return None;
    }

    let prefix_candidate = cmake_dir.parent()?.to_path_buf();
    let prefix_name = prefix_candidate
        .file_name()
        .map(|value| value.to_string_lossy());
    if matches!(prefix_name.as_deref(), Some("lib" | "lib64")) {
        prefix_candidate.parent().map(Path::to_path_buf)
    } else {
        Some(prefix_candidate)
    }
}

fn detect_qt_version_roots(home_dir: Option<&Path>) -> Vec<PathBuf> {
    let mut roots = Vec::new();
    if let Ok(value) = env::var("QT_VERSION_ROOT") {
        let candidate = PathBuf::from(value.trim());
        if candidate.is_dir() {
            roots.push(candidate);
        }
    }

    if let Some(home) = home_dir {
        push_latest_or_self(&mut roots, &home.join("Qt"));
    }

    push_latest_or_self(&mut roots, &PathBuf::from("/opt/Qt"));
    push_latest_or_self(&mut roots, &PathBuf::from("/opt/qt"));

    roots
}

fn push_latest_or_self(targets: &mut Vec<PathBuf>, base_dir: &Path) {
    if !base_dir.is_dir() {
        return;
    }

    if let Some(latest) = latest_version_dir(base_dir) {
        if !targets.contains(&latest) {
            targets.push(latest);
        }
        return;
    }

    let value = base_dir.to_path_buf();
    if !targets.contains(&value) {
        targets.push(value);
    }
}

fn linux_qt_prefix_candidates(version_root: &Path) -> Vec<PathBuf> {
    let mut candidates = vec![
        version_root.join("gcc_64"),
        version_root.join("linux"),
        version_root.to_path_buf(),
    ];
    candidates.retain(|candidate| candidate.is_dir());
    candidates
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

    candidates.sort_by(|left, right| compare_version_paths(left, right));
    candidates.pop()
}

fn is_version_text(text: &str) -> bool {
    !text.is_empty() && text.chars().all(|ch| ch.is_ascii_digit() || ch == '.')
}

fn compare_version_paths(left: &Path, right: &Path) -> std::cmp::Ordering {
    let left_name = left
        .file_name()
        .map(|value| value.to_string_lossy().to_string())
        .unwrap_or_default();
    let right_name = right
        .file_name()
        .map(|value| value.to_string_lossy().to_string())
        .unwrap_or_default();
    compare_version_text(&left_name, &right_name)
}

fn compare_version_text(left: &str, right: &str) -> std::cmp::Ordering {
    let left_parts = parse_version_parts(left);
    let right_parts = parse_version_parts(right);
    let max_len = left_parts.len().max(right_parts.len());
    for index in 0..max_len {
        let left_value = *left_parts.get(index).unwrap_or(&0);
        let right_value = *right_parts.get(index).unwrap_or(&0);
        if left_value != right_value {
            return left_value.cmp(&right_value);
        }
    }
    std::cmp::Ordering::Equal
}

fn parse_version_parts(text: &str) -> Vec<u32> {
    text.split('.')
        .map(|part| part.parse::<u32>().unwrap_or(0))
        .collect()
}

fn run_linux_dependency_preflight(cmake_args: &[String]) -> Result<CommandCapture> {
    let preflight_root = env::temp_dir().join(format!(
        "lvrs-linux-preflight-{}-{}",
        std::process::id(),
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|value| value.as_nanos())
            .unwrap_or(0)
    ));
    let preflight_build_dir = preflight_root.join("build");
    fs::create_dir_all(&preflight_root).with_context(|| {
        format!(
            "failed to create Linux dependency preflight dir: {}",
            preflight_root.display()
        )
    })?;
    fs::write(
        preflight_root.join("CMakeLists.txt"),
        concat!(
            "cmake_minimum_required(VERSION 3.21)\n",
            "project(LVRSLinuxPreflight LANGUAGES CXX)\n",
            "find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Qml Svg Network)\n",
            "foreach(_lvrs_tool IN ITEMS moc rcc qmlimportscanner qmlcachegen qmltyperegistrar)\n",
            "    if(NOT TARGET Qt6::${_lvrs_tool})\n",
            "        message(FATAL_ERROR \"Qt host tool target missing: Qt6::${_lvrs_tool}\")\n",
            "    endif()\n",
            "endforeach()\n"
        ),
    )
    .context("failed to write Linux dependency preflight CMakeLists.txt")?;

    let mut args = vec![
        "-S".to_string(),
        preflight_root.display().to_string(),
        "-B".to_string(),
        preflight_build_dir.display().to_string(),
    ];
    args.extend(cmake_args.iter().cloned());

    let output = Command::new("cmake")
        .args(&args)
        .stdin(Stdio::null())
        .output()
        .context("failed to run Linux dependency preflight")?;
    let combined_output = format!(
        "{}{}",
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );

    let _ = remove_path(&preflight_root);

    Ok(CommandCapture {
        success: output.status.success(),
        output: combined_output,
    })
}

fn tail_lines(text: &str, max_lines: usize) -> String {
    let lines = text
        .lines()
        .filter(|line| !line.trim().is_empty())
        .collect::<Vec<_>>();
    let start = lines.len().saturating_sub(max_lines);
    lines[start..].join("\n")
}

pub(crate) fn detect_host_platform() -> &'static str {
    match env::consts::OS {
        "macos" => "macos",
        "linux" => "linux",
        "windows" => "windows",
        _ => "unknown",
    }
}

fn normalize_platform_list(input: String) -> String {
    input.replace(',', ";")
}

fn default_bootstrap_framework_platforms(host_platform: &str) -> &'static str {
    match host_platform {
        "linux" => "linux",
        "macos" => "macos;ios;android;wasm",
        "windows" => "windows;android;wasm",
        _ => "android;wasm",
    }
}

pub(crate) fn resolve_bootstrap_framework_platforms(
    cli_platforms: Option<&str>,
    host_platform: &str,
) -> String {
    if let Some(value) = cli_platforms {
        return normalize_platform_list(value.to_string());
    }

    if let Ok(value) = env::var(ENV_BOOTSTRAP_FRAMEWORK_PLATFORMS) {
        let trimmed = value.trim();
        if !trimmed.is_empty() {
            return normalize_platform_list(trimmed.to_string());
        }
    }

    default_bootstrap_framework_platforms(host_platform).to_string()
}

fn validate_deprecated_build_dir(build_dir: Option<&Path>, project_root: &Path) -> Result<()> {
    let Some(value) = build_dir else {
        return Ok(());
    };

    let accepted_repo_build = project_root.join("build");
    let accepted_relative = PathBuf::from("build");
    let accepted_dot_relative = PathBuf::from("./build");

    if value == accepted_relative || value == accepted_dot_relative || value == accepted_repo_build
    {
        return Ok(());
    }

    bail!(
        "[LVRS] --build-dir is deprecated. Use the fixed build directory: {}",
        accepted_repo_build.display()
    )
}

fn find_project_root(start: &Path) -> Option<PathBuf> {
    let mut current = Some(start);
    while let Some(path) = current {
        if has_sentinels(path) {
            return Some(path.to_path_buf());
        }
        current = path.parent();
    }
    None
}

fn has_sentinels(path: &Path) -> bool {
    path.join("CMakeLists.txt").exists()
        && path.join("qml").is_dir()
        && path.join("backend").is_dir()
}

pub(crate) fn resolve_project_root(
    project_root_override: Option<PathBuf>,
    install_prefix_hint: Option<&Path>,
) -> Result<PathBuf> {
    resolve_project_root_with_env_candidate(
        project_root_override,
        install_prefix_hint,
        resolve_root_from_env(),
    )
}

fn resolve_project_root_with_env_candidate(
    project_root_override: Option<PathBuf>,
    install_prefix_hint: Option<&Path>,
    env_root_candidate: Option<PathBuf>,
) -> Result<PathBuf> {
    if let Some(path) = project_root_override {
        return validate_project_root_candidate(path, "bootstrap override");
    }

    let env_root_error = if let Some(path) = env_root_candidate {
        match validate_project_root_candidate(path, "environment") {
            Ok(path) => return Ok(path),
            Err(error) => Some(error),
        }
    } else {
        None
    };

    let cwd = env::current_dir().context("failed to read current working directory")?;
    if let Some(path) = find_project_root(&cwd) {
        print_ignored_env_project_root_error(env_root_error.as_ref());
        return Ok(path);
    }

    if let Ok(executable) = env::current_exe() {
        if let Some(start) = executable.parent() {
            if let Some(path) = find_project_root(start) {
                print_ignored_env_project_root_error(env_root_error.as_ref());
                return Ok(path);
            }
        }
    }

    let manifest_hint = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("..");
    if let Some(path) = find_project_root(&manifest_hint) {
        print_ignored_env_project_root_error(env_root_error.as_ref());
        return Ok(path);
    }

    if let Some(path) = resolve_project_root_from_installed_source(install_prefix_hint)? {
        print_ignored_env_project_root_error(env_root_error.as_ref());
        return Ok(path);
    }

    if let Some(error) = env_root_error {
        eprintln!("[LVRS] Ignored invalid project root environment value: {error}");
    }
    bail!(
        "failed to locate LVRS repository root from {} (expected CMakeLists.txt, qml, backend). Set {} to repository root when launching outside the tree.",
        cwd.display(),
        ENV_LVRS_ROOT
    )
}

fn print_ignored_env_project_root_error(error: Option<&anyhow::Error>) {
    if let Some(error) = error {
        eprintln!("[LVRS] Ignoring invalid project root environment value: {error}");
    }
}

fn resolve_project_root_from_installed_source(
    install_prefix_hint: Option<&Path>,
) -> Result<Option<PathBuf>> {
    if let Some(prefix) = install_prefix_hint {
        if let Some(path) = resolve_project_root_from_install_prefix(prefix)? {
            return Ok(Some(path));
        }
    }

    let home_dir = match resolve_home_dir() {
        Ok(home_dir) => home_dir,
        Err(_) => return Ok(None),
    };
    let install_prefix = resolve_install_prefix(None, &home_dir)?;

    resolve_project_root_from_install_prefix(&install_prefix)
}

fn resolve_project_root_from_install_prefix(install_prefix: &Path) -> Result<Option<PathBuf>> {
    let source_install_dir = install_prefix.join("src").join("LVRS");
    let metadata_path = source_install_dir.join(INSTALL_SOURCE_INFO_FILE);

    if let Some(path) = parse_installed_source_project_root(&metadata_path)? {
        if let Ok(resolved) =
            validate_project_root_candidate_inner(path, "installed source metadata", false)
        {
            return Ok(Some(resolved));
        }
    }

    if has_sentinels(&source_install_dir) {
        return Ok(Some(source_install_dir));
    }

    Ok(None)
}

fn parse_installed_source_project_root(metadata_path: &Path) -> Result<Option<PathBuf>> {
    if !metadata_path.is_file() {
        return Ok(None);
    }

    let metadata = fs::read_to_string(metadata_path)
        .with_context(|| format!("failed to read {}", metadata_path.display()))?;

    for line in metadata.lines() {
        if let Some(value) = line.strip_prefix("project_root=") {
            let trimmed = value.trim();
            if !trimmed.is_empty() {
                return Ok(Some(PathBuf::from(trimmed)));
            }
        }
    }

    Ok(None)
}

fn resolve_root_from_env() -> Option<PathBuf> {
    for env_name in [ENV_LVRS_ROOT, ENV_LVRS_PROJECT_ROOT] {
        if let Ok(value) = env::var(env_name) {
            let trimmed = value.trim();
            if !trimmed.is_empty() {
                return Some(PathBuf::from(trimmed));
            }
        }
    }
    None
}

fn validate_project_root_candidate(candidate: PathBuf, source: &str) -> Result<PathBuf> {
    validate_project_root_candidate_inner(candidate, source, true)
}

fn validate_project_root_candidate_inner(
    candidate: PathBuf,
    source: &str,
    allow_install_prefix: bool,
) -> Result<PathBuf> {
    let normalized = if candidate.is_absolute() {
        candidate
    } else {
        env::current_dir()
            .context("failed to read current working directory")?
            .join(candidate)
    };

    if has_sentinels(&normalized) {
        return Ok(normalized);
    }

    if let Some(path) = find_project_root(&normalized) {
        return Ok(path);
    }

    if allow_install_prefix {
        if let Some(path) = resolve_project_root_from_install_prefix(&normalized)? {
            return Ok(path);
        }
    }

    if let Some(path) = relocate_project_root_candidate(&normalized) {
        return Ok(path);
    }

    bail!(
        "invalid LVRS project root from {}: {} (expected CMakeLists.txt, qml, backend)",
        source,
        normalized.display()
    )
}

fn relocate_project_root_candidate(stale_path: &Path) -> Option<PathBuf> {
    let anchor = deepest_existing_ancestor(stale_path)?;
    if !anchor.is_dir() || anchor == Path::new("/") {
        return None;
    }

    let stale_components = normal_path_components(stale_path);
    if stale_components.is_empty() {
        return None;
    }

    let anchor_depth = normal_path_components(&anchor).len();
    let remaining_depth = stale_components.len().saturating_sub(anchor_depth);
    let max_depth = remaining_depth.saturating_add(2).min(6);
    let mut best_match: Option<(usize, usize, PathBuf)> = None;

    walk_project_root_candidates(&anchor, 0, max_depth, &mut |candidate| {
        if !has_sentinels(candidate) {
            return;
        }

        let shared_tail = shared_tail_component_count(candidate, stale_path);
        if shared_tail == 0 {
            return;
        }

        let depth_gap = normal_path_components(candidate)
            .len()
            .abs_diff(stale_components.len());
        let should_replace = match &best_match {
            Some((best_tail, best_gap, best_path)) => {
                shared_tail > *best_tail
                    || (shared_tail == *best_tail
                        && (depth_gap < *best_gap
                            || (depth_gap == *best_gap && candidate < best_path.as_path())))
            }
            None => true,
        };

        if should_replace {
            best_match = Some((shared_tail, depth_gap, candidate.to_path_buf()));
        }
    });

    match best_match {
        Some((shared_tail, _, path)) if shared_tail >= 2 => Some(path),
        _ => None,
    }
}

fn deepest_existing_ancestor(path: &Path) -> Option<PathBuf> {
    let mut current = Some(path);
    while let Some(candidate) = current {
        if candidate.exists() {
            return Some(candidate.to_path_buf());
        }
        current = candidate.parent();
    }
    None
}

fn walk_project_root_candidates(
    root: &Path,
    depth: usize,
    max_depth: usize,
    visit: &mut impl FnMut(&Path),
) {
    visit(root);

    if depth >= max_depth {
        return;
    }

    let entries = match fs::read_dir(root) {
        Ok(entries) => entries,
        Err(_) => return,
    };
    let mut directories = Vec::new();

    for entry in entries.flatten() {
        let Ok(file_type) = entry.file_type() else {
            continue;
        };
        if !file_type.is_dir() || file_type.is_symlink() {
            continue;
        }
        directories.push(entry.path());
    }

    directories.sort();
    for path in directories {
        walk_project_root_candidates(&path, depth + 1, max_depth, visit);
    }
}

fn normal_path_components(path: &Path) -> Vec<OsString> {
    path.components()
        .filter_map(|component| match component {
            Component::Normal(value) => Some(value.to_os_string()),
            _ => None,
        })
        .collect()
}

fn shared_tail_component_count(lhs: &Path, rhs: &Path) -> usize {
    let lhs_components = normal_path_components(lhs);
    let rhs_components = normal_path_components(rhs);

    lhs_components
        .iter()
        .rev()
        .zip(rhs_components.iter().rev())
        .take_while(|(lhs, rhs)| lhs == rhs)
        .count()
}

fn paths_refer_to_same_location(lhs: &Path, rhs: &Path) -> bool {
    match (fs::canonicalize(lhs), fs::canonicalize(rhs)) {
        (Ok(left), Ok(right)) => left == right,
        _ => lhs == rhs,
    }
}

fn ensure_cmake_available() -> Result<()> {
    let status = Command::new("cmake")
        .arg("--version")
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status()
        .context("failed to execute cmake --version")?;
    if status.success() {
        Ok(())
    } else {
        bail!("cmake is required but not found in PATH.")
    }
}

fn run_command<S: AsRef<OsStr>>(program: &str, args: &[S]) -> bool {
    let status = Command::new(program)
        .args(args)
        .stdin(Stdio::inherit())
        .stdout(Stdio::inherit())
        .stderr(Stdio::inherit())
        .status();
    matches!(status, Ok(s) if s.success())
}

fn clean_recreate_dir(target_dir: &Path, target_name: &str) -> Result<()> {
    if !target_dir.exists() {
        fs::create_dir_all(target_dir).with_context(|| {
            format!(
                "failed to create {} directory: {}",
                target_name,
                target_dir.display()
            )
        })?;
        return Ok(());
    }

    remove_path_with_stale_fallback(target_dir, target_name)?;
    fs::create_dir_all(target_dir).with_context(|| {
        format!(
            "failed to recreate {} directory: {}",
            target_name,
            target_dir.display()
        )
    })?;
    Ok(())
}

fn remove_path_with_stale_fallback(target_path: &Path, target_name: &str) -> Result<()> {
    if !target_path.exists() {
        return Ok(());
    }

    if remove_path(target_path).is_ok() {
        return Ok(());
    }

    let parent_dir = target_path
        .parent()
        .map(Path::to_path_buf)
        .unwrap_or_else(|| PathBuf::from("."));
    let base_name = target_path
        .file_name()
        .map(|s| s.to_string_lossy().to_string())
        .unwrap_or_else(|| "lvrs".to_string());
    let stale_path = parent_dir.join(format!(".{}.lvrs-stale-{}", base_name, std::process::id()));

    let _ = remove_path(&stale_path);

    fs::rename(target_path, &stale_path).with_context(|| {
        format!(
            "[LVRS] Failed to relocate {}: {}",
            target_name,
            target_path.display()
        )
    })?;

    if let Err(error) = remove_path(&stale_path) {
        eprintln!(
            "[LVRS] Warning: stale {} remains: {}",
            target_name,
            stale_path.display()
        );
        eprintln!(
            "[LVRS] Fresh install can continue; remove stale path manually if needed. ({error})"
        );
    }

    Ok(())
}

fn remove_path(path: &Path) -> Result<()> {
    if !path.exists() {
        return Ok(());
    }

    let metadata = fs::symlink_metadata(path)
        .with_context(|| format!("failed to inspect path for removal: {}", path.display()))?;
    let file_type = metadata.file_type();

    if file_type.is_dir() && !file_type.is_symlink() {
        fs::remove_dir_all(path)
            .with_context(|| format!("failed to remove directory: {}", path.display()))?;
    } else {
        fs::remove_file(path)
            .with_context(|| format!("failed to remove file: {}", path.display()))?;
    }
    Ok(())
}

fn build_host_examples(build_dir: &Path, build_type: &str) -> Result<()> {
    println!("[LVRS] Building host examples for source snapshot...");
    let build_status = run_command(
        "cmake",
        &[
            "--build".to_string(),
            build_dir.display().to_string(),
            "--config".to_string(),
            build_type.to_string(),
            "--target".to_string(),
            "lvrs_host_examples_all".to_string(),
        ],
    );
    if build_status {
        return Ok(());
    }

    bail!("host example build step failed")
}

fn install_source_snapshot(
    project_root: &Path,
    source_install_dir: &Path,
    build_dir: &Path,
    include_example_bins: bool,
) -> Result<()> {
    if paths_refer_to_same_location(project_root, source_install_dir) {
        println!("[LVRS] Reusing installed source snapshot in place...");
        prune_source_snapshot_example_bin_dirs(source_install_dir)?;
        if include_example_bins {
            install_built_example_bins(build_dir, source_install_dir)?;
        }
        write_source_snapshot_metadata(project_root, source_install_dir)?;
        return Ok(());
    }

    println!("[LVRS] Installing source snapshot...");
    let _ = remove_path(source_install_dir);
    fs::create_dir_all(source_install_dir).with_context(|| {
        format!(
            "failed to create source snapshot directory: {}",
            source_install_dir.display()
        )
    })?;

    for entry in fs::read_dir(project_root)
        .with_context(|| format!("failed to read project root: {}", project_root.display()))?
    {
        let entry = entry?;
        let path = entry.path();
        let name = entry.file_name().to_string_lossy().to_string();
        if should_skip_snapshot_entry(&name) {
            continue;
        }

        if path.is_dir() && name == "rust-cli" {
            copy_rust_cli_snapshot(&path, &source_install_dir.join(&name))?;
        } else if path.is_dir() {
            run_cmake_copy_directory(&path, &source_install_dir.join(&name))?;
        } else {
            run_cmake_copy_file(&path, source_install_dir)?;
        }
    }

    prune_source_snapshot_example_bin_dirs(source_install_dir)?;
    if include_example_bins {
        install_built_example_bins(build_dir, source_install_dir)?;
    }

    write_source_snapshot_metadata(project_root, source_install_dir)?;
    Ok(())
}

fn write_source_snapshot_metadata(project_root: &Path, source_install_dir: &Path) -> Result<()> {
    let source_revision =
        detect_git_revision(project_root).unwrap_or_else(|| "unknown".to_string());
    let installed_at = detect_install_time();
    let info = format!(
        "LVRS source snapshot\nproject_root={}\nsource_revision={}\ninstalled_at={}\n",
        project_root.display(),
        source_revision,
        installed_at
    );
    fs::write(source_install_dir.join(INSTALL_SOURCE_INFO_FILE), info)
        .context("failed to write source snapshot metadata")?;
    Ok(())
}

fn prune_source_snapshot_example_bin_dirs(source_install_dir: &Path) -> Result<()> {
    let snapshot_example_root = source_install_dir.join("example");
    if !snapshot_example_root.is_dir() {
        return Ok(());
    }

    for entry in fs::read_dir(&snapshot_example_root).with_context(|| {
        format!(
            "failed to read source snapshot example dir: {}",
            snapshot_example_root.display()
        )
    })? {
        let entry = entry?;
        let example_dir = entry.path();
        if !example_dir.is_dir() {
            continue;
        }

        let bin_dir = example_dir.join("bin");
        if !bin_dir.is_dir() {
            continue;
        }

        for bin_entry in fs::read_dir(&bin_dir)
            .with_context(|| format!("failed to read snapshot bin dir: {}", bin_dir.display()))?
        {
            let bin_entry = bin_entry?;
            let bin_path = bin_entry.path();
            if is_example_launcher_script(&bin_path)? {
                continue;
            }
            remove_path(&bin_path)?;
        }
    }

    Ok(())
}

fn install_built_example_bins(build_dir: &Path, source_install_dir: &Path) -> Result<()> {
    let built_examples_root = build_dir.join("example");
    if !built_examples_root.is_dir() {
        bail!(
            "host example build output directory not found: {}",
            built_examples_root.display()
        );
    }

    let snapshot_examples_root = source_install_dir.join("example");
    let mut copied_any = false;

    for entry in fs::read_dir(&built_examples_root).with_context(|| {
        format!(
            "failed to read built example dir: {}",
            built_examples_root.display()
        )
    })? {
        let entry = entry?;
        let example_build_dir = entry.path();
        if !example_build_dir.is_dir() {
            continue;
        }

        let built_bin_dir = example_build_dir.join("bin");
        if !built_bin_dir.is_dir() {
            continue;
        }

        let example_name = entry.file_name();
        let snapshot_example_dir = snapshot_examples_root.join(&example_name);
        if !snapshot_example_dir.is_dir() {
            continue;
        }

        let snapshot_bin_dir = snapshot_example_dir.join("bin");
        fs::create_dir_all(&snapshot_bin_dir).with_context(|| {
            format!(
                "failed to create snapshot example bin dir: {}",
                snapshot_bin_dir.display()
            )
        })?;

        for built_entry in fs::read_dir(&built_bin_dir).with_context(|| {
            format!(
                "failed to read built example bin dir: {}",
                built_bin_dir.display()
            )
        })? {
            let built_entry = built_entry?;
            let built_path = built_entry.path();
            let built_name = built_entry.file_name();
            let default_snapshot_path = snapshot_bin_dir.join(&built_name);

            if built_path.is_dir() {
                remove_path(&default_snapshot_path)?;
                run_cmake_copy_directory(&built_path, &default_snapshot_path)?;
                copied_any = true;
                continue;
            }

            let snapshot_path = if is_example_launcher_script(&default_snapshot_path)? {
                let renamed = format!("{}.real", built_name.to_string_lossy());
                snapshot_bin_dir.join(renamed)
            } else {
                default_snapshot_path
            };

            copy_file_preserving_permissions(&built_path, &snapshot_path)?;
            copied_any = true;
        }
    }

    if copied_any {
        return Ok(());
    }

    bail!(
        "no host example binaries were produced under {}",
        built_examples_root.display()
    )
}

fn should_skip_snapshot_entry(name: &str) -> bool {
    name == ".git"
        || name == ".idea"
        || name == ".vscode"
        || name == "build"
        || name == ".build"
        || name.starts_with(".build-")
        || name.starts_with(".build.")
        || name.starts_with("build-")
        || name.starts_with("cmake-build-")
}

fn copy_rust_cli_snapshot(source: &Path, destination: &Path) -> Result<()> {
    fs::create_dir_all(destination).with_context(|| {
        format!(
            "failed to create Rust CLI snapshot directory: {}",
            destination.display()
        )
    })?;

    for entry in fs::read_dir(source)
        .with_context(|| format!("failed to read Rust CLI source: {}", source.display()))?
    {
        let entry = entry?;
        let path = entry.path();
        let name = entry.file_name().to_string_lossy().to_string();
        if name == "target" || should_skip_snapshot_entry(&name) {
            continue;
        }

        if path.is_dir() {
            run_cmake_copy_directory(&path, &destination.join(&name))?;
        } else {
            run_cmake_copy_file(&path, destination)?;
        }
    }

    Ok(())
}

fn run_cmake_copy_directory(source: &Path, destination: &Path) -> Result<()> {
    let args = vec![
        "-E".to_string(),
        "copy_directory".to_string(),
        source.display().to_string(),
        destination.display().to_string(),
    ];
    if run_command("cmake", &args) {
        Ok(())
    } else {
        bail!(
            "cmake -E copy_directory failed: {} -> {}",
            source.display(),
            destination.display()
        )
    }
}

fn run_cmake_copy_file(source: &Path, destination_dir: &Path) -> Result<()> {
    let args = vec![
        "-E".to_string(),
        "copy".to_string(),
        source.display().to_string(),
        destination_dir.display().to_string(),
    ];
    if run_command("cmake", &args) {
        Ok(())
    } else {
        bail!(
            "cmake -E copy failed: {} -> {}",
            source.display(),
            destination_dir.display()
        )
    }
}

fn is_example_launcher_script(path: &Path) -> Result<bool> {
    if !path.is_file() {
        return Ok(false);
    }

    let content = fs::read(path)
        .with_context(|| format!("failed to read launcher candidate: {}", path.display()))?;
    Ok(content.starts_with(b"#!/bin/sh\n"))
}

fn copy_file_preserving_permissions(source: &Path, destination: &Path) -> Result<()> {
    if let Some(parent) = destination.parent() {
        fs::create_dir_all(parent).with_context(|| {
            format!(
                "failed to create destination parent directory: {}",
                parent.display()
            )
        })?;
    }

    if destination.exists() {
        remove_path(destination)?;
    }

    fs::copy(source, destination).with_context(|| {
        format!(
            "failed to copy file: {} -> {}",
            source.display(),
            destination.display()
        )
    })?;

    let permissions = fs::metadata(source)
        .with_context(|| format!("failed to read source metadata: {}", source.display()))?
        .permissions();
    fs::set_permissions(destination, permissions).with_context(|| {
        format!(
            "failed to apply source permissions: {}",
            destination.display()
        )
    })?;
    Ok(())
}

fn detect_git_revision(project_root: &Path) -> Option<String> {
    let status = Command::new("git")
        .arg("-C")
        .arg(project_root)
        .args(["rev-parse", "--is-inside-work-tree"])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status()
        .ok()?;
    if !status.success() {
        return None;
    }

    let output = Command::new("git")
        .arg("-C")
        .arg(project_root)
        .args(["rev-parse", "HEAD"])
        .output()
        .ok()?;
    if !output.status.success() {
        return None;
    }
    let value = String::from_utf8_lossy(&output.stdout).trim().to_string();
    if value.is_empty() { None } else { Some(value) }
}

fn detect_install_time() -> String {
    let output = Command::new("date").arg("+%Y-%m-%d %H:%M:%S %z").output();
    if let Ok(value) = output {
        if value.status.success() {
            let trimmed = String::from_utf8_lossy(&value.stdout).trim().to_string();
            if !trimmed.is_empty() {
                return trimmed;
            }
        }
    }

    let epoch = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);
    epoch.to_string()
}

fn register_cmake_package(
    home_dir: &Path,
    install_prefix: &Path,
    package_config_dir: &Path,
) -> Result<()> {
    let cmake_user_package_dir = if let Ok(appdata) = env::var("APPDATA") {
        PathBuf::from(appdata)
            .join("CMake")
            .join("packages")
            .join("LVRS")
    } else {
        home_dir.join(".cmake").join("packages").join("LVRS")
    };

    fs::create_dir_all(&cmake_user_package_dir).with_context(|| {
        format!(
            "failed to create CMake user package dir: {}",
            cmake_user_package_dir.display()
        )
    })?;

    if cmake_user_package_dir.is_dir() {
        for entry in fs::read_dir(&cmake_user_package_dir).with_context(|| {
            format!(
                "failed to read CMake user package dir: {}",
                cmake_user_package_dir.display()
            )
        })? {
            let entry = entry?;
            let path = entry.path();
            if !path.is_file() {
                continue;
            }
            let mut content = String::new();
            if fs::File::open(&path)
                .and_then(|mut file| file.read_to_string(&mut content))
                .is_ok()
                && content.contains(&install_prefix.display().to_string())
            {
                let _ = fs::remove_file(&path);
            }
        }
    }

    if package_config_dir.is_dir() {
        let epoch = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_secs())
            .unwrap_or(0);
        let registry_entry =
            cmake_user_package_dir.join(format!("{}-{}", epoch, std::process::id()));
        fs::write(
            &registry_entry,
            format!("{}\n", package_config_dir.display()),
        )
        .with_context(|| {
            format!(
                "failed to write registry entry: {}",
                registry_entry.display()
            )
        })?;
        println!(
            "[LVRS] Registered CMake package: {}",
            registry_entry.display()
        );
    } else {
        println!(
            "[LVRS] Registry skip: host package dir not found -> {}",
            package_config_dir.display()
        );
    }

    Ok(())
}

fn install_cli_binary(source: &Path, install_prefix: &Path) -> Result<PathBuf> {
    let binary_dir = install_prefix.join("bin");
    let binary_path = binary_dir.join(format!("lvrs{}", env::consts::EXE_SUFFIX));
    fs::create_dir_all(&binary_dir).with_context(|| {
        format!(
            "failed to create CLI install directory: {}",
            binary_dir.display()
        )
    })?;
    if !paths_refer_to_same_location(source, &binary_path) {
        fs::copy(source, &binary_path)
            .with_context(|| format!("failed to install LVRS CLI: {}", binary_path.display()))?;
    }
    set_executable_bit(&binary_path)?;
    Ok(binary_path)
}

fn write_env_helper(
    env_file: &Path,
    platform_install_root: &Path,
    host_platform: &str,
    host_install_prefix: &Path,
    install_prefix: &Path,
) -> Result<()> {
    if let Some(parent) = env_file.parent() {
        fs::create_dir_all(parent)
            .with_context(|| format!("failed to create env helper dir: {}", parent.display()))?;
    }
    let content = render_env_helper_script(
        platform_install_root.display(),
        host_platform,
        host_install_prefix.display(),
        install_prefix.display(),
        host_install_prefix.display(),
    );
    fs::write(env_file, content)
        .with_context(|| format!("failed to write env helper: {}", env_file.display()))?;

    set_executable_bit(env_file)?;
    Ok(())
}

fn render_env_helper_script(
    platform_install_root: impl std::fmt::Display,
    host_platform: &str,
    host_install_prefix: impl std::fmt::Display,
    install_prefix: impl std::fmt::Display,
    qml_import_root: impl std::fmt::Display,
) -> String {
    let mut content = format!(
        "#!/usr/bin/env sh\n# LVRS environment helper\nexport LVRS_PLATFORMS_ROOT=\"{}\"\nexport LVRS_HOST_PLATFORM=\"{}\"\nexport LVRS_HOST_PREFIX=\"{}\"\n",
        platform_install_root, host_platform, host_install_prefix
    );
    content.push_str(
        r#"_lvrs_prepend_path() {
    _lvrs_var_name="$1"
    _lvrs_path_value="$2"
    eval "_lvrs_current_value=\${${_lvrs_var_name}:-}"
    case ":${_lvrs_current_value}:" in
        *:"${_lvrs_path_value}":*) return ;;
    esac
    if [ -n "${_lvrs_current_value}" ]; then
        eval "export ${_lvrs_var_name}=\"${_lvrs_path_value}:${_lvrs_current_value}\""
    else
        eval "export ${_lvrs_var_name}=\"${_lvrs_path_value}\""
    fi
}
"#,
    );
    content.push_str(&format!(
        "_lvrs_prepend_path PATH \"{}/bin\"\n_lvrs_prepend_path CMAKE_PREFIX_PATH \"{}\"\n_lvrs_prepend_path QML2_IMPORT_PATH \"{}/lib/qt6/qml\"\nunset -f _lvrs_prepend_path\n",
        install_prefix, install_prefix, qml_import_root
    ));
    content
}

#[cfg(unix)]
fn set_executable_bit(path: &Path) -> Result<()> {
    use std::os::unix::fs::PermissionsExt;
    let mut permissions = fs::metadata(path)
        .with_context(|| format!("failed to read metadata: {}", path.display()))?
        .permissions();
    permissions.set_mode(0o755);
    fs::set_permissions(path, permissions)
        .with_context(|| format!("failed to set executable bit: {}", path.display()))?;
    Ok(())
}

#[cfg(not(unix))]
fn set_executable_bit(_path: &Path) -> Result<()> {
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::env;
    use std::path::{Path, PathBuf};
    use std::sync::Mutex;
    use std::time::{SystemTime, UNIX_EPOCH};

    static CURRENT_DIR_TEST_LOCK: Mutex<()> = Mutex::new(());

    #[test]
    fn cli_install_copies_binary_and_supports_running_from_install_prefix() -> Result<()> {
        let root = temp_test_dir("cli-install");
        fs::create_dir_all(&root)?;
        let source = root.join("cli-source");
        fs::write(&source, "test CLI payload")?;
        let prefix = root.join("prefix");
        let installed = install_cli_binary(&source, &prefix)?;
        assert_eq!(fs::read_to_string(&installed)?, "test CLI payload");
        assert_eq!(install_cli_binary(&installed, &prefix)?, installed);
        assert_eq!(fs::read_to_string(&installed)?, "test CLI payload");
        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn install_prefix_uses_sdk_default_and_keeps_overrides() -> Result<()> {
        let home = env::current_dir()?.join("test-home");
        assert_eq!(
            resolve_install_prefix_with_env(None, None, &home)?,
            home.join(".local").join("SDK").join("LVRS")
        );
        assert_eq!(
            resolve_install_prefix_with_env(None, Some("~/custom-env".into()), &home)?,
            home.join("custom-env")
        );
        assert_eq!(
            resolve_install_prefix_with_env(
                Some(PathBuf::from("~/custom-cli")),
                Some("~/custom-env".into()),
                &home,
            )?,
            home.join("custom-cli")
        );
        Ok(())
    }

    struct CurrentDirGuard {
        original: PathBuf,
    }

    impl CurrentDirGuard {
        fn enter(path: &Path) -> Result<Self> {
            let original = env::current_dir()?;
            env::set_current_dir(path)?;
            Ok(Self { original })
        }
    }

    impl Drop for CurrentDirGuard {
        fn drop(&mut self) {
            let _ = env::set_current_dir(&self.original);
        }
    }

    fn temp_test_dir(label: &str) -> PathBuf {
        let nanos = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|value| value.as_nanos())
            .unwrap_or(0);
        env::temp_dir().join(format!(
            "lvrs-install-tests-{label}-{}-{nanos}",
            std::process::id()
        ))
    }

    fn create_project_root(path: &Path) -> Result<()> {
        fs::create_dir_all(path.join("qml"))?;
        fs::create_dir_all(path.join("backend"))?;
        fs::write(
            path.join("CMakeLists.txt"),
            "cmake_minimum_required(VERSION 3.21)\n",
        )?;
        Ok(())
    }

    fn write_launcher_script(path: &Path) -> Result<()> {
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent)?;
        }
        fs::write(path, "#!/bin/sh\nexit 0\n")?;
        Ok(())
    }

    #[test]
    fn prune_source_snapshot_example_bin_dirs_preserves_launchers() -> Result<()> {
        let root = temp_test_dir("prune");
        let bin_dir = root.join("example").join("VisualCatalog").join("bin");
        fs::create_dir_all(&bin_dir)?;
        write_launcher_script(&bin_dir.join("LVRSExampleVisualCatalog"))?;
        fs::write(bin_dir.join("stale-app"), "stale")?;
        fs::write(
            bin_dir.join("LVRSExampleVisualCatalog.real"),
            "stale-runtime",
        )?;

        prune_source_snapshot_example_bin_dirs(&root)?;

        assert!(bin_dir.join("LVRSExampleVisualCatalog").exists());
        assert!(!bin_dir.join("stale-app").exists());
        assert!(!bin_dir.join("LVRSExampleVisualCatalog.real").exists());
        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn install_built_example_bins_copies_fresh_build_outputs() -> Result<()> {
        let root = temp_test_dir("copy");
        let build_bin_dir = root
            .join("build")
            .join("example")
            .join("VisualCatalog")
            .join("bin");
        let snapshot_example_dir = root.join("snapshot").join("example").join("VisualCatalog");
        let stale_bin_dir = snapshot_example_dir.join("bin");

        fs::create_dir_all(&build_bin_dir)?;
        fs::create_dir_all(&stale_bin_dir)?;
        write_launcher_script(&stale_bin_dir.join("LVRSExampleVisualCatalog"))?;
        fs::write(stale_bin_dir.join("old-app"), "stale")?;
        fs::write(
            build_bin_dir.join("LVRSExampleVisualCatalog"),
            "fresh-build-output",
        )?;

        prune_source_snapshot_example_bin_dirs(&root.join("snapshot"))?;
        install_built_example_bins(&root.join("build"), &root.join("snapshot"))?;

        let copied_binary = snapshot_example_dir
            .join("bin")
            .join("LVRSExampleVisualCatalog.real");
        assert!(copied_binary.exists());
        assert_eq!(fs::read_to_string(copied_binary)?, "fresh-build-output");
        assert_eq!(
            fs::read_to_string(
                snapshot_example_dir
                    .join("bin")
                    .join("LVRSExampleVisualCatalog")
            )?,
            "#!/bin/sh\nexit 0\n"
        );
        assert!(!snapshot_example_dir.join("bin").join("old-app").exists());

        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn validate_project_root_candidate_recovers_after_parent_directory_rename() -> Result<()> {
        let root = temp_test_dir("relocate");
        let storage_root = root.join("Storage");
        let relocated_root = storage_root
            .join("Workspace")
            .join("InfraSystem")
            .join("LVRS");
        let stale_root = storage_root.join("static").join("InfraSystem").join("LVRS");

        create_project_root(&relocated_root)?;

        let resolved = validate_project_root_candidate(stale_root, "test")?;
        assert_eq!(resolved, relocated_root);

        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn validate_project_root_candidate_accepts_installed_prefix_from_environment() -> Result<()> {
        let root = temp_test_dir("env-prefix");
        let checkout = root.join("Workspace").join("LVRS");
        let install_prefix = root.join("prefix");
        let snapshot = install_prefix.join("src").join("LVRS");

        create_project_root(&checkout)?;
        create_project_root(&snapshot)?;
        fs::write(
            snapshot.join(INSTALL_SOURCE_INFO_FILE),
            format!(
                "LVRS source snapshot\nproject_root={}\n",
                checkout.display()
            ),
        )?;

        let resolved = validate_project_root_candidate(install_prefix, "environment")?;
        assert_eq!(resolved, checkout);

        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn resolve_project_root_falls_back_to_cwd_when_env_prefix_snapshot_is_missing() -> Result<()> {
        let _guard = CURRENT_DIR_TEST_LOCK.lock().expect("current dir test lock");
        let root = temp_test_dir("env-prefix-missing-snapshot");
        let checkout = root.join("Workspace").join("LVRS");
        let install_prefix = root.join("prefix");

        create_project_root(&checkout)?;
        fs::create_dir_all(&install_prefix)?;

        {
            let _cwd = CurrentDirGuard::enter(&checkout)?;
            let resolved =
                resolve_project_root_with_env_candidate(None, None, Some(install_prefix.clone()))?;
            assert!(paths_refer_to_same_location(&resolved, &checkout));
        }

        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn install_source_snapshot_keeps_in_place_snapshot_contents() -> Result<()> {
        let root = temp_test_dir("in-place-snapshot");
        create_project_root(&root)?;
        fs::write(root.join("README.md"), "keep")?;

        let stale_bin = root.join("example").join("VisualCatalog").join("bin");
        let build_bin = root
            .join("build")
            .join("example")
            .join("VisualCatalog")
            .join("bin");
        fs::create_dir_all(&stale_bin)?;
        fs::create_dir_all(&build_bin)?;
        write_launcher_script(&stale_bin.join("LVRSExampleVisualCatalog"))?;
        fs::write(stale_bin.join("stale-app"), "stale")?;
        fs::write(build_bin.join("LVRSExampleVisualCatalog"), "fresh")?;

        install_source_snapshot(&root, &root, &root.join("build"), true)?;

        assert_eq!(fs::read_to_string(root.join("README.md"))?, "keep");
        assert!(!stale_bin.join("stale-app").exists());
        assert_eq!(
            fs::read_to_string(stale_bin.join("LVRSExampleVisualCatalog"))?,
            "#!/bin/sh\nexit 0\n"
        );
        assert_eq!(
            fs::read_to_string(stale_bin.join("LVRSExampleVisualCatalog.real"))?,
            "fresh"
        );
        assert!(root.join(INSTALL_SOURCE_INFO_FILE).is_file());

        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn install_source_snapshot_excludes_nested_build_artifacts() -> Result<()> {
        let root = temp_test_dir("snapshot-build-artifacts");
        let checkout = root.join("checkout");
        let snapshot = root.join("snapshot");
        create_project_root(&checkout)?;

        let rust_source = checkout.join("rust-cli").join("src");
        let rust_target = checkout.join("rust-cli").join("target").join("debug");
        let rust_build = checkout.join("rust-cli").join("build").join("debug");
        let stale_build = checkout.join(".build.lvrs-stale-test");
        fs::create_dir_all(&rust_source)?;
        fs::create_dir_all(&rust_target)?;
        fs::create_dir_all(&rust_build)?;
        fs::create_dir_all(&stale_build)?;
        fs::write(rust_source.join("main.rs"), "fn main() {}\n")?;
        fs::write(rust_target.join("stale-build-output"), "stale")?;
        fs::write(rust_build.join("stale-build-output"), "stale")?;
        fs::write(stale_build.join("CMakeCache.txt"), "stale")?;

        install_source_snapshot(&checkout, &snapshot, &root.join("build"), false)?;

        assert!(snapshot.join("rust-cli/src/main.rs").is_file());
        assert!(!snapshot.join("rust-cli/target").exists());
        assert!(!snapshot.join("rust-cli/build").exists());
        assert!(!snapshot.join(".build.lvrs-stale-test").exists());

        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn default_bootstrap_framework_platforms_follow_host_policy() {
        assert_eq!(default_bootstrap_framework_platforms("linux"), "linux");
        assert_eq!(
            default_bootstrap_framework_platforms("macos"),
            "macos;ios;android;wasm"
        );
        assert_eq!(
            default_bootstrap_framework_platforms("windows"),
            "windows;android;wasm"
        );
    }

    #[test]
    fn resolve_bootstrap_framework_platforms_prefers_host_defaults() {
        assert_eq!(
            resolve_bootstrap_framework_platforms(None, "linux"),
            "linux"
        );
    }

    #[test]
    fn resolve_qt_install_candidate_accepts_standard_prefix_layout() -> Result<()> {
        let root = temp_test_dir("qt-standard");
        let qt_prefix = root.join("Qt").join("6.8.3").join("gcc_64");
        let qt6_dir = qt_prefix.join("lib").join("cmake").join("Qt6");
        fs::create_dir_all(&qt6_dir)?;
        fs::write(qt6_dir.join("Qt6Config.cmake"), "")?;

        let resolved = resolve_qt_install_candidate(&qt_prefix);
        assert!(resolved.is_some());
        let (resolved_prefix, resolved_qt6_dir) = resolved.unwrap();
        assert_eq!(resolved_prefix, qt_prefix);
        assert_eq!(resolved_qt6_dir, qt6_dir);

        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn resolve_qt_install_candidate_accepts_multiarch_layout() -> Result<()> {
        let root = temp_test_dir("qt-multiarch");
        let qt_prefix = root.join("usr").join("lib").join("x86_64-linux-gnu");
        let qt6_dir = qt_prefix.join("cmake").join("Qt6");
        fs::create_dir_all(&qt6_dir)?;
        fs::write(qt6_dir.join("Qt6Config.cmake"), "")?;

        let resolved = resolve_qt_install_candidate(&qt_prefix);
        assert!(resolved.is_some());
        let (resolved_prefix, resolved_qt6_dir) = resolved.unwrap();
        assert_eq!(resolved_prefix, qt_prefix);
        assert_eq!(resolved_qt6_dir, qt6_dir);

        let resolved_from_qt6_dir = resolve_qt_install_candidate(&qt6_dir).unwrap();
        assert_eq!(resolved_from_qt6_dir.0, qt_prefix);
        assert_eq!(resolved_from_qt6_dir.1, qt6_dir);

        remove_path(&root)?;
        Ok(())
    }

    #[test]
    fn render_env_helper_script_uses_idempotent_path_prepend() {
        let script = render_env_helper_script(
            "/tmp/lvrs/platforms",
            "linux",
            "/tmp/lvrs/platforms/linux",
            "/tmp/lvrs",
            "/tmp/lvrs/platforms/linux",
        );
        assert!(script.contains("export LVRS_PLATFORMS_ROOT=\"/tmp/lvrs/platforms\""));
        assert!(script.contains("_lvrs_prepend_path PATH \"/tmp/lvrs/bin\""));
        assert!(script.contains("_lvrs_prepend_path CMAKE_PREFIX_PATH \"/tmp/lvrs\""));
        assert!(script.contains(
            "_lvrs_prepend_path QML2_IMPORT_PATH \"/tmp/lvrs/platforms/linux/lib/qt6/qml\""
        ));
    }

    #[test]
    fn linux_package_manager_label_matches_command_family() {
        assert_eq!(linux_package_manager_label(LinuxPackageManager::Apt), "apt");
        assert_eq!(linux_package_manager_label(LinuxPackageManager::Dnf), "dnf");
        assert_eq!(
            linux_package_manager_label(LinuxPackageManager::Pacman),
            "pacman"
        );
    }

    #[test]
    fn build_linux_package_install_plan_for_apt_contains_qt_packages() {
        let plan = build_linux_package_install_plan(LinuxPackageManager::Apt);
        let joined = plan.display_commands.join("\n");
        assert!(joined.contains("apt-get install -y"));
        assert!(joined.contains("qt6-base-dev"));
        assert!(joined.contains("qt6-declarative-dev"));
        assert!(joined.contains("qt6-svg-dev"));
    }
}
