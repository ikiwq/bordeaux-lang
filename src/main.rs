use std::fs;

use crate::lang::{parser::Parser, scanner::Scanner};

pub mod lang;

fn main() {
    let contents = fs::read_to_string("main.bdx").expect("Cannot read main file");

    let tokens = match Scanner::scan(&contents) {
        Ok(t) => t,
        Err(errs) => {
            for err in errs {
                println!("{}", err);
            }
            std::process::exit(1);
        }
    };

    let program = match Parser::parse(tokens) {
        Ok(p) => p,
        Err(errs) => {
            for err in errs {
                println!("{}", err);
            }
            std::process::exit(1);
        }
    };

    println!("{:?}", program)
}
