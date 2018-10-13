/** zs-apc-spdu-ctl args.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include <stdio.h>
#include <utility>
#include <config.h>
#include "args.hpp"

using namespace std;

static void print_usage() {
  fprintf(stderr, "USAGE: zs-apc-spdu-ctl [--conf CONFIGFILE] ACTION OBJECT\n");
}

bool zs::parse_args(const int argc, char *argv[], args_data &args) {
  args.conffile = "/etc/zs-apc-spdu.conf";

  if(argc == 2 && zs::string_view(argv[1]) == "--help") {
    print_usage();
    printf("\n"
      "  --conf CONFIGFILE  [default = %s]  use a different configuration\n"
      "\n"
      "actions:\n"
      "  status             print combined outlet status + is-online (via fping)\n"
      "  status-detail      print all associated outlets separately (instead of combined on/off/partial)\n"
      "  switch-on          foreach assoc outlet = ON + wait for is-online\n"
      "  switch-off         wait for is-offline + foreach assoc outlet = OFF\n"
      "\n"
      "config syntax:\n"
      "  # global scope\n"
      "  apc MASTER-APC     (optional) specify the default used APC\n"
      "\n"
      "  # local scope\n"
      "  :OBJECT            specify the OBJECT name\n"
      "  apc APC            (optional if GLOBAL apc is set) specify an APC for this OBJECT only\n"
      "  outlets 1 4 7      specify the associated outlets\n"
      "  host HOST          specify the ping'ed host\n"
      "\n"
      "(C) 2018 Erik Zscheile\n"
      , args.conffile.c_str());
    return false;
  }

  if(argc < 3) {
    print_usage();
    return false;
  }

  const size_t opts_argc = argc - 2;
  for(size_t i = 1; i < opts_argc; ++i) {
    zs::string_view cur = argv[i];
    if(cur == "--conf") {
      if((i + 1) == opts_argc) {
        fprintf(stderr, "zs-apc-spdu-ctl: missing argument for --conf\n");
        return false;
      }
      args.conffile = argv[i + 1];
      ++i;
    } else {
      fprintf(stderr, "zs-apc-spdu-ctl: unknown option: %s\n", string(move(cur)).c_str());
      return false;
    }
  }

  args.action = argv[opts_argc];
  args.ent = argv[opts_argc + 1];

  if(args.action.empty() || args.ent.empty()) {
    fprintf(stderr, "zs-apc-spdu-ctl: got empty action or object\n");
    return false;
  }
  return true;
}
