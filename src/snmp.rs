#![forbid(clippy::as_conversions, clippy::cast_ptr_alignment, trivial_casts, unconditional_recursion)]

use libc::size_t;
use std::ffi::CStr;
use std::os::raw::{c_char, c_void};

enum State {
    Start,
    O,
    N,
    F1,
    F2,
}

fn snmp_parse_stat(inp: &[u8]) -> Option<Vec<bool>> {
    let mut st = State::Start;
    let mut ret = Vec::new();
    for &i in inp {
        match st {
            State::Start => {
                if i == b'O' {
                    st = State::O;
                } else if i != b' ' {
                    return None;
                }
            },

            State::F2 => {
                if i != b' ' {
                    return None;
                }
                st = State::Start;
                ret.push(false);
            }

            State::N => {
                if i != b' ' {
                    return None;
                }
                st = State::Start;
                ret.push(true);
            }

            State::O => {
                // the following checks for 'n' & 'f'
                if (i | 0x8) != 0x6e {
                    return None;
                }
                st = if i & 0x8 != 0 {
                    State::N
                } else {
                    State::F1
                };
            }

            State::F1 => {
                if i != b'f' {
                    return None;
                }
                st = State::F2;
            }
        }
    }

    if let State::Start = st {
        Some(ret)
    } else {
        None
    }
}

#[link(name = "zs_snmp_support")]
extern {
    fn zs_snmp_init();
    fn zs_snmp_cleanup();
    fn zs_snmp_free(s: *mut c_char);

    fn zs_snmp_open(peer: *const c_char, community: *const c_char) -> *mut c_void;
    fn zs_snmp_close(sd: *mut c_void);

    fn zs_snmp_get_stat(sd: *mut c_void, ststr: *mut *mut c_char, stslen: *mut size_t) -> bool;
    fn zs_snmp_set_outlet(sd: *mut c_void, outlet: u8, val: u8) -> bool;
}

#[ctor::ctor]
fn init() {
    unsafe { zs_snmp_init(); }
}

#[ctor::dtor]
fn cleanup() {
    unsafe { zs_snmp_cleanup(); }
}

pub struct Session {
    sd: *mut c_void,
}

#[derive(Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum OutletValue {
    On = 1,
    Off = 2,
    Restart = 3,
}

impl Drop for Session {
    fn drop(&mut self) {
        unsafe {
            zs_snmp_close(self.sd);
        }
    }
}

impl Session {
    pub fn new(peer: &CStr, community: &CStr) -> Session {
        let sd = unsafe {
            zs_snmp_open(peer.as_ptr(), community.as_ptr())
        };
        Session { sd }
    }

    pub fn stat(&mut self) -> Option<Vec<bool>> {
        let mut ststr: *mut c_char = std::ptr::null_mut();
        let mut stslen: size_t = 0;
        let tmp = unsafe {
            zs_snmp_get_stat(self.sd, &mut ststr, &mut stslen)
        };
        if !tmp {
            return None;
        }
        let ret = snmp_parse_stat(unsafe { std::slice::from_raw_parts_mut(ststr as *mut u8, stslen) });
        unsafe { zs_snmp_free(ststr) };
        ret
    }

    pub fn set_outlet(&mut self, outlet: u8, val: OutletValue) -> bool {
        // BEWARE: we are given an zero-indexed outlet, but the C code expects one-indexed outlets
        unsafe { zs_snmp_set_outlet(self.sd, outlet + 1, val as u8) }
    }
}
