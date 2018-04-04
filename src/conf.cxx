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

  string line, entname;
  config_ent *entdat = nullptr;

  while(getline(in, line)) {
    if(line.empty() || line.front() == '#') continue;

    if(line.front() == ':') {
      entname = line.substr(1);
      entdat = &ents[entname];
    } else if(entdat) {
      // entry settings
      const auto sppos = line.find_first_of(' ');
      bool err = (sppos == string::npos);
      if(!err) {
        const auto it = _entfns.find(line.substr(0, sppos));
        if(it == _entfns.end()) err = true;
        else it->second(entdat, line.substr(sppos));
      }
      if(err) fprintf(stderr, "%s: CONFIG ERROR @ %s: invalid line: %s\n", file.c_str(), entname.c_str(), line.c_str());
    } else {
      // global settings
      if(line.substr(0, 4) == "apc ")
        apc = line.substr(4);
      else
        fprintf(stderr, "%s: CONFIG ERROR @ GLOBAL: invalid line: %s\n", file.c_str(), line.c_str());
    }
  }

  if(ents.empty()) {
    fprintf(stderr, "%s: CONFIG ERROR: no entries found\n", file.c_str());
    return false;
  }

  bool err = false;
  for(const auto &i : ents) {
    if(i.second.host.empty()) {
      fprintf(stderr, "%s: CONFIG ERROR @ %s: no host given\n", file.c_str(), i.first.c_str());
      err = true;
    }
    if(i.second.apc.empty() && apc.empty()) {
      fprintf(stderr, "%s: CONFIG ERROR @ %s: no APC SPDU given\n", file.c_str(), i.first.c_str());
      err = true;
    }
    if(i.second.outlets.empty()) {
      fprintf(stderr, "%s: CONFIG ERROR @ %s: no outlets given\n", file.c_str(), i.first.c_str());
      err = true;
    }
  }

  return !err;
}
