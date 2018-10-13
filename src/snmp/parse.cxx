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
      case ' ':
        if(i == 'O')     break;
      case 0: // 'f' 2
      case 'n':
        if(i != ' ')     return false;
        if(state != ' ') out.emplace_back(state);
        break;

      case 'O':
        // the following checks for 'n' & 'f'
        if((i | 0x8) != 0x6e)
          return false;
        break;

      case 'f':
        if(i != 'f') return false;
        state = 0;
        continue;

      default: return false;
    }
    state = i;
  }

  return (state == ' ');
}
