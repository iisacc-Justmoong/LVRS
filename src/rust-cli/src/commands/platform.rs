use anyhow::Result;

pub fn run(verbose: u8) -> Result<()> {
    println!("platform: {}", std::env::consts::OS);
    println!("arch: {}", std::env::consts::ARCH);
    println!("family: {}", std::env::consts::FAMILY);

    if verbose > 0 {
        println!("exe_suffix: {}", std::env::consts::EXE_SUFFIX);
        println!("dll_prefix: {}", std::env::consts::DLL_PREFIX);
        println!("dll_suffix: {}", std::env::consts::DLL_SUFFIX);
    }

    Ok(())
}
