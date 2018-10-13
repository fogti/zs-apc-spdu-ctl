/** zs-apc-spdu-ctl main.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include <stdio.h>
#include <future>
#include <thread>
#include <chrono>

#include "args.hpp"
#include "conf.hpp"
#include "snmp/zs_snmp.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  zs::args_data args;
  if(!zs::parse_args(argc, argv, args)) {
    puts("PARSE ARGS failed");
    return 1;
  }
  zs::config config;
  if(!config.read_from(args.conffile)) {
    puts("CONFIG READ failed");
    return 1;
  }

  zs::config_ent *selent = 0;
  {
    const auto selenti = config.ents.find(args.ent);
    if(selenti == config.ents.end()) {
      printf("%s: object not found\n", args.ent.c_str());
      return 1;
    }
    selent = &(selenti->second);
  }

  if(args.action == "status-verbose") {
    zs::snmp my_snmp;
    if(!my_snmp.init(config.get_apc_of(selent), "public"))
      return 2;

    vector<bool> st;
    if(!my_snmp.get_stat(st)) return 2;

    for(const auto i : selent->outlets) {
      if(!i || (i - 1) > st.size()) continue;
      printf("%u = %d\n", static_cast<uint32_t>(i), st[i - 1] ? 1 : 0);
    }
  } else if(args.action == "status") {
    zs::snmp my_snmp;
    if(!my_snmp.init(config.get_apc_of(selent), "public"))
      return 2;

    future<bool> is_online = async(launch::async, [&] { return selent->host_is_online(); });
    vector<bool> st;
    if(!my_snmp.get_stat(st)) return 2;

    printf("OUTLETS: ");
    switch(selent->get_outlets(my_snmp)) {
      case zs::ent_snmp_state::FAIL:
        puts("unknown"); break;
      case zs::ent_snmp_state::DONE:
        puts("on"); break;
      case zs::ent_snmp_state::PARTIAL:
        puts("partial on"); break;
      default: puts("off");
    }
    printf("NETWORK: %sline\n", is_online.get() ? "on" : "off");
  } else if(args.action == "switch-on") {
    zs::snmp my_snmp;
    if(!my_snmp.init(config.get_apc_of(selent), "private"))
      return 2;

    if(selent->set_outlets(my_snmp, ZS_SNMP_ON) != zs::ent_snmp_state::DONE)
      return 2;

    printf("waiting for server to start ... ");
    if(!selent->wait_for_host(true)) {
      puts("- failed");
      return 2;
    }

    puts("- online");
  } else if(args.action == "switch-off") {
    zs::snmp my_snmp;
    if(!my_snmp.init(config.get_apc_of(selent), "private"))
      return 2;

    printf("waiting for server to shutdown ... ");
    if(!selent->wait_for_host(false)) {
      puts("- failed");
      return 2;
    }

    this_thread::sleep_for(chrono::seconds(1));

    if(selent->set_outlets(my_snmp, ZS_SNMP_OFF) != zs::ent_snmp_state::NONE)
      return 2;

    puts("- offline");
  }

  return 0;
}
