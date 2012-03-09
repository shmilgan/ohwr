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

#include <hw/trace.h>

#include "rtu.h"
#include "rtu_hash.h"


/**
 * \brief Polynomial used to calculate the MAC entry hash code.
 */
static uint32_t hash_poly;

static uint16_t crc16(uint16_t const init_crc, uint16_t const message);

void rtu_hash_set_poly(uint16_t poly)
{
    hash_poly = ((0x10000 | poly) << 3 );
    TRACE(TRACE_INFO, "poly hw = %x, poly sw = %x\n", poly, hash_poly);
}

uint16_t rtu_hash(uint8_t mac[ETH_ALEN], uint8_t fid)
{
  uint16_t hash = 0xFFFF;
  
  hash = crc16(hash, (0xFFFF & fid));
  hash = crc16(hash, ((uint16_t)mac[0] << 8) | mac[1]);
  hash = crc16(hash, ((uint16_t)mac[2] << 8) | mac[3]);
  hash = crc16(hash, ((uint16_t)mac[4] << 8) | mac[5]);

  return hash & (HTAB_ENTRIES - 1); /* warning: assumes that HTAB_ENTRIES is a power of 2 */
}

/*
-------------------------------------------------------------------------------
-- SOME EXPLANATION REGARDING VHDL vs C  CRC IMPLEMENTATION BY ML
-------------------------------------------------------------------------------
-- C code to produce exactly the same CRC as VHDL generated 
-- with http://outputlogic.com/ .
-- it uses naive method, it's not optimal at all
-- but it's good enough to chech whether VHDL works OK
-- It was made (by maciej.lipinski@cern.ch) modifying source from here:
-- http://www.netrino.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
-- the website provides explanation of CRC
-- 
-- To get the hex representation of POLY to be used with the C function, which 
-- is different than hex representation of polly used to generate VHDL code 
-- (i.e.:0x1021 <->0x88108), here is the trick:
--
-- 1) we are using the following poly equation: 1+x^5+x^12+x^16;
-- 2) it translates to binary: (1) 0001 0000 0010 0001 = 0x1021 
--                              |                        |
--                              |   this is default      |   this you can find in 
--                              |-> it's the 16th bit    |-> the wiki as description
--                                                           of the polly equation
-- 
-- 3) we include the "default" bit into the polly and add zeroes at the end
--    creating 20 bit polly, like this
--     (1) 0001 0000 0010 0001 => 1000 1000 0001 0000 1000 = 0x88108 
--
--------------------------------------------------------------------------------
--|        name   |      polly equation     | polly (hex) | our polly | tested |
--------------------------------------------------------------------------------
--|  CRC-16-CCITT | 1+x^5+x^12+x^16         |    0x1021   |  0x88108  |  yes   |
--|  CRC-16-IBM   | 1+x^2+x^15+x^16         |    0x8005   |  0xC0028  |  yes   |
--|  CRC-16-DECT  | 1+x^3+x^7+x^8+x^10+x^16 |    0x0589   |  0x82C48  |  yes   |  
--------------------------------------------------------------------------------
*/
static uint16_t crc16(uint16_t const init_crc, uint16_t const message)
{
    uint32_t remainder;	
    int bit;

    // Initially, the dividend is the remainder.
    remainder = message^init_crc;
    // For each bit position in the message....
    for (bit = 20; bit > 0; --bit) {
        // If the uppermost bit is a 1...
        if (remainder & 0x80000) {
            // XOR the previous remainder with the divisor.
            remainder ^= hash_poly;
        }
        //Shift the next bit of the message into the remainder.
        remainder = (remainder << 1);
    }
    // Return only the relevant bits of the remainder as CRC.
    return (remainder >> 4);
}



