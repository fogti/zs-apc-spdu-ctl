use std::time::{Duration, Instant};
use std::process::Command;

fn get_fping_cmd(is_ipv6: bool) -> String {
    let mut cmd = "/usr/sbin/fping".to_string();
    if is_ipv6 {
        cmd += "6";
    }
    cmd
}

impl super::Host {
    fn is_ipv6(&mut self) -> bool {
        if self.cached_ipv6.is_none() {
            use dns_lookup::{getaddrinfo, AddrInfoHints};
            let mut sockets =
            match getaddrinfo(Some(&self.host), None, Some(AddrInfoHints {
                flags: libc::AI_NUMERICHOST,
                ..AddrInfoHints::default()
            })) {
                Ok(x) => x,
                Err(_) => return false,
            };
            self.cached_ipv6 = Some(sockets.any(|i| i.map(|j| j.address == libc::AF_INET6).unwrap_or(false)));
        }
        self.cached_ipv6.unwrap()
    }

    pub fn is_online(&mut self) -> bool {
        let st = Command::new(get_fping_cmd(self.is_ipv6()))
            .args(["-q", &self.host])
            .status();
        // fail rather hard if spawning fping fails, because we could be wasting multiple minutes...
        st.expect("fping failed").success()
    }

    pub fn wait_for_host(&mut self, online: bool) -> bool {
        const MAX_OFFLINE_DUR: Duration = Duration::from_secs(120);
        const STEP_DUR: Duration = Duration::from_millis(10);

        let fping_cmd = get_fping_cmd(self.is_ipv6());
        if online {
            // wait for online max ~8,5 minutes
            let st = Command::new(fping_cmd)
                .args(["-q", "-r", "10", &self.host])
                .status()
                .expect("unable to call fping");
            st.success()
        } else {
            // wait for offline, but max 2 minutes
            let start = Instant::now();

            loop {
                let st = Command::new(&fping_cmd)
                    .args(["-q", &self.host])
                    .status()
                    .expect("unable to call fping");
                if st.code().is_none() {
                    return false;
                } else if !st.success() {
                    return true;
                }
                let curd = Instant::now().duration_since(start);
                if MAX_OFFLINE_DUR <= curd {
                    return false;
                }
                let remaining = MAX_OFFLINE_DUR - curd;
                std::thread::sleep(remaining / 10 + STEP_DUR);
            }
        }
    }
}
