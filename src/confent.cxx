/** zs-apc-spdu-ctl confent.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <thread>
#include <chrono>

#include "conf.hpp"

using namespace std;

namespace zs {

config_ent::config_ent() noexcept {
  _cached_ipv6[1] = _cached_ipv6[0] = false;
}

bool config_ent::host_is_ipv6() noexcept {
  if(!_cached_ipv6[0]) {
    struct addrinfo hint, *res = nullptr;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = PF_UNSPEC;
    hint.ai_flags = AI_NUMERICHOST;

    if(getaddrinfo(host.c_str(), nullptr, &hint, &res))
      return false;

    _cached_ipv6[0] = true;
    _cached_ipv6[1] = (res->ai_family == AF_INET6);
    freeaddrinfo(res);
  }

  return _cached_ipv6[1];
}

bool config_ent::host_is_online() {
  string cmd = "/usr/sbin/fping";
  cmd.reserve(50);
  if(host_is_ipv6()) cmd += '6';
  cmd += " '" + host + "' &>/dev/null";
  return system(cmd.c_str());
}

bool config_ent::wait_for_host(const bool online) {
  string cmd = "/usr/sbin/fping";
  cmd.reserve(60);
  if(host_is_ipv6()) cmd += '6';
  // wait for online max ~8,5 minutes
  if(online) cmd += " -r 10";
  cmd += " '" + host + "' &>/dev/null";
  if(online) return system(cmd.c_str());

  // wait for offline, but max 2 minutes
  size_t i;
  for(i = 0; i < 120 && system(cmd.c_str()); ++i)
    this_thread::sleep_for(chrono::seconds(1));
  return i < 120;
}

}
