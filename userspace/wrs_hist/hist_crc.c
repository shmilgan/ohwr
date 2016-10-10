/*
 * Content of this file is based on the code taken from the website:
 * http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
 */

/**********************************************************************
 *
 * Filename:    crc.c
 *
 * Description: Fast implementation of the CRC standards.
 *
 * Notes:       The parameters for each supported CRC standard are
 *		defined in the header file crc.h.  The implementations
 *		here should stand up to further additions to that list.
 *
 *
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#include "hist_crc.h"

#define WIDTH  (8 * sizeof(uint8_t))
#define TOPBIT (1 << (WIDTH - 1))
#define POLYNOMIAL HIST_POLYNOMIAL_8_1W

static uint8_t crcTable[256];

void crc_init(void)
{
	uint8_t  remainder;
	int dividend;
	uint8_t bit;

	/* Compute the remainder of each possible dividend. */
	for (dividend = 0; dividend < 256; ++dividend) {
		/* Start with the dividend followed by zeros. */
		remainder = dividend << (WIDTH - 8);

		/* Perform modulo-2 division, a bit at a time. */
		for (bit = 8; bit > 0; --bit) {
			/* Try to divide the current data bit. */
			if (remainder & TOPBIT) {
				remainder = (remainder << 1) ^ POLYNOMIAL;
			} else {
				remainder = (remainder << 1);
			}
		}

		/* Store the result into the table. */
		crcTable[dividend] = remainder;
	}
}

uint8_t crc_fast(uint8_t const message[], int nBytes)
{
	uint8_t data;
	uint8_t remainder = 0;
	int byte;

	/* Divide the message by the polynomial, a byte at a time. */
	for (byte = 0; byte < nBytes; ++byte) {
		data = message[byte] ^ (remainder >> (WIDTH - 8));
		remainder = crcTable[data] ^ (remainder << 8);
	}

	/* The final remainder is the CRC. */
	return remainder;
}   /* crcFast() */