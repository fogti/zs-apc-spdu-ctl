use crate::snmp::{Session as SnmpSession, OutletValue};

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum EntrySnmpState {
    None,
    Done,
    Partial,
}

impl super::ApcSlice {
    pub fn foreach_outlet<F: FnMut(u8)>(&self, mut f: F) {
        let (mut ols, mut i) = (self.outlets, 0);
        while ols > 0 {
            if ols & 0b1 == 0b1 {
                f(i);
            }
            ols >>= 1;
            i += 1;
        }
    }

    fn outlets_cnt2state(&self, cnt: u32) -> EntrySnmpState {
        use EntrySnmpState::*;
        if cnt == self.outlets.count_ones() {
            Done
        } else if cnt > 0 {
            Partial
        } else {
            None
        }
    }

    pub fn set_outlets(&self, snmp: &mut SnmpSession, val: OutletValue) -> EntrySnmpState {
        let mut cnt = 0;
        let mut outlets_all_cnt = 0;
        self.foreach_outlet(|i| {
            if snmp.set_outlet(i, val) {
                cnt += 1;
            }
            outlets_all_cnt += 1;
        });
        self.outlets_cnt2state(if val == OutletValue::Off {
            outlets_all_cnt - cnt
        } else {
            cnt
        })
    }

    pub fn eval_outlets(&self, st: &[bool]) -> EntrySnmpState {
        let mut cnt = 0;
        self.foreach_outlet(|i| {
            if *st.get(usize::from(i)).unwrap_or(&false) {
                cnt += 1;
            }
        });
        self.outlets_cnt2state(cnt)
    }
}
