/*
 * Note: this file originally auto-generated by mib2c using
 *  : generic-table-enums.m2c 12526 2005-07-15 22:41:16Z rstory $
 *
 * $Id:$
 */
/*
 * White Rabbit SNMP
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_snmpd v1.0
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: Enumeration values depending on the objects definition.
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

#ifndef IEEE8021BRIDGEBASEPORTTABLE_ENUMS_H
#define IEEE8021BRIDGEBASEPORTTABLE_ENUMS_H

#ifdef __cplusplus
extern "C" {
#endif

 /*
 * NOTES on enums
 * ==============
 *
 * Value Mapping
 * -------------
 * If the values for your data type don't exactly match the
 * possible values defined by the mib, you should map them
 * below. For example, a boolean flag (1/0) is usually represented
 * as a TruthValue in a MIB, which maps to the values (1/2).
 *
 */
/*************************************************************************
 *************************************************************************
 *
 * enum definitions for table ieee8021BridgeBasePortTable
 *
 *************************************************************************
 *************************************************************************/

/*************************************************************
 * constants for enums for the MIB node
 * ieee8021BridgeBasePortCapabilities (BITS / ASN_OCTET_STR)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redefinitions of the enum values.
 */
#ifndef IEEE8021BRIDGEBASEPORTCAPABILITIES_ENUMS
#define IEEE8021BRIDGEBASEPORTCAPABILITIES_ENUMS

#define IEEE8021BRIDGEBASEPORTCAPABILITIES_DOT1QDOT1QTAGGING_FLAG  (1 << (7-0))
#define IEEE8021BRIDGEBASEPORTCAPABILITIES_DOT1QCONFIGURABLEACCEPTABLEFRAMETYPES_FLAG  (1 << (7-1))
#define IEEE8021BRIDGEBASEPORTCAPABILITIES_DOT1QINGRESSFILTERING_FLAG  (1 << (7-2))

#endif /* IEEE8021BRIDGEBASEPORTCAPABILITIES_ENUMS */

#define IEEE8021BRIDGEBASEPORTCAPABILITIES_FLAG  0


/*************************************************************
 * constants for enums for the MIB node
 * ieee8021BridgeBasePortTypeCapabilities (BITS / ASN_OCTET_STR)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redefinitions of the enum values.
 */
#ifndef IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_ENUMS
#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_ENUMS

#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_CUSTOMERVLANPORT_FLAG  (1 << (7-0))
#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_PROVIDERNETWORKPORT_FLAG  (1 << (7-1))
#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_CUSTOMERNETWORKPORT_FLAG  (1 << (7-2))
#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_CUSTOMEREDGEPORT_FLAG  (1 << (7-3))
#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_CUSTOMERBACKBONEPORT_FLAG  (1 << (7-4))
#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_VIRTUALINSTANCEPORT_FLAG  (1 << (7-5))
#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_DBRIDGEPORT_FLAG  (1 << (7-6))

#endif /* IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_ENUMS */

#define IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_FLAG IEEE8021BRIDGEBASEPORTTYPECAPABILITIES_DBRIDGEPORT_FLAG

/*************************************************************
 * constants for enums for the MIB node
 * ieee8021BridgeBasePortType (IEEE8021BridgePortType / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redefinitions of the enum values.
 */
#ifndef IEEE8021BRIDGEPORTTYPE_ENUMS
#define IEEE8021BRIDGEPORTTYPE_ENUMS

#define IEEE8021BRIDGEPORTTYPE_NONE  1
#define IEEE8021BRIDGEPORTTYPE_CUSTOMERVLANPORT  2
#define IEEE8021BRIDGEPORTTYPE_PROVIDERNETWORKPORT  3
#define IEEE8021BRIDGEPORTTYPE_CUSTOMERNETWORKPORT  4
#define IEEE8021BRIDGEPORTTYPE_CUSTOMEREDGEPORT  5
#define IEEE8021BRIDGEPORTTYPE_CUSTOMERBACKBONEPORT  6
#define IEEE8021BRIDGEPORTTYPE_VIRTUALINSTANCEPORT  7
#define IEEE8021BRIDGEPORTTYPE_DBRIDGEPORT  8

#endif /* IEEE8021BRIDGEPORTTYPE_ENUMS */


/*************************************************************
 * constants for enums for the MIB node
 * ieee8021BridgeBasePortExternal (TruthValue / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redefinitions of the enum values.
 */
#ifndef TRUTHVALUE_ENUMS
#define TRUTHVALUE_ENUMS

#define TRUTHVALUE_TRUE  1
#define TRUTHVALUE_FALSE  2

#endif /* TRUTHVALUE_ENUMS */


/*************************************************************
 * constants for enums for the MIB node
 * ieee8021BridgeBasePortAdminPointToPoint (INTEGER / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redefinitions of the enum values.
 */
#ifndef IEEE8021BRIDGEBASEPORTADMINPOINTTOPOINT_ENUMS
#define IEEE8021BRIDGEBASEPORTADMINPOINTTOPOINT_ENUMS

#define IEEE8021BRIDGEBASEPORTADMINPOINTTOPOINT_FORCETRUE  1
#define IEEE8021BRIDGEBASEPORTADMINPOINTTOPOINT_FORCEFALSE  2
#define IEEE8021BRIDGEBASEPORTADMINPOINTTOPOINT_AUTO  3

#endif /* IEEE8021BRIDGEBASEPORTADMINPOINTTOPOINT_ENUMS */


/*************************************************************
 * constants for enums for the MIB node
 * ieee8021BridgeBasePortOperPointToPoint (TruthValue / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redefinitions of the enum values.
 */
#ifndef TRUTHVALUE_ENUMS
#define TRUTHVALUE_ENUMS

#define TRUTHVALUE_TRUE  1
#define TRUTHVALUE_FALSE  2

#endif /* TRUTHVALUE_ENUMS */




#ifdef __cplusplus
}
#endif

#endif /* IEEE8021BRIDGEBASEPORTTABLE_ENUMS_H */
