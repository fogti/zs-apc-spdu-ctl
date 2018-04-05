/** zs-apc-spdu-ctl conf.hpp
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#pragma once
#include <inttypes.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "snmp/zs_snmp.hpp"

namespace zs {

enum struct ent_snmp_state {
  NONE, DONE, PARTIAL, FAIL
};

struct config_ent {
  std::string apc, host;
  std::vector<uint8_t> outlets;

  config_ent() noexcept;
  bool host_is_ipv6() noexcept;
  bool host_is_online();

  /* wait_for_host
   * waits for a host to become up/down
   * @param online  specified target (1 -> wait for up ...)
   * @return success?
   */
  bool wait_for_host(const bool online);

  /* set_outlets
   * sets on/off outlets
   * @param &snmp  expected already initialized zs::snmp instance
   */
  auto set_outlets(zs::snmp&, const uint8_t val) -> ent_snmp_state;

  auto get_outlets(zs::snmp&) -> ent_snmp_state;
  auto get_outlets(const std::vector<bool> &st) const noexcept -> ent_snmp_state;

 private:
  bool _cached_ipv6[2];

  auto outlets_cnt2state(const size_t cnt) const noexcept -> ent_snmp_state;
};

struct config {
  std::string apc;
  std::unordered_map<std::string, config_ent> ents;

  bool read_from(const std::string &file);
  auto get_apc_of(const config_ent* const ent) -> const char *;
};

}
