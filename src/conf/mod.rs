use std::collections::HashMap;

#[derive(Clone)]
pub struct ApcSlice {
    pub apc: String,
    outlets: u32,
}

#[derive(Clone)]
pub struct Host {
    pub host: String,
    cached_ipv6: Option<bool>,
}

pub struct ConfigEntry {
    pub asl: ApcSlice,
    pub host: Host,
}

pub type Config = HashMap<String, ConfigEntry>;

impl Default for ApcSlice {
    fn default() -> Self {
        Self {
            apc: String::new(),
            outlets: 0,
        }
    }
}

impl Default for Host {
    fn default() -> Self {
        Self {
            host: String::new(),
            cached_ipv6: None,
        }
    }
}

impl Default for ConfigEntry {
    fn default() -> Self {
        Self {
            asl: ApcSlice::default(),
            host: Host::default(),
        }
    }
}

mod host;
mod outlets;
mod parse;

pub use outlets::EntrySnmpState;
pub use parse::*;
