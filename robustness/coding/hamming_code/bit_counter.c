
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
/*
 * =====================================================================================
 *
 *       Filename:  bit_counter.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  18.03.2011 21:44:37
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#define TWO(c)     (0x1u << (c))
#define MASK(c) (((unsigned int)(-1)) / (TWO(TWO(c)) + 1u))
#define COUNT(x,c) ((x) & MASK(c)) + (((x) >> (TWO(c))) & MASK(c))

int bitcount(unsigned int );
int bitcount_1(unsigned int );
int bitcount_2(int);
int bitcount_3(unsigned int);


int bitcount (unsigned int n)  {
            n = COUNT(n, 1);
            n = COUNT(n, 1);
            n = COUNT(n, 2);
            n = COUNT(n, 3);
            n = COUNT(n, 4);
                       
           /*  n = COUNT(n, 5) ;    for 64-bit integers */
                           
           return n ;                  
}


int bitcount_1 (unsigned int n)   {
    int count = 8 * sizeof(int);

    n ^= (unsigned int) - 1;
    
    while (n)
    {
        count--;
        n &= (n - 1);
    }
    return count ;
}

int bitcount_2(int n)
{

unsigned int uCount;

       uCount = n 
         - ((n >> 1) & 033333333333)
         - ((n >> 2) & 011111111111);
       return (int) (((uCount + (uCount >> 3))& 030707070707) % 63);
       
}

int bitcount_3(unsigned int v)
{
    unsigned int c=0;

    while(v > 0)

//    for (c = 0; v; v >>= 1)
    {   
        c++;
        v = v  >> 1;
    }
    return c;
}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
    int
main ( int argc, char *argv[] )
{
   int number_bits;

    number_bits = bitcount_3(00000000000000);
    printf("Numero de bits %d \n",number_bits);
    return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
