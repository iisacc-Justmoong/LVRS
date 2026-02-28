use super::install;
use crate::cli::BootstrapArgs;
use anyhow::{Context, Result, bail};
use std::env;
use std::fs;
use std::path::{Path, PathBuf};

const DESKTOP_MOBILE_PLATFORMS: &str = "macos;linux;windows;ios;android";
const DESKTOP_MOBILE_WASM_PLATFORMS: &str = "macos;linux;windows;ios;android;wasm";
const MAIN_CPP_RELATIVE_PATH: &str = "main.cpp";
const MAIN_ROOT_OBJECT_MARKER: &str = "rootObject = QStringLiteral(\"Main\")";
const MAIN_BOOTSTRAP_CALL_MARKER: &str = "runBootstrappedQmlApp";

pub fn run(mut args: BootstrapArgs, verbose: u8) -> Result<()> {
    let cwd = env::current_dir().context("failed to read current working directory")?;
    let project_root = find_project_root(&cwd).with_context(|| {
        format!(
            "failed to locate LVRS repository root from {} (expected CMakeLists.txt, qml, backend)",
            cwd.display()
        )
    })?;

    ensure_main_entrypoints_ready(&project_root)?;

    if args.install.platforms.is_none() {
        args.install.platforms = Some(default_bootstrap_platforms(args.with_wasm).to_string());
    }

    println!(
        "[LVRS] Bootstrap profile: {}",
        if args.with_wasm {
            "desktop+mobile+wasm"
        } else {
            "desktop+mobile"
        }
    );
    println!(
        "[LVRS] Main entrypoint : {}",
        project_root.join(MAIN_CPP_RELATIVE_PATH).display()
    );
    println!("[LVRS] Main root object: Main");

    install::run(args.install, verbose)
}

fn default_bootstrap_platforms(with_wasm: bool) -> &'static str {
    if with_wasm {
        DESKTOP_MOBILE_WASM_PLATFORMS
    } else {
        DESKTOP_MOBILE_PLATFORMS
    }
}

fn ensure_main_entrypoints_ready(project_root: &Path) -> Result<()> {
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
