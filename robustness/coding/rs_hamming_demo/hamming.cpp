#include "hamming.h"
/*
 * =====================================================================================
 *
 *       Filename:  hamming code plus parity bit 64 72
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  04/07/2011 01:48:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Cesar Prados Boda (cp), c.prados@gsi.de
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 *
 * =====================================================================================*/


#define NUM_CHECK_BITS 7 // plus parity bit
#define BIT_MASK       0x1
#define SIZE           8 // 8 words 64 bits



unsigned int pbits0[NUM_CHECK_BITS][SIZE] ={	{0x5B, 0xAD, 0xAA, 0x56, 0x55, 0x55, 0x55, 0xAB}, 		// check vector 1
												{0x6D, 0x36, 0x33, 0x99, 0x99, 0x99, 0xD9, 0x0C}, 	 	// check vector 2
												{0x8E, 0xC7, 0xC3, 0xE3, 0xE1, 0xE1, 0xE1, 0xF1}, 	 	// check vector 3
												{0xF0, 0x07, 0xFC, 0xE3, 0x1F, 0xE0, 0x1F, 0xFF},		// check vector 4
												{0x00, 0xF8, 0xFF, 0x03, 0xE0, 0xFF, 0x1F, 0x00},		// check vector 5
												{0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0x01},		// check vector 6
												{0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF}		// check vector 7
};




unsigned int hamming_decode(std::string frame,unsigned int ichunk, unsigned int chunks, unsigned int *nBytep, unsigned int *nBitP)
{

	unsigned char parity[]="";
	unsigned int  checkBitResult=0;
	unsigned result;


		for(int i=0; i<NUM_CHECK_BITS; ++i) {	// parity vector x
			int temp=0;
			for(int j=0 ; j<SIZE ; j++)		    // byte by byte the frame
			{

			temp =  ((bitset<8>(static_cast<unsigned char>(frame[ichunk + j])) & bitset<8>(pbits0[i][j])).count());

				parity[0] ^= (unsigned char)((((bitset<8>((uint8_t)(frame[ichunk + j])) & bitset<8>(pbits0[i][j])).count() & 0x1 ?
							 1 : 0)
							 << (i+1)));
			}
		}

		// Parity bit, XOR of all the bits and the party bits
		unsigned numBits=0;

		for(int j=0 ; j<SIZE ; j++)		 // byte by byte the frame and the parity vector
				{
					numBits += (bitset<8>(static_cast<unsigned char>(frame[ichunk+j])).count());
				}
		parity[0] ^= (unsigned char)(numBits & 0x1 ?
						  1 : 0); // Postion 0 is for the parity bit */

		unsigned int parityBit = 0;

		if((parity[0] & 0x1) == (static_cast<unsigned char>(frame[chunks]) & 0x1))
			parityBit = 1;		// No error or double error

		checkBitResult = 0;
		for(int i=1;i<NUM_CHECK_BITS;i++)
			checkBitResult ^= (((parity[0] >> (i)) ^ (static_cast<unsigned char >(frame[chunks]) >> (i))) & (BIT_MASK))<< (i-1);

		if((checkBitResult!=0) && (parityBit == 1 ))
		{
			result = 2; 	// Double Error;
		}
		else if(checkBitResult != 0 )// single error, we can repair it
		{
			checkBitResult = postionInfraeme(checkBitResult);
			unsigned int nByte = checkBitResult / SIZE;
			unsigned int nBit = checkBitResult % SIZE;
			*nBitP = nBit;
			*nBytep = nByte;

			//cout << "************ERROR IN BYTE " << nByte << " BIT " << nBit << "************" << endl;

			result = 1 ;
		}
		else // No errors
		{
			result = 0;
		}

	return result ;
}

int postionInfraeme(int postion)
{
	int postionDiff;

	if((postion >4) && (postion <=7)) postionDiff = postion - 3;
	else if((postion >8)&& (postion <=15)) postionDiff = postion - 4;
	else if((postion >16)&& (postion <=31)) postionDiff = postion - 5;
	else if(postion >32) postionDiff = postion - 6;

	// the error is in the parity checks


	return postionDiff;
}

std::string hamming_encode(const  char *frame)
{

unsigned char parity[]="";



	for(int i=0; i<NUM_CHECK_BITS; ++i) {// parity vector x
		for(int j=0 ; j<SIZE ; j++)		 // byte by byte the frame and the parity vector
		{

			parity[0] ^= (unsigned char)((((bitset<8>(frame[j]) & bitset<8>(pbits0[i][j])).count() & 0x1 ?
					  1 : 0)
					  << (i+1)));

		}
	}
	// Parity bit, XOR of all the bits and the party bits
	int numBits=0;

	for(int j=0 ; j<SIZE ; j++)		 // byte by byte the frame and the parity vector
			{
				numBits += (bitset<8>(frame[j]).count());
			}

	parity[0] ^= (unsigned char)(numBits & 0x1 ?
					  1 : 0); // Postion 0 is for the parity bit */


	string eFrame;
	eFrame.resize(1);

	if(parity[0]== 0)     // if the parity check is 0
	    eFrame[0] = 0; // set to 0000000

	else eFrame[0]= parity[0]; // in the last byte parity word


	return eFrame;
}


