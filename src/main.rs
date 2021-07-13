#![forbid(clippy::as_conversions, clippy::cast_ptr_alignment, trivial_casts, unconditional_recursion)]

mod args;
mod conf;
mod snmp;

use std::convert::TryInto;
use std::ffi::CString;
use std::io::Write;
use std::mem::drop;
use std::process::exit;

use crate::snmp::OutletValue;
use crate::args::{Action, Result as PAR};
use crate::conf::EntrySnmpState;

const ERRPFX: &str = "zs-apc-spdu-ctl: ";

fn main() {
    let argd = match args::parse_args() {
        PAR::Got(a) => a,
        PAR::Help => {
            args::print_usage();
            args::print_help();
            exit(0)
        }
        PAR::Error(e) => {
            use crate::args::Error::*;
            match e {
                NotEnoughArgs => args::print_usage(),
                MissingConfArg => eprintln!("{}missing argument for --conf", ERRPFX),
                UnknownOption(i) => eprintln!("{}unknown option: {}", ERRPFX, i),
                EmptyActionOrObject => eprintln!("{}got empty action or object", ERRPFX),
            }
            exit(1)
        }
    };

    let mut fh = match std::fs::File::open(&argd.conffile) {
        Ok(x) => x,
        Err(e) => {
            eprintln!("{}{}: config file not found", ERRPFX, argd.conffile);
            exit(1);
        }
    };

    let mut config = match conf::parse_config(std::io::BufReader::new(fh)) {
        Ok(x) => x,
        Err(e) => {
            use crate::conf::{Error as CEK};
            for i in e {
                match i {
                    CEK::Line { kind, linenr } => eprintln!("{}CONFIG ERROR: line {}: {}", ERRPFX, linenr, kind),
                    CEK::NoEnts => eprintln!("{}CONFIG ERROR: no entries found", ERRPFX),
                    CEK::EntMissingField { entname, field } => eprintln!("{}CONFIG ERROR @ {}: no '{}' given", ERRPFX, entname, field),
                }
            }
            eprintln!("{}CONFIG READ failed", ERRPFX);
            exit(1);
        }
    };

    let mut confent = match config.get_mut(&argd.ent) {
        Some(x) => x,
        None => {
            eprintln!("{}unknown object: {}", ERRPFX, argd.ent);
            exit(1);
        }
    };

    let community = CString::new(if argd.action.needs_priv() {
        "private"
    } else {
        "public"
    }).unwrap();

    let mut snmp_sess = crate::snmp::Session::new(&CString::new(confent.asl.apc.as_bytes()).expect("invalid APC name"), &community);
    drop(community);
    let mut suc = true;

    match argd.action {
        Action::Status => {
            let mut host = confent.host.clone();
            let is_online = std::thread::spawn(move || host.is_online());
            print!("OUTLETS: ");
            match snmp_sess.stat() {
                None => println!("unknown"),
                Some(st) => {
                    let summary = match confent.asl.eval_outlets(&st) {
                        EntrySnmpState::Done => "on",
                        EntrySnmpState::Partial => "partial on",
                        EntrySnmpState::None => "off",
                    };
                    println!("{}", summary);
                    confent.asl.foreach_outlet(|i| if usize::from(i) < st.len() {
                        println!("  {} = {}", i + 1, if st[i as usize] { "1" } else { "0" });
                    });
                },
            }
            println!("NETWORK: {}line", if is_online.join().expect("network fping thread failed") { "on" } else { "off" });
        }

        Action::Switch(true) => {
            if confent.asl.set_outlets(&mut snmp_sess, OutletValue::On) != EntrySnmpState::Done {
                suc = false;
            } else {
                print!("waiting for server to start ... ");
                std::io::stdout().flush().unwrap();
                if !confent.host.wait_for_host(true) {
                    println!("failed");
                    suc = false;
                } else {
                    println!("online");
                }
            }
        }

        Action::Switch(false) => {
            println!("waiting for server to stop ... ");
            if !confent.host.wait_for_host(false) {
                println!("- failed");
                suc = false;
            } else {
                std::io::stdout().flush().unwrap();
                // graceful shutdown
                std::thread::sleep(std::time::Duration::from_secs(1));

                if confent.asl.set_outlets(&mut snmp_sess, OutletValue::Off) != EntrySnmpState::None {
                    println!("- partial");
                    suc = false;
                } else {
                    println!("- offline");
                }
            }
        }
    }

    drop(snmp_sess);

    if !suc {
        exit(2);
    }
}
