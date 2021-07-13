const DFL_CONFFILE: &str = "/etc/zs-apc-spdu.conf";

#[derive(Clone, Copy)]
pub enum Action {
    Status,
    Switch(bool),
}

impl Action {
    pub fn needs_priv(self) -> bool {
        match self {
            Action::Status => false,
            Action::Switch(_) => true,
        }
    }
}

pub struct Args {
    pub conffile: String,
    pub action: Action,
    pub ent: String,
}

pub fn print_usage() {
    eprintln!("USAGE: zs-apc-spdu-ctl [--conf CONFIGFILE] ACTION OBJECT");
}

pub fn print_help() {
    print_usage();
    println!("\n  --conf CONFIGFILE  [default = {}]  use a different configuration\n\n{}",
        DFL_CONFFILE,
        indoc::indoc!(r#"
          actions:
            status             print outlet status + is-online (via fping)
            switch-on          foreach assoc outlet = ON + wait for is-online
            switch-off         wait for is-offline + foreach assoc outlet = OFF

          config syntax:
            # global scope
            apc MASTER-APC     (optional) specify the default used APC

            # local scope
            :OBJECT            specify the OBJECT name
            apc APC            (optional if GLOBAL apc is set) specify an APC for this OBJECT only
            outlets 1 4 7      specify the associated outlets
            host HOST          specify the ping'ed host

          (C) 2021 Alain Zscheile"#),
    );
}

pub enum Error {
    NotEnoughArgs,
    MissingConfArg,
    UnknownOption(String),
    EmptyObject,
    InvalidAction,
}

pub enum Result {
    Got(Args),
    Help,
    Error(Error),
}

pub fn parse_args() -> Result {
    use self::Error::*;
    use self::Result::*;

    let mut conffile = None;
    let mut argsi: Vec<_> = std::env::args().skip(1).collect();

    if &argsi[..] == ["--help"] {
        return Help;
    }

    if argsi.len() < 3 {
        return Error(NotEnoughArgs);
    }

    let (action, ent) = {
        let mut posargs = argsi.split_off(argsi.len() - 2).into_iter();
        let action = posargs.next().unwrap();
        let ent = posargs.next().unwrap();
        assert!(posargs.next().is_none());
        (action, ent)
    };

    let mut argsi = argsi.into_iter();
    while let Some(i) = argsi.next() {
        if i == "--conf" {
            if let Some(j) = argsi.next() {
                conffile = Some(j);
            } else {
                return Error(MissingConfArg);
            }
        } else {
            return Error(UnknownOption(i));
        }
    }

    if ent.is_empty() {
        return Error(EmptyObject);
    }

    Got(Args {
        conffile: conffile.unwrap_or_else(|| DFL_CONFFILE.to_string()),
        action: match action.as_str() {
            "status" => Action::Status,
            "switch-on" => Action::Switch(true),
            "switch-off" => Action::Switch(false),
            _ => return Error(InvalidAction)
        },
        ent,
    })
}
