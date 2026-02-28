use anyhow::{Context, Result};
use std::env;
use std::path::Path;

const SENTINELS: [&str; 3] = ["CMakeLists.txt", "qml", "backend"];

pub fn run(verbose: u8) -> Result<()> {
    let cwd = env::current_dir().context("failed to read current working directory")?;
    println!("doctor: workspace check");
    println!("cwd: {}", cwd.display());

    if let Some(root) = find_repo_root(&cwd) {
        println!("repo_root: {}", root.display());
        println!("status: ready");

        if verbose > 0 {
            for sentinel in SENTINELS {
                println!("ok: {}", root.join(sentinel).display());
            }
        }
    } else {
        let mut missing = Vec::new();
        for sentinel in SENTINELS {
            let candidate = cwd.join(sentinel);
            if !candidate.exists() {
                missing.push(sentinel);
            }
        }

        println!("status: partial");
        println!("missing: {}", missing.join(", "));
        println!("hint: expected LVRS sentinels were not found in current or parent paths.");
    }

    if verbose > 0 {
        println!("cargo.toml: {}", exists_local("Cargo.toml"));
        println!("src/main.rs: {}", exists_local("src/main.rs"));
    }

    Ok(())
}

fn exists_local(path: &str) -> bool {
    Path::new(path).exists()
}

fn find_repo_root(start: &Path) -> Option<&Path> {
    let mut cursor = Some(start);
    while let Some(path) = cursor {
        if has_sentinels(path) {
            return Some(path);
        }
        cursor = path.parent();
    }
    None
}

fn has_sentinels(path: &Path) -> bool {
    SENTINELS.iter().all(|name| path.join(name).exists())
}
