/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: Hash function presented below, produces hash which corresponds 
 *              to hash produced by VHDL generated on this page: 
 *              http://outputlogic.com/
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
#ifndef __WHITERABBIT_RTU_HASH_H
#define __WHITERABBIT_RTU_HASH_H

#include "mac.h"

/*
  Hash polynomials implemented by RTU
  ---------------------------------------------------------
  |        name   |      poly equation      | poly (hex)  | 
  ---------------------------------------------------------
  |  CRC-16-CCITT | 1+x^5+x^12+x^16         |    0x1021   |
  |  CRC-16-IBM   | 1+x^2+x^15+x^16         |    0x8005   |
  |  CRC-16-DECT  | 1+x^3+x^7+x^8+x^10+x^16 |    0x0589   |
  ---------------------------------------------------------
*/
#define HW_POLYNOMIAL_CCITT         0x1021 
#define HW_POLYNOMIAL_IBM           0x8005 
#define HW_POLYNOMIAL_DECT          0x0589 

void rtu_hash_set_poly(uint16_t poly);
uint16_t rtu_hash(uint8_t mac[ETH_ALEN], uint8_t fid);

#endif /*__WHITERABBIT_RTU_HASH_H*/

