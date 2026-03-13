mod bootstrap;
mod doctor;
mod install;
mod platform;

use crate::cli::{Cli, Command};

pub fn dispatch(cli: Cli) -> anyhow::Result<()> {
    match cli.command {
        Command::Install(args) => install::run(args, cli.verbose),
        Command::Bootstrap(args) => bootstrap::run(args, cli.verbose),
        Command::Doctor(args) => doctor::run(args, cli.verbose),
        Command::Platform => platform::run(cli.verbose),
        Command::Echo { message } => {
            println!("{message}");
            Ok(())
        }
    }
}
