use clap::{ArgAction, Args, Parser, Subcommand};
use std::path::PathBuf;

#[derive(Debug, Parser)]
#[command(
    name = "lvrs",
    version,
    about = "LVRS utility CLI bootstrap",
    long_about = None
)]
pub struct Cli {
    #[arg(short, long, action = ArgAction::Count, global = true)]
    pub verbose: u8,

    #[command(subcommand)]
    pub command: Command,
}

#[derive(Debug, Subcommand)]
pub enum Command {
    /// Install LVRS framework packages (replacement of install.sh).
    Install(InstallArgs),

    /// Bootstrap desktop/mobile multi-platform setup from Main entrypoint defaults.
    Bootstrap(BootstrapArgs),

    /// Run host/bootstrap dependency checks and optional fixups.
    Doctor(DoctorArgs),

    /// Print platform information.
    Platform,

    /// Print a message (sanity command for CLI plumbing).
    Echo { message: String },
}

#[derive(Debug, Args)]
pub struct InstallArgs {
    /// Install prefix (default: ~/.local/LVRS)
    #[arg(long, value_name = "path")]
    pub prefix: Option<PathBuf>,

    /// Deprecated. Build directory is fixed to <repo>/build
    #[arg(long = "build-dir", value_name = "path")]
    pub build_dir: Option<PathBuf>,

    /// CMake build type (default: CMAKE_BUILD_TYPE or Release)
    #[arg(long = "build-type", value_name = "type")]
    pub build_type: Option<String>,

    /// Bootstrap platforms (comma/semicolon list)
    #[arg(long, value_name = "list")]
    pub platforms: Option<String>,

    /// Deprecated no-op (clean reinstall is always enabled)
    #[arg(long)]
    pub clean: bool,

    /// Disable host configure-time example targets and omit snapshot example binaries
    #[arg(long = "without-examples")]
    pub without_examples: bool,

    /// Disable host configure-time test targets
    #[arg(long = "without-tests")]
    pub without_tests: bool,

    /// Linux only: install missing distro packages for host build dependencies before configure
    #[arg(long = "install-linux-deps")]
    pub install_linux_deps: bool,

    /// Deprecated/unsupported
    #[arg(long = "force-x86-qt-tools")]
    pub force_x86_qt_tools: bool,

    /// Skip source snapshot copy into <prefix>/src/LVRS
    #[arg(long = "no-source-snapshot")]
    pub no_source_snapshot: bool,

    /// Skip CMake user package registry registration
    #[arg(long = "no-registry")]
    pub no_registry: bool,

    /// Additional arguments passed to cmake configure after `--`
    #[arg(last = true)]
    pub cmake_args: Vec<String>,
}

#[derive(Debug, Args)]
pub struct BootstrapArgs {
    /// Include wasm in the default bootstrap platform matrix
    #[arg(long = "with-wasm")]
    pub with_wasm: bool,

    #[command(flatten)]
    pub install: InstallArgs,
}

#[derive(Debug, Args)]
pub struct DoctorArgs {
    /// Install prefix to inspect (default: ~/.local/LVRS)
    #[arg(long, value_name = "path")]
    pub prefix: Option<PathBuf>,

    /// Linux only: install missing distro packages for host build dependencies
    #[arg(long = "fix", alias = "install-linux-deps")]
    pub fix: bool,

    /// Validate Main-entry bootstrap readiness and cross-platform auto-detect hints
    #[arg(long = "bootstrap")]
    pub bootstrap: bool,

    /// Bootstrap platforms to evaluate with --bootstrap (comma/semicolon list)
    #[arg(long, value_name = "list")]
    pub platforms: Option<String>,

    /// Include wasm in the default bootstrap platform set for --bootstrap
    #[arg(long = "with-wasm")]
    pub with_wasm: bool,
}

#[cfg(test)]
mod tests {
    use super::*;
    use clap::Parser;

    #[test]
    fn doctor_fix_flags_parse() {
        let cli = Cli::try_parse_from(["lvrs", "doctor", "--fix"]).expect("doctor --fix");
        match cli.command {
            Command::Doctor(args) => assert!(args.fix),
            other => panic!("unexpected command: {other:?}"),
        }

        let cli = Cli::try_parse_from(["lvrs", "doctor", "--install-linux-deps"])
            .expect("doctor --install-linux-deps");
        match cli.command {
            Command::Doctor(args) => assert!(args.fix),
            other => panic!("unexpected command: {other:?}"),
        }
    }

    #[test]
    fn doctor_bootstrap_flags_parse() {
        let cli = Cli::try_parse_from([
            "lvrs",
            "doctor",
            "--bootstrap",
            "--with-wasm",
            "--platforms",
            "linux,android,wasm",
        ])
        .expect("doctor bootstrap flags");

        match cli.command {
            Command::Doctor(args) => {
                assert!(args.bootstrap);
                assert!(args.with_wasm);
                assert_eq!(args.platforms.as_deref(), Some("linux,android,wasm"));
            }
            other => panic!("unexpected command: {other:?}"),
        }
    }
}
