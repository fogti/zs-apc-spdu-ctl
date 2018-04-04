/** zs-apc-spdu-ctl zs_snmp.cxx
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#include <stdio.h>
#include <string.h>

// include snmp.h after zs_snmp.hpp
//  because NETSNMP_IMPORT must be defined before include snmp.h
#include "zs_snmp.hpp"
#include <net-snmp/library/snmp.h>
#include <net-snmp/library/asn1.h>
#include <net-snmp/library/system.h>

using namespace std;

zs::snmp::snmp() noexcept: _sess(nullptr) {
  SOCK_STARTUP;
}

zs::snmp::~snmp() {
  if(_sess) {
    snmp_close(_sess);
    _sess = nullptr;
  }
  SOCK_CLEANUP;
}

auto zs::snmp::handle_vars2err(netsnmp_variable_list *vars) noexcept -> const char * {
  if(vars->next_variable) return "unexpected multi response";
  switch(vars->type) {
    case SNMP_NOSUCHOBJECT:   return "no such object";
    case SNMP_NOSUCHINSTANCE: return "no such instance";
    case SNMP_ENDOFMIBVIEW:   return "EOF (no data)";
    default: return 0;
  }
}

bool zs::snmp::init(const char * const peer, const char * const community) noexcept {
  if(_sess) snmp_close(_sess);

  netsnmp_session tmpsess;
  snmp_sess_init(&tmpsess);
  tmpsess.version       = SNMP_VERSION_1;

  // XXX: the following casts are ugly,
  // but we can guarantee that the strings aren't modified
  tmpsess.peername      = const_cast<char*>(peer);
  tmpsess.community     = reinterpret_cast<uint8_t*>(const_cast<char*>(community));
  tmpsess.community_len = strlen(community);

  _sess = snmp_open(&tmpsess);
  if(!_sess) snmp_sess_perror("zs::snmp::init", &tmpsess);
  return _sess;
}

bool zs::snmp::get_stat(vector<bool> &st) {
  oid name[MAX_OID_LEN];
  size_t namlen = MAX_OID_LEN;

  // hardcoded APC SPDU status OID
  if(!snmp_parse_oid(".1.3.6.1.4.1.318.1.1.4.2.2.0", name, &namlen)) {
    snmp_perror("zs::snmp::get_stat");
    return false;
  }

  netsnmp_pdu *pdu = snmp_pdu_create(SNMP_MSG_GET), *response;
  snmp_add_null_var(pdu, name, namlen);

  bool ret = true;

  switch(snmp_synch_response(_sess, pdu, &response)) {
    case STAT_TIMEOUT:
      fprintf(stderr, "zs::snmp::get_stat: timeout @ peer %s\n", _sess->peername);
      ret = false;
      break;

    case STAT_SUCCESS:
      if(response->errstat == SNMP_ERR_NOERROR) {
        const auto vars = response->variables;

        const char *err = handle_vars2err(vars);
        if(err)
          { /* do nothing */ }
        else if(vars->type != ASN_OCTET_STR)
          err = "wrong var type (expected OCTET STRING)";
        else if(vars->val_len == 0)
          err = "got empty string";
        else if(!parse_stat(string(reinterpret_cast<char*>(vars->val.string),
                                   vars->val_len), st))
          err = "got invalid status";

        if(err) {
          fprintf(stderr, "zs::snmp::get_stat: %s @ peer %s\n", err, _sess->peername);
          fprint_variable(stderr, vars->name, vars->name_length, vars);
        }
        ret = !static_cast<bool>(err);
      } else {
        fprintf(stderr, "zs::snmp::get_stat: error in packet @ peer %s: %s\n",
                _sess->peername, snmp_errstring(response->errstat));
        ret = false;
      }
      break;

    default:
      snmp_sess_perror("zs::snmp::get_stat", _sess);
      ret = false;
  }

  if(response) snmp_free_pdu(response);
  return ret;
}

bool zs::snmp::set_outlet(uint8_t outlet, uint8_t val) {
  oid name[MAX_OID_LEN];
  size_t namlen = MAX_OID_LEN;

  const string tmp_oid = ".1.3.6.1.4.1.318.1.1.4.4.2.1.3."
    + to_string(static_cast<uint32_t>(outlet));

  if(!snmp_parse_oid(tmp_oid.c_str(), name, &namlen)) {
    snmp_perror("zs::snmp::set_outlet");
    return false;
  }

  netsnmp_pdu *pdu = snmp_pdu_create(SNMP_MSG_SET), *response;
  long ltmp = val;
  snmp_pdu_add_variable(pdu, name, namlen, ASN_INTEGER, &ltmp, sizeof(ltmp));

  bool ret = false;

  switch(snmp_synch_response(_sess, pdu, &response)) {
    case STAT_TIMEOUT:
      fprintf(stderr, "zs::snmp::set_outlet: timeout @ peer %s\n", _sess->peername);
      break;

    case STAT_SUCCESS:
      if(response->errstat == SNMP_ERR_NOERROR) {
        const auto vars = response->variables;
        const char *st = handle_vars2err(vars);
        if(!st) {
          if(vars->type != ASN_INTEGER)
            st = "wrong var type (expected INTEGER)";
          else {
            st = "OK";
            ret = true;
          }
        }
        fprintf(stderr, "zs::snmp::set_outlet: %s @ peer %s\n", st, _sess->peername);

        if(vars->type == ASN_INTEGER) {
          fprintf(stderr, "zs::snmp::set_outlet: %s:%u:%u = %ld\n", _sess->peername,
                  static_cast<uint32_t>(outlet), static_cast<uint32_t>(val),
                  *vars->val.integer);
        } else {
          fprint_variable(stderr, vars->name, vars->name_length, vars);
        }
      } else {
        fprintf(stderr, "zs::snmp::set_outlet: error in packet @ peer %s: %s\n",
                _sess->peername, snmp_errstring(response->errstat));
      }
      break;

    default:
      snmp_sess_perror("zs::snmp::set_outlet", _sess);
  }

  if(response) snmp_free_pdu(response);
  return ret;
}
