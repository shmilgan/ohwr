/****************************************************************************/
/*																			*/
/*	Module:			mb_io.h	(MicroBlaster)									*/	
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

#ifndef INC_MB_IO_H
#define INC_MB_IO_H

#define EMBEDDED	2


/*///////////////////////*/
/* Functions Prototyping */
/*///////////////////////*/

int		ReadByteBlaster		( int port );
void	WriteByteBlaster	( int port, int data, int test );

#define FPGA_MAIN 0
#define FPGA_CLKB 1

#endif /* INC_MB_IO_H */
