/*
 * shw_ver.c
 *
 * Obtain the HW version and FPGA type.
 *
 *  Created on: Jan 20, 2012
 *  Authors:
 * 		- Benoit RAT
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License...
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <shw_io.h>
#include "pio.h"


#ifndef __GIT_VER__
#define __GIT_VER__ "x.x"
#endif

#ifndef __GIT_USR__
#define __GIT_USR__ "?"
#endif


void help(const char* pgrname)
{
	printf("usage: %s <command>\n", pgrname);
	printf("available commands are:\n"
				"   -p PCB version\n"
				"   -f FPGA size\n"
				"   -c Compiling time\n"
				"   -v version (git)\n"
				"   -a All (default)\n"
				"\n");
		exit(1);
}


int main(int argc, char **argv)
{
	char func='a';
	if(argc>=2 && argv[1][0]=='-')
	{
		func=argv[1][1];
	}
	else func='a';

    assert_init(shw_pio_mmap_init());
	shw_io_init();

	shw_io_t* all_io = (shw_io_t*)_all_shw_io;


	switch(func)
	{
	case 'p':
		printf("%s\n",get_shw_info(func));
		break;
	case 'f':
		printf("%s\n",get_shw_info(func));
		break;
	case 'c':
		printf("%s %s\n",__DATE__,__TIME__);
		 break;
	case 'v':
		printf("%s %s\n",__GIT_VER__,__GIT_USR__);
	case 'a':
		printf("PCB:%s, FPGA:%s; version: %s (%s); compiled at %s %s\n",
				get_shw_info('p'),get_shw_info('f'),__GIT_VER__,__GIT_USR__,__DATE__,__TIME__);
		break;
	case 'h':
	default:
		help(argv[0]);
		return 1;
	}
	return 0;
}
