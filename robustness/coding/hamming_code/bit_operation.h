/*
 * =====================================================================================
 *
 *       Filename:  bit_operation.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2011 03:16:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Cesar Prados Boda (cp), c.prados@gsi.de
 *        Company:  GSI
 *
 * =====================================================================================
 */



#include <ctype.h>

#define TWO(c)     (0x1u << (c))
#define MASK(c) (((unsigned int)(-1)) / (TWO(TWO(c)) + 1u))
#define COUNT(x,c) ((x) & MASK(c)) + (((x) >> (TWO(c))) & MASK(c))

int bitcount(unsigned int );
int bitcount_1(unsigned int );
int bitcount_2(int);
