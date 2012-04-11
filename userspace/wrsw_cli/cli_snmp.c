/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: SNMP utility functions to simplify handling SNMP stuff from CLI
 *              modules.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cli_snmp.h"

#define LOCALHOST "127.0.0.1"

static netsnmp_session *session;


/**
 * Initializes SNMP operational parameters at the client side.
 * @param username authentication string associated to the principal.
 * @param password must be at least 8 characters long.
 * @return 0 if snmp session was opened. -1 in case of error.
 */
int cli_snmp_init(char *username, char *password)
{
    netsnmp_session sess;

    init_snmp("cli");

    snmp_sess_init( &sess );

    sess.peername = LOCALHOST;
    sess.version = SNMP_VERSION_3;
    sess.securityName = username;
    sess.securityNameLen = strlen(sess.securityName);
    // the security level is authenticated, but not encrypted,
    // since local access does not require it
    sess.securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;
    sess.securityAuthProto = usmHMACMD5AuthProtocol;
    sess.securityAuthProtoLen = sizeof(usmHMACMD5AuthProtocol)/sizeof(oid);
    sess.securityAuthKeyLen = USM_AUTH_KU_LEN;

    if (generate_Ku(sess.securityAuthProto,
                    sess.securityAuthProtoLen,
                    (u_char *) password, strlen(password),
                    sess.securityAuthKey,
                    &sess.securityAuthKeyLen) != SNMPERR_SUCCESS) {
        snmp_log(LOG_ERR, "Error generating Ku from password.\n");
        return -1;
    }

    session = snmp_open(&sess);
    if (!session) {
        snmp_sess_perror("cli: SNMP", &sess);
        return -1;
    }

    return 0;
}

/**
 * The callee is responsible for freeing the PDU response returned from
 * this method
 */
static netsnmp_pdu *cli_snmp_get(oid _oid[MAX_OID_LEN], size_t oid_len)
{
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;
    int status;

    pdu = snmp_pdu_create(SNMP_MSG_GET);
    snmp_add_null_var(pdu, _oid, oid_len);

    status = snmp_synch_response(session, pdu, &response);

    errno = EPROTO;
    if (status == STAT_SUCCESS) {
        if (response->errstat != SNMP_ERR_NOERROR)
            fprintf(stderr, "Error in SNMP response: %s\n",
                    snmp_errstring(response->errstat));
        else
            errno = 0;
    } else if (status == STAT_TIMEOUT) {
        fprintf(stderr, "SNMP Timeout: No Response from %s\n",
                session->peername);
    } else { /* status == STAT_ERROR */
        snmp_sess_perror("cli: SNMP", session);
    }

    return response;
}

/**
 * The callee is responsible for freeing the PDU response returned from
 * this method
 */
netsnmp_pdu *cli_snmp_getnext(oid _oid[MAX_OID_LEN], size_t *oid_len)
{
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;
    int status;

    pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
    pdu->max_repetitions = 1;
    pdu->non_repeaters = 0;
    snmp_add_null_var(pdu, _oid, *oid_len);

    status = snmp_synch_response(session, pdu, &response);

    errno = EPROTO;
    if (status == STAT_SUCCESS) {
        if (response->errstat != SNMP_ERR_NOERROR)
            fprintf(stderr, "Error in SNMP response: %s\n",
                    snmp_errstring(response->errstat));
        else
            errno = 0;
    } else if (status == STAT_TIMEOUT) {
        fprintf(stderr, "SNMP Timeout: No Response from %s\n",
                session->peername);
    } else { /* status == STAT_ERROR */
        snmp_sess_perror("cli: SNMP", session);
    }

    return response;
}

int cli_snmp_int(netsnmp_variable_list *vars)
{
    if (vars) {
        switch(vars->type) {
        case ASN_TIMETICKS:
        case ASN_GAUGE:
        case ASN_COUNTER:
        case ASN_INTEGER:
            return *(vars->val).integer;
        default:
            errno = EINVAL;
            fprintf(stderr, "The value type is not an integer. Error %d: %s\n",
                    errno, strerror(errno));
        }
    }
    return 0;
}

uint64_t cli_snmp_counter(netsnmp_variable_list *vars)
{
    uint32_t high;
    uint32_t low;

    if (vars) {
        switch(vars->type) {
        case ASN_COUNTER64:
            high = vars->val.counter64->high;
            low  = vars->val.counter64->low;
            return ((uint64_t)high << 32) | low;
        default:
            errno = EINVAL;
            fprintf(stderr, "The value type is not a counter64. Error %d: %s\n",
                    errno, strerror(errno));
        }
    }
    return 0;
}

char *cli_snmp_string(netsnmp_variable_list *vars)
{
    char *sp;

    if (vars) {
        switch(vars->type) {
        case ASN_OCTET_STR:
            sp = malloc(1 + vars->val_len);
            if(sp != NULL) {
                memcpy(sp, vars->val.string, vars->val_len);
                sp[vars->val_len] = '\0';
                return sp;
            }
        default:
            errno = EINVAL;
            fprintf(stderr, "The value type is not a string. Error %d: %s\n",
                    errno, strerror(errno));
	    }
    }
    return NULL;
}

