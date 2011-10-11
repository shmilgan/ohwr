/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: OID manipulation routines
 *
 * Fixes:
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
#ifndef __WHITERABBIT_SNMP_UTILS_H
#define __WHITERABBIT_SNMP_UTILS_H

#include <stdio.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "mac.h"

#define DEFAULT_COMPONENT_ID 1

#define _LOG_ERR(...)  snmp_log(LOG_ERR,   MIB_MOD ": " __VA_ARGS__)
#define _LOG_DBG(...)  snmp_log(LOG_DEBUG, MIB_MOD ": " __VA_ARGS__)
#define _LOG_INF(...)  snmp_log(LOG_INFO,  MIB_MOD ": " __VA_ARGS__)

void update_oid(
    netsnmp_request_info           *req,
    netsnmp_handler_registration   *reginfo,
    int                            column,
    netsnmp_variable_list          *indexes);

/**
 * \brief Helper function to convert mac address into a string
 * (without strdup!)
 */
static inline char *mac_to_str(uint8_t mac[ETH_ALEN])
{
 	static char str[40];
    snprintf(str, 40, "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return str;
}

#endif /*__WHITERABBIT_SNMP_UTILS_H*/
