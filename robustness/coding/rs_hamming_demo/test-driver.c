/*
 * =====================================================================================
 *
 *       Filename:  test-driver.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/07/2011 01:48:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Cesar Prados Boda (cp), c.prados@gsi.de
 *         			Wesley Terpstra   		w.terpstra@gsi.de
 *
 *  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 *
 * =====================================================================================
 */

#include "fec.h"
#include <stdio.h>
#include <stdint.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

//#define DEMO__ // makes a binary for the demo
int main()
{
    unsigned char msg[] =   "PAYLOAD1"
							"PAYLOAD2"
    					    "PAYLOAD3"
							"PAYLOAD4";


//    unsigned char msg[] = 	"Nel mezzo del   "
//    						"camin di nostra "
//    						"vita mi ritrovai"
//    						" per una selva  "
//    						"oscura,ché la   "
//    						"diritta via era "
//    						"smarrita........";



    int c;
    const unsigned char* buf;
    const unsigned char* cbuf;
    unsigned int i, len,len_temp;
    unsigned char *temp;


    printf("\n \n \n");
    printf("*************************************************************\n");
    printf(" WE WANT TO ENCODE: \n");
    printf("");

    	for (i = 0; i < sizeof(msg)-1; ++i)
        {

            printf("%c", msg[i]);
        }

    printf("\n");
    printf("*************************************************************\n");
    printf("\n \n \n");

#ifdef DEMO__

    system("sleep 1");

#endif



    fec_open();

    for (c = 0; len = sizeof(msg)-1, (buf = fec_encode(msg, &len, c)) != 0; ++c)
    {
        //for (i = 0; i < len; ++i)
            //printf("%c(%02x)", isprint(buf[i])?buf[i]:"x", buf[i]);
        //printf("\n");
    }
    // fflush(stdout);

    

    len = sizeof(msg)-1;
    buf = fec_encode(msg, &len, 0);
    cbuf = fec_decode(buf, &len);


    len = sizeof(msg)-1;
    buf = fec_encode(msg, &len, 1);
    cbuf = fec_decode(buf, &len);

    len = sizeof(msg)-1;

    buf = fec_encode(msg, &len, 3);




    if (cbuf != 0) {
        printf("\n \n \n");
        printf("******************************************************************\n");
        printf("!!!!!!!!!!!!!!!!!THE CONTROL MESSAGE IS DECODED!!!!!!!!!!!!!!!!!\n");
        printf("******************************************************************\n");
    	printf("");
        for (i = 0; i < len; ++i)
        {

            printf("%c", cbuf[i]);
        }
        printf("\n");
    }
    else
    {
    printf("\n \n \n");
    printf("******************************************************************\n");
    printf("!!!!!!!!!!!!!!!!!THE CONTROL MESSAGE IS NOT DECODED!!!!!!!!!!!!!!!!!\n");
    printf("******************************************************************\n");
    }


    fec_close();
    return 0;
}
