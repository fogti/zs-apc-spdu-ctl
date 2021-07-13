use std::collections::HashMap;
use std::fmt;
use super::{ConfigEntry, Config};

pub enum EntErrorKind {
    InvalidOutlet,
    InvalidStmt,
    Io(std::io::Error),
}

use EntErrorKind::*;

pub enum EntField {
    Apc,
    Host,
    Outlets,
}

pub enum Error {
    Line {
        kind: EntErrorKind,
        linenr: usize,
    },
    EntMissingField {
        entname: String,
        field: EntField,
    },
    NoEnts,
}

impl fmt::Display for EntErrorKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            InvalidOutlet => write!(f, "invalid outlet"),
            InvalidStmt => write!(f, "invalid config stmt"),
            Io(e) => write!(f, "{}", e),
        }
    }
}

impl fmt::Display for EntField {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use EntField::*;
        write!(f, "{}", match self {
            Apc => "apc",
            Host => "host",
            Outlets => "outlets",
        })
    }
}

impl ConfigEntry {
    fn handle_stmt(&mut self, cmd: &str, arg: &str) -> Result<(), EntErrorKind> {
        match cmd {
            "apc" => {
                self.asl.apc = arg.to_string();
                Ok(())
            }
            "host" => {
                self.host.host = arg.to_string();
                Ok(())
            }
            "outlets" => {
                for i in arg.split(' ') {
                    let i = i.parse::<u32>().map_err(|_| InvalidOutlet)?;
                    if i == 0 {
                        return Err(InvalidOutlet);
                    }
                    self.asl.outlets |= 1 << (i - 1);
                }
                Ok(())
            }
            _ => Err(InvalidStmt),
        }
    }
}

enum ParseMode {
    Global,
    Entity(String),
}

pub fn parse_config<Input>(input: Input) -> Result<Config, Vec<Error>>
where
    Input: std::io::Read + std::io::BufRead,
{
    let mut glapc = None;
    let mut ret: Config = HashMap::new();
    let mut errs = Vec::new();

    let mut it = input.lines()
        .enumerate()
        .filter_map(|(linenr, i)| {
            if let Ok(line) = &i {
                if line.is_empty() || line.starts_with('#') {
                    return None;
                }
            }
            Some(match i {
                Ok(j) => Ok((linenr, j)),
                Err(e) => Err(Error::Line {
                    kind: EntErrorKind::Io(e),
                    linenr,
                })
            })
        });

    let mut mode = ParseMode::Global;

    while let Some(i) = it.next() {
        let (linenr, line) = match i {
            Ok(x) => x,
            Err(e) => {
                errs.push(e);
                break;
            },
        };
        if line.starts_with(':') {
            mode = ParseMode::Entity(line[1..].to_string());
        } else if let Some(sppos) = line.find(' ') {
            if sppos == 0 {
                errs.push(Error::Line {
                    kind: InvalidStmt,
                    linenr,
                }.into());
                continue;
            }

            // split line
            let (cmd, arg) = (&line[..sppos], &line[sppos + 1..]);

            match &mode {
                ParseMode::Global => {
                    if cmd == "apc" {
                        glapc = Some(arg.to_string());
                    } else {
                        errs.push(Error::Line {
                            kind: InvalidStmt,
                            linenr,
                        }.into());
                    }
                }
                ParseMode::Entity(e) => {
                    if let Err(kind) = ret.entry(e.to_string()).or_default().handle_stmt(cmd, arg) {
                        errs.push(Error::Line {
                            kind,
                            linenr,
                        }.into());
                    }
                }
            }
        } else {
            errs.push(Error::Line {
                kind: InvalidStmt,
                linenr,
            }.into());
        }
    }

    if ret.is_empty() {
        errs.push(Error::NoEnts);
        return Err(errs);
    }

    for (entname, i) in &mut ret {
        if i.host.host.is_empty() {
            errs.push(Error::EntMissingField {
                entname: entname.clone(),
                field: EntField::Host,
            });
        }
        if i.asl.apc.is_empty() {
            if let Some(apc) = &glapc {
                i.asl.apc = apc.to_string();
            } else {
                errs.push(Error::EntMissingField {
                    entname: entname.clone(),
                    field: EntField::Apc,
                });
            }
        }
        if i.asl.outlets == 0 {
            errs.push(Error::EntMissingField {
                entname: entname.clone(),
                field: EntField::Outlets,
            });
        }
    }

    if errs.is_empty() {
        Ok(ret)
    } else {
        Err(errs)
    }
}
