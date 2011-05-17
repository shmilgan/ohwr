/*
 * cmdline.h
 *
 *  Created on: 07-Jul-2009
 *      Author: poliveir
 */

#ifndef CMDLINE_H_
#define CMDLINE_H_


// constants/macros/typdefs
typedef void (*CmdlineFuncPtrType)(void);

#define ASCII_CR				0x0D

// size of command database
// (maximum number of commands the cmdline system can handle)
#define CMDLINE_MAX_COMMANDS	6

// maximum length (number of characters) of each command string
// (quantity must include one additional byte for a null terminator)
#define CMDLINE_MAX_CMD_LENGTH	15

uint8_t* cmdlineGetArgStr(uint8_t argnum);
long cmdlineGetArgInt(uint8_t argnum);
void cmdlineAddCommand(char* newCmdString, CmdlineFuncPtrType newCmdFuncPtr);

void helpFunction(void);
void set_available_cmd(uint8_t value);
uint8_t get_available_cmd(void);
void cmdlineInputFunc(unsigned char c);
void cmdlineProcessInputString(void);
void cmdlinePrintPrompt(void);
void cmdlinePrintError(void);
void cmdlineSetOutputFunc(void (*output_func)(unsigned char c));
void cmdlineMainLoop(void);

#endif /* CMDLINE_H_ */
