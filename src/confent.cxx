/** zs-apc-spdu-ctl confent.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "conf.hpp"

using namespace std;

namespace zs {

config_ent::config_ent() noexcept {
  _cached_ipv6[0] = false;
  _cached_ipv6[1] = false;
}

bool config_ent::host_is_ipv6() noexcept {
  if(_cached_ipv6[0]) return _cached_ipv6[1];

  struct addrinfo hint, *res = nullptr;
  memset(&hint, 0, sizeof(hint));

  hint.ai_family = PF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;

  if(getaddrinfo(host.c_str(), nullptr, &hint, &res))
    return false;

  const bool ret = (res->ai_family == AF_INET6);
  freeaddrinfo(res);

  _cached_ipv6[0] = true;
  _cached_ipv6[1] = ret;
  return ret;
}

bool config_ent::host_is_online() {
  string cmd = "/usr/sbin/fping";
  if(host_is_ipv6()) cmd += '6';
  cmd += " '" + host + "' &>/dev/null";
  return system(cmd.c_str());
}

auto config_ent::set_outlets(zs::snmp &my_snmp, const uint8_t val) -> ent_snmp_state {
  size_t cnt = 0;
  for(const auto i : outlets) if(my_snmp.set_outlet(i, val)) ++cnt;
  return outlets_cnt2state(cnt);
}

auto config_ent::get_outlets(zs::snmp &my_snmp) -> ent_snmp_state {
  vector<bool> st;
  if(!my_snmp.get_stat(st)) return ent_snmp_state::FAIL;
  return get_outlets(st);
}

auto config_ent::get_outlets(const std::vector<bool> &st) const noexcept -> ent_snmp_state {
  size_t cnt = 0;
  for(const auto i : outlets) {
    if(!i || (i - 1) > st.size()) continue;
    if(st[i - 1]) ++cnt;
  }
  return outlets_cnt2state(cnt);
}

auto config_ent::outlets_cnt2state(const size_t cnt) const noexcept -> ent_snmp_state {
  if(cnt == outlets.size())
    return ent_snmp_state::DONE;
  if(cnt)
    return ent_snmp_state::PARTIAL;

  return ent_snmp_state::NONE;
}

}
