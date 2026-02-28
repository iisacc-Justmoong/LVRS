use crate::cli::InstallArgs;
use anyhow::{Context, Result, bail};
use std::env;
use std::ffi::OsStr;
use std::fs;
use std::io::Read;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::time::{SystemTime, UNIX_EPOCH};

const DEFAULT_BOOTSTRAP_FRAMEWORK_PLATFORMS: &str = "macos;linux;windows;ios;android;wasm";

pub fn run(args: InstallArgs, _verbose: u8) -> Result<()> {
    if args.force_x86_qt_tools {
        bail!("[LVRS] --force-x86-qt-tools is unsupported. Apple x86 paths are disabled.");
    }

    let cwd = env::current_dir().context("failed to read current working directory")?;
    let project_root = find_project_root(&cwd).with_context(|| {
        format!(
            "failed to locate LVRS repository root from {} (expected CMakeLists.txt, qml, backend)",
            cwd.display()
        )
    })?;

    let build_dir = project_root.join("build");
    validate_deprecated_build_dir(args.build_dir.as_deref(), &project_root)?;

    let home_dir = resolve_home_dir()?;
    let install_prefix = args
        .prefix
        .unwrap_or_else(|| home_dir.join(".local").join("LVRS"));
    let platform_install_root = install_prefix.join("platforms");
    let host_platform = detect_host_platform();
    let host_install_prefix = platform_install_root.join(&host_platform);
    let bootstrap_framework_platforms = normalize_platform_list(
        args.platforms
            .unwrap_or_else(|| DEFAULT_BOOTSTRAP_FRAMEWORK_PLATFORMS.to_string()),
    );

    let build_type = args
        .build_type
        .or_else(|| env::var("CMAKE_BUILD_TYPE").ok())
        .unwrap_or_else(|| "Release".to_string());
    let source_snapshot = !args.no_source_snapshot;
    let register_cmake_registry = !args.no_registry;
    let build_examples = !args.without_examples;
    let build_tests = !args.without_tests;

    let source_install_dir = install_prefix.join("src").join("LVRS");
    let package_config_dir = host_install_prefix.join("lib").join("cmake").join("LVRS");

    ensure_cmake_available()?;

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

    let configure_status = run_command("cmake", &configure_args);
    if !configure_status {
        eprintln!("[LVRS] Configure failed.");
        eprintln!("[LVRS] If Qt is not auto-detected, pass your Qt prefix, e.g.:");
        eprintln!("       CMAKE_PREFIX_PATH=/path/to/Qt lvrs install");
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
            "[LVRS] Apple targets never use x86. Check iOS/Qt kit architecture (arm64) and retry."
        );
        bail!("build step failed");
    }

    println!("[LVRS] Multi-platform framework install completed.");

    if source_snapshot {
        install_source_snapshot(&project_root, &source_install_dir)?;
    }

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
    println!("[LVRS] Downstream CMake  : find_package(LVRS CONFIG REQUIRED)");

    Ok(())
}

fn flag_as_number(value: bool) -> u8 {
    if value { 1 } else { 0 }
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

fn detect_host_platform() -> &'static str {
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

fn install_source_snapshot(project_root: &Path, source_install_dir: &Path) -> Result<()> {
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

        if path.is_dir() {
            run_cmake_copy_directory(&path, &source_install_dir.join(&name))?;
        } else {
            run_cmake_copy_file(&path, source_install_dir)?;
        }
    }

    let source_revision =
        detect_git_revision(project_root).unwrap_or_else(|| "unknown".to_string());
    let installed_at = detect_install_time();
    let info = format!(
        "LVRS source snapshot\nproject_root={}\nsource_revision={}\ninstalled_at={}\n",
        project_root.display(),
        source_revision,
        installed_at
    );
    fs::write(source_install_dir.join("INSTALL_SOURCE_INFO.txt"), info)
        .context("failed to write source snapshot metadata")?;
    Ok(())
}

fn should_skip_snapshot_entry(name: &str) -> bool {
    name == ".git"
        || name == ".idea"
        || name == ".vscode"
        || name == "build"
        || name.starts_with("build-")
        || name.starts_with("cmake-build-")
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
    let content = format!(
        "#!/usr/bin/env sh\n# LVRS environment helper\nexport LVRS_PLATFORMS_ROOT=\"{}\"\nexport LVRS_HOST_PLATFORM=\"{}\"\nexport LVRS_HOST_PREFIX=\"{}\"\nexport CMAKE_PREFIX_PATH=\"{}:${{CMAKE_PREFIX_PATH:-}}\"\nexport QML2_IMPORT_PATH=\"{}/lib/qt6/qml:${{QML2_IMPORT_PATH:-}}\"\n",
        platform_install_root.display(),
        host_platform,
        host_install_prefix.display(),
        install_prefix.display(),
        host_install_prefix.display()
    );
    fs::write(env_file, content)
        .with_context(|| format!("failed to write env helper: {}", env_file.display()))?;

    set_executable_bit(env_file)?;
    Ok(())
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
