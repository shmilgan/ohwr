/*
 * cmdline.c
 *
 *  Created on: 07-Jul-2009
 *      Author: poliveir
 */
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include "stdio.h"
#include "cmdline.h"
#include "string.h"
#include "uart.h"



uint8_t PROGMEM CmdlinePrompt[] = "cmd>";
uint8_t PROGMEM CmdlineNotice[] = "cmdline: ";
uint8_t PROGMEM CmdlineCmdNotFound[] = "command not found";

static char CmdlineCommandList[CMDLINE_MAX_COMMANDS][CMDLINE_MAX_CMD_LENGTH];
static CmdlineFuncPtrType CmdlineFunctionList[CMDLINE_MAX_COMMANDS];

static uint8_t available_cmd;

CmdlineFuncPtrType CmdlineExecFunction;

#define CMDLINE_BUFFERSIZE 15

char CmdlineBuffer[CMDLINE_BUFFERSIZE];
uint8_t CmdlineBufferLength=0;
uint8_t CmdlineBufferEditPos=0;

// number of commands currently registered
uint8_t CmdlineNumCommands;

void cmdlineInit(void)
{
	// initialize input buffer
	CmdlineBufferLength = 0;
	CmdlineBufferEditPos = 0;
	// initialize executing function
	CmdlineExecFunction = 0;
	// initialize command list
	CmdlineNumCommands = 0;


}


void set_available_cmd(uint8_t value)
{
	available_cmd= value;

}
uint8_t get_available_cmd(void)
{
	return available_cmd;

}


void cmdlineInputFunc(unsigned char c)
{
	if( (c >= 0x20) && (c < 0x7F) )
	{
		// character is printable
		// is this a simple append
		if(CmdlineBufferEditPos == CmdlineBufferLength)
		{
			// add it to the command line buffer
			CmdlineBuffer[CmdlineBufferEditPos++] = c;
			// update buffer length
			CmdlineBufferLength++;
		}
	}
	// handle special characters
	else if(c == ASCII_CR)
	{
		// user pressed [ENTER]

		// add null termination to command
		CmdlineBuffer[CmdlineBufferLength++] = 0;
		CmdlineBufferEditPos++;
		// command is complete, process it
		cmdlineProcessInputString();
		// reset buffer
		CmdlineBufferLength = 0;
		CmdlineBufferEditPos = 0;
	}
}
void cmdlineAddCommand(char* newCmdString, CmdlineFuncPtrType newCmdFuncPtr)
{
	// add command string to end of command list
	strcpy(CmdlineCommandList[CmdlineNumCommands], newCmdString);
	// add command function ptr to end of function list
	CmdlineFunctionList[CmdlineNumCommands] = newCmdFuncPtr;
	// increment number of registered commands
	CmdlineNumCommands++;
}
void cmdlineMainLoop(void)
{
	// do we have a command/function to be executed
	if(CmdlineExecFunction)
	{
		// run it
		CmdlineExecFunction();
		// reset
		CmdlineExecFunction = 0;
		// output new prompt
		cmdlinePrintPrompt();
	}
}

void cmdlineProcessInputString(void)
{
	uint8_t cmdIndex;
	uint8_t i=0;

	CmdlineExecFunction=0;
	// find the end of the command (excluding arguments)
	// find first whitespace character in CmdlineBuffer
	while( !((CmdlineBuffer[i] == ' ') || (CmdlineBuffer[i] == 0)) ) i++;

	if(!i)
	{
		// command was null or empty
		// output a new prompt
		cmdlinePrintPrompt();
		// we're done
		return;
	}

	// search command list for match with entered command
	for(cmdIndex=0; cmdIndex<CmdlineNumCommands; cmdIndex++)
	{
		if( !strncmp(CmdlineCommandList[cmdIndex], CmdlineBuffer, i) )
		{
			// user-entered command matched a command in the list (database)
			// run the corresponding function
			printf(" cmd %s found\r\n",CmdlineCommandList[cmdIndex]);
			CmdlineExecFunction = CmdlineFunctionList[cmdIndex];
			// new prompt will be output after user function runs
			// and we're done
			return;
		}
	}

	printf("cmd not found\r\n");
	// if we did not get a match
	// output an error message
	cmdlinePrintError();
	// output a new prompt
	cmdlinePrintPrompt();
}

