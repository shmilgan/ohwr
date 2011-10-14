/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
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


#ifndef __WHITERABBIT_CLI_SNMP_H
#define __WHITERABBIT_CLI_SNMP_H

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

int cli_snmp_init(char *username, char *password);

int cli_snmp_int(netsnmp_variable_list *vars);
char *cli_snmp_string(netsnmp_variable_list *vars);

int cli_snmp_get_int(oid _oid[MAX_OID_LEN], size_t oid_len);
char *cli_snmp_get_string(oid _oid[MAX_OID_LEN], size_t oid_len);

netsnmp_pdu *cli_snmp_getnext(oid _oid[MAX_OID_LEN]);

int cli_snmp_getnext_int(oid _oid[MAX_OID_LEN], size_t *oid_len);
char *cli_snmp_getnext_string(oid _oid[MAX_OID_LEN], size_t *oid_len);

void cli_snmp_set_int(oid _oid[MAX_OID_LEN], char *val, char type);
void cli_snmp_set_str(oid _oid[MAX_OID_LEN], char *val);

void cli_snmp_close();


#endif /* __WHITERABBIT_CLI_SNMP_H */
