/** zs-apc-spdu-ctl snmp/parse.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include "zs_snmp.hpp"

/* zs::snmp::parse_stat parse an APC SPDU status value (string of On/Off sep ' ')
 * @param in   the string got from APC
 * @param out  the current outlet values
 * @return     parse success?
 */
bool zs::snmp::parse_stat(const std::string &in, std::vector<bool> &out) {
  /* parser state
     start := ' '
     0x0 'f' (2)
     'O' | 'n' | ' ' | 'f'
   */
  char state = ' ';
  out.clear();

  for(const auto i : in) {
    switch(state) {
      case 0: // 'f' 2
        if(i != ' ') return false;
        state = i;
        out.emplace_back(false);
        break;

      case ' ':
        if(i == ' ') break;
        if(i != 'O') return false;
        state = i;
        break;

      case 'O':
        switch(i) {
          case 'n': case 'f': state = i; break;
          default: return false;
        }
        break;

      case 'n':
        if(i != ' ') return false;
        state = i;
        out.emplace_back(true);
        break;

      case 'f':
        if(i != 'f') return false;
        state = 0;
        break;

      default: return false;
    }
  }

  return (state == ' ');
}
