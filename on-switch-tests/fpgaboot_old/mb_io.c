/****************************************************************************/
/*																			*/
/*	Module:			mb_io.c	(MicroBlaster)									*/	
/*																			*/
/*					Copyright (C) Altera Corporation 2004					*/
/*																			*/
/*	Descriptions:	Defines all IO control functions. operating system		*/
/*					is defined here. Functions are operating system 		*/
/*					dependant.												*/
/*																			*/
/*	Revisions:		1.0	12/10/01 Sang Beng Ng								*/
/*					Supports Altera ByteBlaster hardware download cable		*/
/*					on Windows NT.											*/
/*					1.1	05/28/04 Chuin Tein Ong								*/
/*					Supports both Altera ByteBlaster II and ByteBlaster MV  */
/*					download cables	on Windows NT.							*/
/*																			*/
/****************************************************************************/

#include "mb_io.h"
#include <stdio.h>


int fopen_rbf( char argv[], char* mode)
{
	FILE* file_id;

	file_id = fopen( argv, mode );

	return (int) file_id;
}

int	fclose_rbf( int file_id)
{
	fclose( (FILE*) file_id);

	return 0;
}

int fseek_rbf( int finputid, int start, int end )
{
	int seek_position;

	seek_position = fseek( (FILE*) finputid, start, end );

	return seek_position;
}

int ftell_rbf( int finputid )
{
	int file_size;

	file_size = ftell( (FILE*) finputid );

	return file_size;
}

int fgetc_rbf( int finputid )
{
	int one_byte;

	one_byte = fgetc( (FILE*) finputid );

	return one_byte;
}

void delay ( int factor)
{
	int i;
	for (i=0;i<factor;i++) asm volatile("nop");
}