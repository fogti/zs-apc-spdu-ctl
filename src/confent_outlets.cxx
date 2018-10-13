/** zs-apc-spdu-ctl confent_outlets.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include "conf.hpp"

using namespace std;

namespace zs {

auto config_ent::set_outlets(zs::snmp &my_snmp, const uint8_t val) -> ent_snmp_state {
  size_t cnt = 0;
  for(const auto i : outlets) if(my_snmp.set_outlet(i, val)) ++cnt;
  return outlets_cnt2state((val == ZS_SNMP_OFF) ? (outlets.size() - cnt) : cnt);
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
