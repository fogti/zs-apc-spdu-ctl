/** zs-apc-spdu-ctl confent.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include "conf.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netdb.h>

#include <chrono>
#include <thread>
#include <vector>

#include <zs/ll/string/csarray.hpp>

using namespace std;

namespace zs {

config_ent::config_ent() noexcept {
  _cached_ipv6[1] = _cached_ipv6[0] = false;
}

bool config_ent::host_is_ipv6() noexcept {
  if(!_cached_ipv6[0]) {
    struct addrinfo hint, *res = nullptr;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = PF_UNSPEC;
    hint.ai_flags = AI_NUMERICHOST;

    if(getaddrinfo(host.c_str(), nullptr, &hint, &res))
      return false;

    _cached_ipv6[0] = true;
    _cached_ipv6[1] = (res->ai_family == AF_INET6);
    freeaddrinfo(res);
  }

  return _cached_ipv6[1];
}

static void prepare_fping_args(vector<string> &args, const bool use_ipv6, const string &host, const vector<string> &extra_args) {
  args.clear();
  args.reserve(3 + extra_args.size());
  args.emplace_back("/usr/sbin/fping");
  if(use_ipv6) args.back().push_back('6');
  args.emplace_back("-q");
  args.insert(args.end(), extra_args.begin(), extra_args.end());
  args.emplace_back(host);
  args.shrink_to_fit();
}

static int spawn_prog(const vector<string> &args) {
  const pid_t pid = fork();

  switch(pid) {
    case -1:
      perror("fork()");
      return 127;

    case 0:
      // child process
      {
        llzs::CStringArray csa(args);
        execvp(args.front().c_str(), csa.data());
        exit(127);
      }

    default: break;
  }

  // parent process
  int status;
  while(-1 == waitpid(pid, &status, 0)) ;
  return status;
}

bool config_ent::host_is_online() {
  vector<string> args;
  prepare_fping_args(args, host_is_ipv6(), host, {});
  const int ret = spawn_prog(args);
  return WIFEXITED(ret) && WEXITSTATUS(ret) == 0;
}

bool config_ent::wait_for_host(const bool online) {
  vector<string> args;

  if(online) {
    // wait for online max ~8,5 minutes
    prepare_fping_args(args, host_is_ipv6(), host, {"-r", "10"});
    const int ret = spawn_prog(args);
    return WIFEXITED(ret) && WEXITSTATUS(ret) == 0;
  }

  prepare_fping_args(args, host_is_ipv6(), host, {});

  // wait for offline, but max 2 minutes
  const auto start = chrono::steady_clock::now();

  while(true) {
    const int ret = spawn_prog(args);
    if(!WIFEXITED(ret) || WEXITSTATUS(ret) == 127)
      return false;
    if(WEXITSTATUS(ret) != 0)
      break;

    const auto remaining = 120000 - chrono::duration_cast<chrono::milliseconds>(
                                    chrono::steady_clock::now() - start).count();
    if(remaining <= 0) return false;
    this_thread::sleep_for(chrono::milliseconds(remaining / 10 + 10));
  }

  return true;
}

}
