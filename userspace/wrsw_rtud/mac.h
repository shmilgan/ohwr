/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: MAC address type related operations.
 *
 * Fixes:
 *              Alessandro Rubini
 *              Benoit Rat
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

#ifndef __WHITERABBIT_RTU_MAC_H
#define __WHITERABBIT_RTU_MAC_H

#include <stdint.h>
#include <string.h>

#define ETH_ALEN 6
#define ETH_ALEN_STR 18

/**
 * \brief Check whether two mac addresses are equal.
 * @return 1 if both addresses are equal. 0 otherwise.
 */
static inline int mac_equal(uint8_t a[ETH_ALEN], uint8_t b[ETH_ALEN])
{
	return !memcmp(a, b, ETH_ALEN);
}

/**
 * \brief copies src mac address into dst mac address.
 * @return pointer to dst mac address
 */
static inline uint8_t *mac_copy(uint8_t dst[ETH_ALEN], uint8_t src[ETH_ALEN])
{
	return memcpy(dst, src, ETH_ALEN);
}

/**
 * \brief Set MAC address to 00:00:00:00:00:00.
 * @return pointer to mac address
 */
static inline uint8_t *mac_clean(uint8_t mac[ETH_ALEN])
{
	return memset(mac, 0x00, ETH_ALEN);
}

char *mac_to_string(uint8_t mac[ETH_ALEN]);
char *mac_to_buffer(uint8_t mac[ETH_ALEN], char buffer[ETH_ALEN_STR]);
int mac_from_str(uint8_t * tomac, const char *fromstr);

#endif /* __WHITERABBIT_RTU_MAC_H */
