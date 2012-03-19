/*
-------------------------------------------------------------------------------
-- Title      : Routing Table Unit Common (for SIM & HW) Functions 
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : rtu_common.c
-- Authors    : Maciej Lipinski (maciej.lipinski@cern.ch)
-- Company    : CERN BE-CO-HT
-- Created    : 2010-06-30
-- Last update: 2010-06-30
-- Description: This file contain functions which interface H/W

*/
#include <stdio.h>
#include <inttypes.h>
#include "rtu_sim.h"



#define POLYNOMIAL  SIM_POLYNOMIAL_CCITT 

/*
   ================================================================================
				    HASH COMPUTATION
   ================================================================================
*/
/*

==================================================================================
hash function presented below, produces hash which correspondes to hash produced 
by VHDL generated on this page :generated with: http://outputlogic.com/
==================================================================================
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
-- To get the hex representation of POLY to be used with the C function, which is different
-- than hex representation of polly used to generate VHDL code (i.e.:0x1021 <->0x88108), 
-- here is the trick:
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
--



*/

uint16_t 
crcNaive16_2(uint16_t const message, uint16_t const init_crc)
{
    uint32_t  remainder;	
    int bit;

    /*
     * Initially, the dividend is the remainder.
     */
    
    remainder = message^init_crc;// (message << 4) | 0x000;
    //remainder = 0xFF ^ message;

    /*
     * For each bit position in the message....
     */
    for (bit = 20; bit > 0; --bit)
    {
        /*
         * If the uppermost bit is a 1...
         */
        if (remainder & 0x80000)
        {
            /*
             * XOR the previous remainder with the divisor.
             */
	    if(CFG.hash_poly == SIM_POLYNOMIAL_DECT || CFG.hash_poly == SIM_POLYNOMIAL_CCITT || CFG.hash_poly == SIM_POLYNOMIAL_IBM)
              remainder ^= CFG.hash_poly;
	    else
	      remainder ^= POLYNOMIAL;
        }

        /*
         * Shift the next bit of the message into the remainder.
         */
        remainder = (remainder << 1);
    }

    /*
     * Return only the relevant bits of the remainder as CRC.
     */
    return (remainder >> 4);

    return remainder;
}   /* crcNaive() */

uint16_t
hash(uint16_t mac_hi, uint32_t mac_lo, uint8_t fid)
{
    
  uint16_t tmp;
  
  tmp = crcNaive16_2((0x0000 | fid)            ,0xFFFF);
  tmp = crcNaive16_2(mac_hi                    ,tmp);
  tmp = crcNaive16_2((0xFFFF & (mac_lo >> 16 )),tmp);
  tmp = crcNaive16_2((0xFFFF & mac_lo)         ,tmp);

  return (0x7FF & tmp);
}

uint32_t zbt_addr_get(/*int bank,*/ int bucket, uint16_t mac_hi, uint32_t mac_lo, uint8_t fid)
{
    uint16_t tmp;
    tmp = hash(mac_hi, mac_lo, fid);
    
    uint32_t ret = 0xFFFF & ((tmp << 5) | ((0x3 & bucket) << 3) | 0x0);
    
    return ret;
/*    
    if(bank)
      return ret;
    else
      return (0xFFFF & ret); 
*/  
}
