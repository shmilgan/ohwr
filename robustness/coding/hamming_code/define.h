/*
 * =============================================================================
 *
 *       Filename:  define.h
 *
 *    Description:  define
 *
 *        Version:  1.0
 *        Created:  03/24/2011 02:13:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Cesar Prados Boda (cp), c.prados@gsi.de
 *        Company:  GSI
 *
 * ==============================================================================*/

//#define PARIiTY_CHECK_1 0x55B // 10101011011
//#define PARITY_CHECK_2 0x66D // 11001101101
//#define PARITY_CHECK_4 0x78E // 11110001110
//#define PARITY_CHECK_8 0x7F0 // 11111110000

#define BITS_VECTOR 32
#define LOG_2_32    5
#define MOD_BIT_32  31

//parity check vectors for 64 bits words, 
#define PARITY_CHECK_1  0xfdfff80e  
#define PARITY_CHECK_2  0x56aaaff4
#define PARITY_CHECK_4  0x1222266f
#define PARITY_CHECK_8  0x202060e
#define PARITY_CHECK_16 0x2000600 
#define PARITY_CHECK_32 0xfdffffff
#define PARITY_CHECK_64 0xfe000000

#define RATE_3_1         2
#define RATE_7_4         3
#define RATE_15_11       4
#define RATE_31_26       5
#define RATE_63_57       6
#define RATE_127_120     7
#define RATE_255_247     8
#define RATE_511_502     9
#define RATE_1023_1013  10
#define RATE_2047_2036  11 
#define RATE_4095_4083  12
#define RATE_8191_8178  13
#define RATE_12000_11986 14

//TO DO define the rest of the legnth

#define FRAME_4     1
#define FRAME_12000 375


// In a payload of 1500
#define MAX_LENGTH_PAYLOAD 12000
#define READ_DFILTER 6
#define READ_DVECTOR 5
#define READ_FILTER 4
#define READ_VECTOR 3
#define CLEAN       2
#define WRITE       1
#define READ        0

#define ERROR   1
#define OK      0


// will be 21 encoded words with 7 parity check bits
// thus, 1344 bits are information 147 bit parity check bits
#define CODE_WORD 64           
#define ENCODED_WORD 71

#define BIT_MASK       0x1 

#define NUM_PARITY_BITS 4

#ifdef  DBG
    #define dbg(x, ...) fprintf(stderr, "(debug) " x, ##__VA_ARGS__)
#else
    #define dbg(x, ...) 
#endif



