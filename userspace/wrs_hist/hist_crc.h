/*
 * White Rabbit switch history handling, CRC calculations
 * Copyright (C) 2016, CERN.
 *
 * Authors:     Adam Wujek (CERN)
 *
 * Description: Header for functions to calculate CRC.
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
#ifndef __HIST_CRC_H__
#define __HIST_CRC_H__

#include <inttypes.h>

/*
  Crc polynomials
  ---------------------------------------------------------
  |        name   |      poly equation      | poly (hex)  |
  ---------------------------------------------------------
  |  CRC-8-CCITT  | 1+x^1+x^2+x^8           |    0x07     |
  |  CRC-8-1Wire  | 1+x^4+x^5+x^8           |    0x31     |
  ---------------------------------------------------------
*/
#define HIST_POLYNOMIAL_8_CCITT      0x07
#define HIST_POLYNOMIAL_8_1W         0x31


void crc_init(void);
uint8_t crc_fast(uint8_t const message[], int nBytes);


#endif /*__HIST_CRC_H__*/
