/** zs-apc-spdu-ctl conf.hpp
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#pragma once
#include <inttypes.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace zs {

struct config_ent {
  std::string apc, host;
  std::vector<uint8_t> outlets;
};

struct config {
  std::string apc;
  std::unordered_map<std::string, config_ent> ents;

  bool read_from(const std::string &file);
};

}
