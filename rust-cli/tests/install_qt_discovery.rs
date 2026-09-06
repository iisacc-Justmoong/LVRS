#![cfg(target_os = "macos")]

use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::{SystemTime, UNIX_EPOCH};

struct InstallFixture {
    root: PathBuf,
    project: PathBuf,
    prefix: PathBuf,
    qt_version: PathBuf,
}

impl InstallFixture {
    fn new(label: &str) -> Self {
        let nonce = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        let root = Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("build")
            .join(format!("install-qt-{label}-{}-{nonce}", std::process::id()));
        let project = root.join("project");
        let prefix = root.join("installed");
        let qt_version = root.join("external-qt/6.8.3");
        fs::create_dir_all(project.join("qml")).unwrap();
        fs::create_dir_all(project.join("backend")).unwrap();
        fs::create_dir_all(prefix.join("platforms/macos/lib")).unwrap();
        fs::write(
            prefix.join("platforms/macos/lib/existing-library"),
            "keep me",
        )
        .unwrap();
        fs::write(
            project.join("CMakeLists.txt"),
            r#"cmake_minimum_required(VERSION 3.21)
project(LVRS LANGUAGES NONE)
if(NOT DEFINED Qt6_DIR)
    message(FATAL_ERROR "Host Qt6_DIR was not supplied")
endif()
find_package(Qt6 CONFIG REQUIRED PATHS "${Qt6_DIR}" NO_DEFAULT_PATH)
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/resolved-qt.txt" "${Qt6_DIR}")
message(FATAL_ERROR "Intentional configure failure after Qt discovery")
"#,
        )
        .unwrap();
        Self::qt_package(&qt_version.join("macos"));
        Self {
            root,
            project,
            prefix,
            qt_version,
        }
    }

    fn qt_package(prefix: &Path) -> PathBuf {
        let package = prefix.join("lib/cmake/Qt6");
        fs::create_dir_all(&package).unwrap();
        fs::write(package.join("Qt6Config.cmake"), "set(Qt6_FOUND TRUE)\n").unwrap();
        package
    }

    fn command(&self) -> Command {
        let mut command = Command::new(env!("CARGO_BIN_EXE_lvrs"));
        for (key, _) in std::env::vars_os() {
            let key_text = key.to_string_lossy();
            if key_text.starts_with("QT")
                || key_text.starts_with("Qt6")
                || key_text.starts_with("LVRS")
                || key_text.starts_with("CMAKE")
                || key_text.starts_with("QML")
                || key_text.starts_with("DYLD_")
            {
                command.env_remove(key);
            }
        }
        command
            .current_dir(&self.project)
            .env("LVRS_ROOT", &self.project)
            .env("QT_VERSION_ROOT", &self.qt_version)
            .args([
                "install",
                "--platforms",
                "macos",
                "--without-examples",
                "--without-tests",
                "--no-registry",
                "--no-source-snapshot",
                "--prefix",
            ])
            .arg(&self.prefix);
        command
    }

    fn assert_configure_probe(&self, mut command: Command, expected: &Path) {
        let output = command.output().unwrap();
        let diagnostics = format!(
            "{}\n{}",
            String::from_utf8_lossy(&output.stdout),
            String::from_utf8_lossy(&output.stderr)
        );
        assert!(
            !output.status.success(),
            "fixture must fail configure: {diagnostics}"
        );
        let actual = fs::read_to_string(self.project.join("resolved-qt.txt"))
            .unwrap_or_else(|error| panic!("Qt was not discovered: {error}\n{diagnostics}"));
        assert_eq!(actual, expected.to_string_lossy(), "{diagnostics}");
        assert_eq!(
            fs::read_to_string(self.prefix.join("platforms/macos/lib/existing-library"))
                .expect("a configure failure must preserve the previous install"),
            "keep me"
        );
    }
}

impl Drop for InstallFixture {
    fn drop(&mut self) {
        let _ = fs::remove_dir_all(&self.root);
    }
}

#[test]
fn install_discovers_external_macos_kit_with_a_stale_search_path() {
    let fixture = InstallFixture::new("external");
    let mut command = fixture.command();
    command.env("CMAKE_PREFIX_PATH", fixture.root.join("missing-home-qt"));
    fixture.assert_configure_probe(command, &fixture.qt_version.join("macos/lib/cmake/Qt6"));
}

#[test]
fn install_keeps_explicit_typed_qt6_dir_and_existing_install_on_failure() {
    let fixture = InstallFixture::new("explicit-dir");
    let package = InstallFixture::qt_package(&fixture.root.join("custom-qt"));
    let mut command = fixture.command();
    command
        .arg("--")
        .arg(format!("-DQt6_DIR:PATH={}", package.display()));
    fixture.assert_configure_probe(command, &package);
}

#[test]
fn install_uses_explicit_macos_prefix_before_default_kits() {
    let fixture = InstallFixture::new("host-prefix");
    let prefix = fixture.root.join("custom-qt");
    let package = InstallFixture::qt_package(&prefix);
    let mut command = fixture.command();
    command.env("LVRS_BOOTSTRAP_QT_PREFIX_MACOS", &prefix);
    fixture.assert_configure_probe(command, &package);
}

#[test]
fn install_keeps_qt_from_explicit_cmake_prefix_path() {
    let fixture = InstallFixture::new("cmake-prefix");
    let prefix = fixture.root.join("custom-qt");
    let package = InstallFixture::qt_package(&prefix);
    let mut command = fixture.command();
    command.arg("--").arg(format!(
        "-DCMAKE_PREFIX_PATH:STRING={};{}",
        fixture.root.join("other-sdk").display(),
        prefix.display()
    ));
    fixture.assert_configure_probe(command, &package);
}
