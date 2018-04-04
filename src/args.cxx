/** zs-apc-spdu-ctl args.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include <stdio.h>
#include <utility>
#include <config.h>
#include "args.hpp"

using namespace std;

bool zs::parse_args(const int argc, char *argv[], args_data &args) {
  args.conffile = "/etc/zs-apc-spdu.conf";

  if(argc < 3) {
    fprintf(stderr, "USAGE: zs-apc-spdu-ctl [--conf CONFIGFILE] ACTION OBJECT\n");
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