void cmdlinePrintPrompt(void)
{
	// print a new command prompt
	uint8_t* ptr = CmdlinePrompt;
	while(pgm_read_byte(ptr)) uart_putc( pgm_read_byte(ptr++) );
}

uint8_t PROGMEM str1[] ="\r\nAvailable commands are:\r\n";
uint8_t PROGMEM str2[] ="help      - displays available commands\r\n";
uint8_t PROGMEM str3[] ="pwr %d %d %d - dev_addr channel ch_coun\r\n";
uint8_t PROGMEM str4[]= "send_fru - dumps command arguments as decimal integers\r\n";
uint8_t PROGMEM str5[] ="toogle_pwr - toogle_pwr\r\n";
uint8_t PROGMEM str6[] ="FAN %d - control fans colling module\r\n";


void helpFunction(void)
{

	uint8_t* ptr = str1;
	while(pgm_read_byte(ptr)) uart_putc( pgm_read_byte(ptr++) );
    ptr = str2;
	while(pgm_read_byte(ptr)) uart_putc( pgm_read_byte(ptr++) );
	ptr = str3;
	while(pgm_read_byte(ptr)) uart_putc( pgm_read_byte(ptr++) );
	ptr = str4;
	while(pgm_read_byte(ptr)) uart_putc( pgm_read_byte(ptr++) );

	ptr = str5;
	while(pgm_read_byte(ptr)) uart_putc( pgm_read_byte(ptr++) );

	ptr = str6;
	while(pgm_read_byte(ptr)) uart_putc( pgm_read_byte(ptr++) );



}
// function pointer to single character output routine
static void (*cmdlineOutputFunc)(unsigned char c);

void cmdlineSetOutputFunc(void (*output_func)(unsigned char c))
{
	// set new output function
	cmdlineOutputFunc = output_func;

	// should we really do this?
	// print a prompt
	//cmdlinePrintPrompt();
}

void cmdlinePrintError(void)
{
	uint8_t * ptr;

	// print a notice header
	// (u08*) cast used to avoid compiler warning
	ptr = (uint8_t*)CmdlineNotice;
	while(pgm_read_byte(ptr)) uart_putc( pgm_read_byte(ptr++) );

	// print the offending command
	ptr = (uint8_t*)CmdlineBuffer;
	while((*ptr) && (*ptr != ' ')) uart_putc(*ptr++);

	uart_putc(':');
	uart_putc(' ');

	// print the not-found message
	// (u08*) cast used to avoid compiler warning
	ptr = (uint8_t*)CmdlineCmdNotFound;
	while(pgm_read_byte(ptr)) uart_putc( pgm_read_byte(ptr++) );

	uart_putc('\r');
	uart_putc('\n');
}
// return string pointer to argument [argnum]
uint8_t* cmdlineGetArgStr(uint8_t argnum)
{
	// find the offset of argument number [argnum]
	uint8_t idx=0;
	uint8_t arg;

	// find the first non-whitespace character
	while( (CmdlineBuffer[idx] != 0) && (CmdlineBuffer[idx] == ' ')) idx++;

	// we are at the first argument
	for(arg=0; arg<argnum; arg++)
	{
		// find the next whitespace character
		while( (CmdlineBuffer[idx] != 0) && (CmdlineBuffer[idx] != ' ')) idx++;
		// find the first non-whitespace character
		while( (CmdlineBuffer[idx] != 0) && (CmdlineBuffer[idx] == ' ')) idx++;
	}
	// we are at the requested argument or the end of the buffer
	return (uint8_t*)&CmdlineBuffer[idx];
}


// return argument [argnum] interpreted as a decimal integer
long cmdlineGetArgInt(uint8_t argnum)
{
	char* endptr;
	return strtol(cmdlineGetArgStr(argnum), &endptr, 10);

}
