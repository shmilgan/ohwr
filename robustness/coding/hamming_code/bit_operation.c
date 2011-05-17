/*
 * =====================================================================================
 *
 *       Filename:  bit_operation.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2011 03:20:53 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Cesar Prados Boda (cp), c.prados@gsi.de
 *        Company:  GSI
 *
 * =====================================================================================
 */


#include "bit_operation.h"
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
