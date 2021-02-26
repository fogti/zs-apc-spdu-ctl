/** zs-apc-spdu-ctl args.hpp
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#pragma once
#include <string>
#include <string_view>

namespace zs {

struct args_data {
  std::string conffile;
  std::string_view action, ent;
};

/* parse_args
 * @param argc       argument count
 * @param argv       argument values
 * @param args (out) parsed, structured arguments
 * @return parsing success?
 */
bool parse_args(const int argc, char *argv[], args_data &args);

}
