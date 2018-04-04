/** zs-apc-spdu-ctl main.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include <stdio.h>

#include "conf.hpp"
#include "snmp/zs_snmp.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  zs::config config;
  {
    std::string cfgfile = "/etc/zs-apc-spdu.conf";
    if(argc > 1) cfgfile = argv[1];
    if(!config.read_from(cfgfile)) {
      puts("CONFIG READ failed");
      return 1;
    }
  }
  return 0;

  // test
  zs::snmp my_snmp;

  if(!my_snmp.init("192.168.6.88", "private")) {
    puts("INIT failed");
    return 1;
  }

  if(!my_snmp.set_outlet(8, ZS_SNMP_ON)) {
    puts("SET_OUTLET failed");
    return 1;
  }

  if(!my_snmp.init("192.168.6.88", "public")) {
    puts("INIT failed");
    return 1;
  }

  vector<bool> st;
  if(!my_snmp.get_stat(st)) {
    puts("GET_STAT failed");
    return 1;
  }

  for(const auto i : st) printf(" %d", i ? 1 : 0);
  puts("");

  return 0;
}
