/** zs-apc-spdu-ctl zs_snmp_support.c
    (C) 2018 Alain Zscheile
    License: MIT

    License WARNING: parts of this file come from 'net-snmp',
      look at docs/COPYING.net-snmp
 **/

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

// include these first
// NETSNMP_IMPORT must be defined before include snmp.h
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/types.h>

#include <net-snmp/library/snmp.h>
#include <net-snmp/library/asn1.h>
#include <net-snmp/library/system.h>

typedef struct zs_snmp_sessdata {
    netsnmp_session * sess;
    char *peer, *community;
} zs_snmp_sessdata;

void zs_snmp_init(void) {
  SOCK_STARTUP;
}

void zs_snmp_cleanup(void) {
  SOCK_CLEANUP;
}

static const char * handle_vars2err(const netsnmp_variable_list *vars) {
  if(vars->next_variable) return "unexpected multi response";
  switch(vars->type) {
    case SNMP_NOSUCHOBJECT:   return "no such object";
    case SNMP_NOSUCHINSTANCE: return "no such instance";
    case SNMP_ENDOFMIBVIEW:   return "EOF (no data)";
    default: return 0;
  }
}

void zs_snmp_close(zs_snmp_sessdata * sd) {
  if(!sd)
    return;
  if(sd->sess)
    snmp_close(sd->sess);
  if(sd->peer)
    free(sd->peer);
  if(sd->community)
    free(sd->community);

  free(sd);
}

// prereq: zs_snmp_init() was already called, and zs_snmp_cleanup() wasn't called since.
zs_snmp_sessdata * zs_snmp_open(const char * const peer, const char * const community) {
  zs_snmp_sessdata * sd = (zs_snmp_sessdata *) malloc(sizeof(zs_snmp_sessdata));
  if(!sd) return 0;

  netsnmp_session tmpsess;
  snmp_sess_init(&tmpsess);
  tmpsess.version       = SNMP_VERSION_1;

  // XXX: the following casts are ugly,
  // but we can guarantee that the strings aren't modified
  // to be really sure, we just create a copy
  sd->peer              = strdup(peer);
  sd->community         = strdup(community);
  tmpsess.peername      = sd->peer;
  tmpsess.community     = (uint8_t*)(sd->community);
  tmpsess.community_len = strlen(community);

  sd->sess = snmp_open(&tmpsess);
  if(!sd->sess) {
    snmp_sess_perror("zs_snmp_open", &tmpsess);
    zs_snmp_close(sd);
    sd = 0;
  }
  return sd;
}

void zs_snmp_free(char * s) {
  free(s);
}

bool zs_snmp_get_stat(zs_snmp_sessdata * sd, char ** ststr, size_t * stslen) {
  oid name[MAX_OID_LEN];
  size_t namlen = MAX_OID_LEN;

  // hardcoded APC SPDU status OID
  if(!snmp_parse_oid(".1.3.6.1.4.1.318.1.1.4.2.2.0", name, &namlen)) {
    snmp_perror("zs_snmp_get_stat");
    return false;
  }

  netsnmp_pdu *pdu = snmp_pdu_create(SNMP_MSG_GET), *response;
  snmp_add_null_var(pdu, name, namlen);

  bool ret = false;

  switch(snmp_synch_response(sd->sess, pdu, &response)) {
    case STAT_TIMEOUT:
      fprintf(stderr, "zs_snmp_get_stat: timeout @ peer %s\n", sd->sess->peername);
      break;

    case STAT_SUCCESS:
      if(response->errstat == SNMP_ERR_NOERROR) {
        const netsnmp_variable_list *vars = response->variables;

        const char *err = handle_vars2err(vars);
        if(err)
          { /* do nothing */ }
        else if(vars->type != ASN_OCTET_STR)
          err = "wrong var type (expected OCTET STRING)";
        else if(vars->val_len == 0)
          err = "got empty string";
        else {
          *stslen = vars->val_len;
          *ststr = malloc(*stslen + 1);
          if(!ststr) {
            err = "unable to allocate buffer for status";
          } else {
            // string with
            memcpy(*ststr, vars->val.string, *stslen);
            ststr[0][vars->val_len] = 0;
            ret = true;
          }
          // !parse_stat(string(reinterpret_cast<char*>(vars->val.string), vars->val_len), st)
        }

        if(err) {
          fprintf(stderr, "zs_snmp_get_stat: %s @ peer %s\n", err, sd->sess->peername);
          fprint_variable(stderr, vars->name, vars->name_length, vars);
        }
      } else {
        fprintf(stderr, "zs_snmp_get_stat: error in packet @ peer %s: %s\n",
                sd->sess->peername, snmp_errstring(response->errstat));
      }
      break;

    default:
      snmp_sess_perror("zs_snmp_get_stat", sd->sess);
  }

  if(response) snmp_free_pdu(response);
  return ret;
}

bool zs_snmp_set_outlet(zs_snmp_sessdata * sd, uint8_t outlet, uint8_t val) {
  oid name[MAX_OID_LEN];
  size_t namlen = MAX_OID_LEN;

  {
    char tmp_oid[34] = ".1.3.6.1.4.1.318.1.1.4.4.2.1.3.";
    if(outlet < 10) {
      tmp_oid[31] = '0' + outlet;
      tmp_oid[32] = 0;
    } else {
      tmp_oid[31] = '0' + (outlet / 10);
      tmp_oid[32] = '0' + (outlet % 10);
    }
    tmp_oid[33] = 0;

    if(!snmp_parse_oid(tmp_oid, name, &namlen)) {
      snmp_perror("zs_snmp_set_outlet");
      return false;
    }
  }

  netsnmp_pdu *pdu = snmp_pdu_create(SNMP_MSG_SET), *response;
  long ltmp = val;
  snmp_pdu_add_variable(pdu, name, namlen, ASN_INTEGER, &ltmp, sizeof(ltmp));

  bool ret = false;

  switch(snmp_synch_response(sd->sess, pdu, &response)) {
    case STAT_TIMEOUT:
      fprintf(stderr, "zs_snmp_set_outlet: timeout @ peer %s\n", sd->sess->peername);
      break;

    case STAT_SUCCESS:
      if(response->errstat == SNMP_ERR_NOERROR) {
        const netsnmp_variable_list *vars = response->variables;
        const char *st = handle_vars2err(vars);
        const bool is_int = (vars->type == ASN_INTEGER);
        if(!st) {
          if(!is_int)
            st = "wrong var type (expected INTEGER)";
          else {
            st = "OK";
            ret = true;
          }
        }
        fprintf(stderr, "zs_snmp_set_outlet: %s @ peer %s\n", st, sd->sess->peername);

        if(is_int) {
          fprintf(stderr, "zs_snmp_set_outlet: %s:%u:%u = %ld\n", sd->sess->peername,
                  (uint32_t)(outlet), (uint32_t)(val),
                  *vars->val.integer);
        } else {
          fprint_variable(stderr, vars->name, vars->name_length, vars);
        }
      } else {
        fprintf(stderr, "zs_snmp_set_outlet: error in packet @ peer %s: %s\n",
                sd->sess->peername, snmp_errstring(response->errstat));
      }
      break;

    default:
      snmp_sess_perror("zs_snmp_set_outlet", sd->sess);
  }

  if(response) snmp_free_pdu(response);
  return ret;
}
