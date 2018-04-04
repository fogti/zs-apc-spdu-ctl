/** zs-apc-spdu-ctl zs_snmp.hpp
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#pragma once
#include <inttypes.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/types.h>

#include <string>
#include <vector>

#define ZS_SNMP_ON       1
#define ZS_SNMP_OFF      2
#define ZS_SNMP_RESTART  3

namespace zs {
class snmp {
  netsnmp_session *_sess;

  auto handle_vars2err(netsnmp_variable_list *vars) noexcept -> const char *;
  bool parse_stat(const std::string &in, std::vector<bool> &out);

 public:
  snmp() noexcept;
  ~snmp();

  // init - closes a previous session and opens a new session
  bool init(const char * const peer, const char * const community) noexcept;

  // get_stat - retrieves the status from the APC
  bool get_stat(std::vector<bool> &st);

  bool set_outlet(uint8_t outlet, uint8_t val);
};
}
