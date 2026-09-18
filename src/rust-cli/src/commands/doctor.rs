use super::{bootstrap, install};
use crate::cli::DoctorArgs;
use anyhow::{Result, bail};
use std::env;
use std::io::{self, Write};

pub fn run(args: DoctorArgs, verbose: u8) -> Result<()> {
    let cwd = env::current_dir()?;
    let home_dir = install::resolve_home_dir()?;
    let install_prefix = install::resolve_install_prefix(args.prefix.clone(), &home_dir)?;
    let project_root = install::resolve_project_root(None, Some(&install_prefix))?;
    let host_platform = install::detect_host_platform();

    println!("[LVRS] Doctor cwd        : {}", cwd.display());
    println!("[LVRS] Doctor root       : {}", project_root.display());
    println!("[LVRS] Doctor host       : {host_platform}");
    println!("[LVRS] Install prefix    : {}", install_prefix.display());

    let mut host_configure_args = Vec::new();
    let host_qt = install::ensure_linux_host_prerequisites(
        host_platform,
        &mut host_configure_args,
        &[],
        args.fix,
    )?;
    println!("[LVRS] Host prerequisites: ready");
    if let Some(config) = &host_qt {
        println!("[LVRS] Linux Qt         : {}", config.prefix.display());
        println!("[LVRS] Linux Qt6_DIR    : {}", config.qt6_dir.display());
        println!("[LVRS] Qt detect        : {}", config.source);
        if verbose > 0 && !config.injected.is_empty() {
            println!("[LVRS] Host injects:");
            for item in &config.injected {
                println!("  - {item}");
            }
        }
    }

    if args.bootstrap {
        let (platforms, source) =
            bootstrap::resolve_bootstrap_platforms(args.platforms.clone(), args.with_wasm);
        let selected_platforms = bootstrap::parse_platform_list(&platforms);
        if selected_platforms.is_empty() {
            bail!(
                "[LVRS] bootstrap platform list is empty. set --platforms with at least one target."
            );
        }

        bootstrap::ensure_main_entrypoints_ready(&project_root)?;
        let mut bootstrap_cmake_args = Vec::new();
        let hints =
            bootstrap::apply_auto_bootstrap_hints(&mut bootstrap_cmake_args, &selected_platforms)?;

        println!("[LVRS] Bootstrap source : {source}");
        println!("[LVRS] Bootstrap targets: {}", selected_platforms.join(";"));
        println!("[LVRS] Bootstrap entry : ready");

        if hints.injected.is_empty() {
            println!("[LVRS] Bootstrap hints  : none");
        } else {
            println!("[LVRS] Bootstrap hints  :");
            for item in &hints.injected {
                println!("  - {item}");
            }
        }

        if hints.warnings.is_empty() {
            println!("[LVRS] Bootstrap detect : ready");
        } else {
            println!("[LVRS] Bootstrap warnings:");
            for item in &hints.warnings {
                println!("  - {item}");
            }
            let _ = io::stdout().flush();
            bail!(
                "[LVRS] bootstrap readiness is incomplete. install the missing toolchains or rerun doctor with --platforms limited to the targets you intend to build."
            );
        }

        if verbose > 0 && !bootstrap_cmake_args.is_empty() {
            println!("[LVRS] Bootstrap injects:");
            for item in &bootstrap_cmake_args {
                println!("  - {item}");
            }
        }
    }

    println!("[LVRS] Doctor status     : ready");
    Ok(())
}