int cli_snmp_get_int(oid _oid[MAX_OID_LEN], size_t oid_len)
{
    netsnmp_pdu *response;
    int value = 0, err = 0;

    response = cli_snmp_get(_oid, oid_len);
    if (response) {
        if(response->errstat == SNMP_ERR_NOERROR)
            value = cli_snmp_int(response->variables);
        err = errno;
        snmp_free_pdu(response);
        errno = err;
    }

    return value;
}

uint64_t cli_snmp_get_counter(oid _oid[MAX_OID_LEN], size_t oid_len)
{
    netsnmp_pdu *response;
    uint64_t value = 0;
    int err = 0;

    response = cli_snmp_get(_oid, oid_len);
    if (response) {
        if(response->errstat == SNMP_ERR_NOERROR)
            value = cli_snmp_counter(response->variables);
        err = errno;
        snmp_free_pdu(response);
        errno = err;
    }

    return value;
}

/**
 * The callee is responsible for freeing memory for returned value.
 */
char *cli_snmp_get_string(oid _oid[MAX_OID_LEN], size_t oid_len)
{
    netsnmp_pdu *response;
    char *value = NULL;
    int err = 0;

    response = cli_snmp_get(_oid, oid_len);
    if (response) {
        if(response->errstat == SNMP_ERR_NOERROR)
            value = cli_snmp_string(response->variables);
        err = errno;
        snmp_free_pdu(response);
        errno = err;
	}

	return value;
}

int cli_snmp_getnext_int(oid _oid[MAX_OID_LEN], size_t *oid_len)
{
    netsnmp_pdu *response;
    int err = 0, value = 0;
    netsnmp_variable_list *vars;

    response = cli_snmp_getnext(_oid, oid_len);
    if (response) {
        if(response->errstat == SNMP_ERR_NOERROR) {
            vars = response->variables;
            memcpy(_oid, vars->name, vars->name_length * sizeof(oid));
            *oid_len = vars->name_length;
            value = cli_snmp_int(vars);
        }
        err = errno;
        snmp_free_pdu(response);
        errno = err;
    }

    return value;
}

char *cli_snmp_getnext_string(oid _oid[MAX_OID_LEN], size_t *oid_len)
{
    netsnmp_pdu *response;
    char *value = NULL;
    int err = 0;
    netsnmp_variable_list *vars;

    response = cli_snmp_getnext(_oid, oid_len);
    if (response) {
        if(response->errstat == SNMP_ERR_NOERROR) {
            vars = response->variables;
            memcpy(_oid, vars->name, vars->name_length * sizeof(oid));
            *oid_len = vars->name_length;
            value = cli_snmp_string(response->variables);
        }
        err = errno;
        snmp_free_pdu(response);
        errno = err;
    }

    return value;
}

void cli_snmp_set(oid _oid[][MAX_OID_LEN],
                  size_t oid_len[],
                  char *val[],
                  char type[],
                  int nvars)
{
    netsnmp_pdu *pdu;
    netsnmp_pdu *response = NULL;
    int status, i, ret;

    pdu = snmp_pdu_create(SNMP_MSG_SET);

    for (i = 0; i < nvars; i++) {
        if ((ret = snmp_add_var(pdu, _oid[i], oid_len[i], type[i], val[i]))) {
            fprintf(stderr, "Error %d: %s\n", ret, snmp_errstring(ret));
            return;
        }
    }

    status = snmp_synch_response(session, pdu, &response);

    if (status == STAT_SUCCESS) {
        if (response->errstat != SNMP_ERR_NOERROR)
            fprintf(stderr, "Error in SNMP response: %s\n",
                    snmp_errstring(response->errstat));
    } else if (status == STAT_TIMEOUT) {
        fprintf(stderr, "SNMP Timeout: No Response from %s\n",
                session->peername);
    } else { /* status == STAT_ERROR */
        snmp_sess_perror("cli: SNMP", session);
    }

    if (response)
        snmp_free_pdu(response);
}

/* The cli_snmp_set function may handle all the SET operations, including this
   one. However, we maintain this special case (which is the most used) in order
   to keep the callers simpler. */
void cli_snmp_set_int(oid _oid[MAX_OID_LEN], size_t oid_len, char *val, char type)
{
    oid __oid[1][MAX_OID_LEN];
    size_t _oid_len[1]  = {oid_len};
    char *_val[1]       = {val};
    char _type[1]       = {type};

    memcpy(__oid[0], _oid, MAX_OID_LEN * sizeof(oid));

    cli_snmp_set(__oid, _oid_len, _val, _type, 1);
}

/**
 * Closes SNMP session.
 */
void cli_snmp_close()
{
    snmp_close(session);
}
