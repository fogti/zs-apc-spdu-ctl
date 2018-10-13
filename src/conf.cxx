/** zs-apc-spdu-ctl conf.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <functional>
#include "conf.hpp"

using namespace std;

bool zs::config::read_from(const string &file) {
  static const unordered_map<string, function<void (config_ent *entdat, const string &)>> _entfns = {
#define ENTFN(KW,FN) { #KW, [](config_ent *entdat, const string &l) FN },
    ENTFN(apc, { entdat->apc = l; })
    ENTFN(host, { entdat->host = l; })
    ENTFN(outlets, {
      string outlet;
      istringstream ss(l);
      while(getline(ss, outlet, ' ')) {
        if(outlet.empty()) continue;
        entdat->outlets.emplace_back(stoi(outlet));
      }
    })
#undef ENTFN
  };

  ifstream in(file.c_str());
  if(!in) {
    fprintf(stderr, "%s: config file not found\n", file.c_str());
    return false;
  }

  string line, entname = "GLOBAL";
  config_ent *entdat = nullptr;
  bool err;

  while(getline(in, line)) {
    if(line.empty() || line.front() == '#') continue;
    err = false;

    if(line.front() == ':') {
      entname = line.substr(1);
      entdat = &ents[entname];
    } else if(entdat) {
      // entry settings
      const auto sppos = line.find_first_of(' ');
      err = (sppos == string::npos);
      if(!err) {
        const auto it = _entfns.find(line.substr(0, sppos));
        if(it == _entfns.end()) err = true;
        else it->second(entdat, line.substr(sppos + 1));
      }
    } else {
      // global settings
      if(line.substr(0, 4) == "apc ")
        apc = line.substr(4);
      else
        err = true;
    }
    if(err) fprintf(stderr, "%s: CONFIG ERROR @ %s: invalid line: %s\n", file.c_str(), entname.c_str(), line.c_str());
  }

  if(ents.empty()) {
    fprintf(stderr, "%s: CONFIG ERROR: no entries found\n", file.c_str());
    return false;
  }

  err = false;
  for(const auto &i : ents) {
    const auto chk_missing_field = [&](const char *fieldname, const auto &fieldcont) {
      if(fieldcont.empty()) {
        fprintf(stderr, "%s: CONFIG ERROR @ %s: no %s given\n", file.c_str(), i.first.c_str(), fieldname);
        err = true;
      }
    };
    chk_missing_field("host", i.second.host);
    if(apc.empty())
      chk_missing_field("APC SPDU", i.second.apc);
    chk_missing_field("outlets", i.second.outlets);
  }

  return !err;
}

auto zs::config::get_apc_of(const config_ent* const ent) -> const char * {
  if(ent && !ent->apc.empty()) return ent->apc.c_str();
  return apc.c_str();
}
