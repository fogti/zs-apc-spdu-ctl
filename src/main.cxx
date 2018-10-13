/** zs-apc-spdu-ctl main.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include <stdio.h>
#include <future>
#include <thread>
#include <chrono>
#include <functional>
#include <unordered_map>

#include "args.hpp"
#include "conf.hpp"
#include "snmp/zs_snmp.hpp"

using namespace std;

struct actdat_t {
  bool need_priv;
  function<bool ()> fn;

  actdat_t()
    : need_priv(false) { }

  actdat_t(bool np_, const decltype(fn) &fn_)
    : need_priv(np_), fn(fn_) { }

  actdat_t(const actdat_t &o) = default;
};

int main(int argc, char *argv[]) {
  zs::args_data args;
  if(!zs::parse_args(argc, argv, args))
    return 1;
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

  zs::snmp my_snmp;
  const unordered_map<string, actdat_t> jt = {
    { "status", { false, [&] {
      future<bool> is_online = async(launch::async, [&] { return selent->host_is_online(); });

      printf("OUTLETS: ");
      vector<bool> st;
      if(!my_snmp.get_stat(st))
        puts("unknown");
      else {
        switch(selent->get_outlets(st)) {
          case zs::ent_snmp_state::DONE:
            puts("on"); break;
          case zs::ent_snmp_state::PARTIAL:
            puts("partial on"); break;
          default: puts("off");
        }

        for(const auto i : selent->outlets) {
          if(!i || (i - 1) > st.size()) continue;
          printf("  %u = %d\n", static_cast<uint32_t>(i), st[i - 1] ? 1 : 0);
        }
      }

      printf("NETWORK: %sline\n", is_online.get() ? "on" : "off");
      return true;
    }}},
    { "switch-on", { true, [&] {
      if(selent->set_outlets(my_snmp, ZS_SNMP_ON) != zs::ent_snmp_state::DONE)
        return false;

      printf("waiting for server to start ... ");
      if(!selent->wait_for_host(true)) {
        puts("failed");
        return false;
      }

      puts("online");
      return true;
    }}},
    { "switch-off", { true, [&] {
      printf("waiting for server to stop ...\n");
      if(!selent->wait_for_host(false)) {
        puts("- failed");
        return false;
      }
      fflush(stdout);

      // graceful shutdown
      this_thread::sleep_for(chrono::seconds(1));

      if(selent->set_outlets(my_snmp, ZS_SNMP_OFF) != zs::ent_snmp_state::NONE) {
        puts("- partial");
        return false;
      }

      puts("- offline");
      return true;
    }}},
  };

  actdat_t actiondata;
  {
    const auto actit = jt.find(args.action);
    if(actit == jt.end()) {
      fprintf(stderr, "%s: UNKNOWN ACTION\n", args.action.c_str());
      return 1;
    }
    actiondata = actit->second;
  }

  if(!my_snmp.init(config.get_apc_of(selent), actiondata.need_priv ? "private" : "public"))
    return 2;

  if(!actiondata.fn())
    return 2;

  return 0;
}
