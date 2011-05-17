#include "fec.h"
#include "hamming.h"
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "crc.c"


//#define DEMO__ // makes a binary for the demo


#ifdef DEMO_PRINT__
	#define  dbg(x, ...) fprintf(stderr, "(debug) " x, ##__VA_ARGS__)
#else
	#define  dbg(x, ...)
#endif

#define K 2 // Configuration parmamiter
#define MAX_output_messages    1024

extern void RS_code(unsigned int fragLen, std::vector<const unsigned char*>& fragments);

/*
 * ====================================================================================
 *
 *       Filename:  fec.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/07/2011 12:58:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Cesar Prados Boda (cp), c.prados@gsi.de
 *         			Wesley Terpstra   		w.terpstra@gsi.de
 *        Company:  GSI
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 * =====================================================================================
 */

static uint32_t outgoing_msgID = 0;

void fec_open()
{
    srand(time(0));
    outgoing_msgID = rand();
}

void fec_close()
{
}

static unsigned int fec_chopchop(unsigned int msize) {
    // !!! Do something intelligent!
    return 2;
}

struct State 
{
    unsigned int msize;
    unsigned int divsize;
    unsigned int fragments_received;
    std::vector<std::string> received;
};

typedef std::map<uint32_t, State> Map;
typedef std::queue<uint32_t> Queue;

static Map cache;
static Queue inCache;

static std::string result;
const unsigned char* fec_decode(const unsigned char* chunk, unsigned int* len)
{
	unsigned int length = *len;
    unsigned int chunks = length / 9;
    if (length % 9 != 0 || length < 18) return 0;


    std::string eChunk(reinterpret_cast<const char*>(chunk));

    unsigned short crc1 = crcSlow(reinterpret_cast<const unsigned char*>(eChunk.substr(8,8).c_str()),8);

#ifdef DEMO__
	cout << "************************START DECODING CONTROL MESSAGE************************ " << endl;

    system("sleep 1");
#endif
    for (unsigned int i = 0; i < chunks; ++i) {


#ifdef DEMO__
    	 cout << endl;
    	 cout << "---------------------ENCODED HAMMING WORD RX-----------------" <<  endl;
    	 cout << "------------------------------------------------------" <<  endl;

    	 cout << "ENCODED MESSAGE " << eChunk.substr(i*8,8) << endl;
    	 cout << "PARITY BITS " << eChunk.substr((chunks*8)+i,8) << endl;
#endif

    	 unsigned int nByte;
    	 unsigned int nBit;

    	int decodeResult = hamming_decode(eChunk, i*8, (chunks*8)+i,&nByte,&nBit); //c


    	if(decodeResult == 0)  cout << "NO ERRORS" << endl;
    	else if(decodeResult == 1) // One error o more than 2 errors
    	{
    		// CRC after fix the bit
    		unsigned short crc2 = crcSlow(reinterpret_cast<const unsigned char*>(eChunk.substr(12,8).c_str()),8);

    		if(crc1 == crc2) // One error and is going to be fixed
    		{
    			eChunk[i*8+nByte] ^= (char)(0x1 << (nBit-1)); // THe bit is fixed

				#ifdef DEMO__
    			cout << "FIXED MESSAGE " << eChunk.substr(i*8,8) << endl;
    			cout << "CRC RX " << crc1 << " CRC FIXED " << crc2 << endl;
				#endif
    		}
    		else // More than 2 errros
    		{
				#ifdef DEMO__
    			cout << "FIXED MESSAGE ??????????" <<  endl;
    		    system("sleep 1");
    		    cout << eChunk.substr(i*8,8) << endl;
    		    cout << "I DON'T THINK SO..." <<  endl;
    		    cout << "CRC ORIGINAL " << crc1 << " CRC RX " << crc2  << endl;
    			 cout << "MORE THAN TWO ERROR" << endl;
    		     cout << "************************NO CORRECTION FRAME LOST************************ " << endl<< endl;
				#endif
    		     return 0;
    		}
    	}	// Two errors!!!
    	else if(decodeResult == 2){
    		 cout << "DOUBLE ERROR" << endl;
    		 cout << "************************NO CORRECTION FRAME LOST************************ " << endl<< endl;
    		return 0;
    	}


    	 cout << "-----------------------------------------------------" <<  endl;
#ifdef DEMO__
    system("sleep 0.5");
#endif

    }
#ifdef DEMO__
    cout << "************************END DECODING CONTROL MESSAGE************************ " << endl;
#endif


    uint64_t header = 0;
    for (int i = 0; i < 8; ++i) 
    {
        header <<= 8;
        header |= static_cast<uint8_t> (eChunk[i]);

    }

    uint32_t mID         = (header >> 32) & 0xFFFFFFFF;
    unsigned int msize   = (header >> 20) & 0xFFF;
    unsigned int fragLen = (header >>  8) & 0xFFF;
    unsigned int index   = (header >>  0) & 0xFF;

    unsigned int output_messages = fec_chopchop(msize);
    unsigned int divsize = (msize+output_messages-1) / output_messages;
    unsigned int rsinsize = (divsize+7) & ~7;

    if (fragLen != length) return 0;
    if (fragLen != (rsinsize+8)/8*9) return 0;
    if (index > output_messages+K) return 0;

    Map::iterator state = cache.find(mID);
    if (state == cache.end())
    {
        if (inCache.size() > MAX_output_messages)
        {
            uint32_t kill = inCache.front();
            inCache.pop();
            cache.erase(cache.find(kill));
        }
        inCache.push(mID);

        State& newState = cache[mID]; // add it
        state = cache.find(mID);

        newState.msize = msize;
        newState.fragments_received = 0;
        newState.divsize = (msize + output_messages-1) / output_messages;
        newState.received.resize(output_messages + K);
    }

    if (state->second.msize != msize) return 0; // Doesn't fit with other packets
    if (!state->second.received[index].empty()) return 0; // Duplicated packet

    // Grab the data portion of the buffer
    state->second.received[index] = eChunk.substr(8, (chunks-1)*8);
    //state->second.received[index] = std::string(reinterpret_cast<const char*>(chunk)+8, (chunks-1)*8);

    //If we don't have enough packets yet (or already decode), stop now
    if (++state->second.fragments_received != output_messages) return 0;

    // DECODING TIME!
    std::vector<const unsigned char*> fragments;
    fragments.resize(state->second.received.size());
    for (unsigned int i = 0; i < fragments.size(); ++i)
        if (state->second.received[i].empty())
            fragments[i] = 0;
        else
            fragments[i] = reinterpret_cast<const unsigned char*>(state->second.received[i].data());

    // Do the work
    RS_code(divsize, fragments);

    // Reassemble the packet
    result.clear();
    result.reserve(divsize*output_messages);
    for (unsigned int i = 0; i < output_messages; ++i)
        for (unsigned int j = 0; j < divsize; ++j)
            result.push_back(fragments[i][j]);

    result.resize(msize); // clip the padding and done

    *len = result.size();
    return reinterpret_cast<const unsigned char*>(result.data());
}
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------

static std::vector<std::string> output_messages;

static void fec_setup(const unsigned char* chunk, unsigned int len)
{
    unsigned int msize = len;
    output_messages.clear();
    output_messages.resize(fec_chopchop(msize));

    unsigned int divsize = (msize+output_messages.size()-1) / output_messages.size();
    unsigned int rsinsize = (divsize+7) & ~7;

    // Make a -id
    uint32_t msgID = ++outgoing_msgID;

    std::string buf(reinterpret_cast<const char*>(chunk), msize);
    buf.resize(divsize*output_messages.size()); // Pad with 0 to a multiple of output_messages

    std::vector<const unsigned char*> fragments;
    fragments.reserve(output_messages.size()+K);

    for (unsigned int i = 0; i < output_messages.size(); ++i)
        fragments.push_back(reinterpret_cast<const unsigned char*>(buf.data() + i*divsize));
    for (unsigned int i = 0; i < K; ++i)
        fragments.push_back(0);

    // Do the actual RS-encoding
    RS_code(divsize, fragments);

    output_messages.resize(output_messages.size() + K);

#ifdef DEMO__
     cout << "************************START ENCODING CONTROL MESSAGE************************ " << endl;
#endif

    for (unsigned int i = 0; i < output_messages.size(); ++i)
    {
        std::string msg(8, 'x');

        unsigned int fragLen = (rsinsize + 8)/8*9;
        uint64_t header =
            ((uint64_t)msgID   << 32) |
            ((uint64_t)msize   << 20) |
            ((uint64_t)fragLen <<  8) |
            ((uint64_t)i       <<  0);

        for (int j = 7; j > 0; --j) {
            uint8_t low = header & 0xFF;
            header >>= 8;
            msg[j] = static_cast<char>(low);
         }

        msg += std::string(reinterpret_cast<const char*>(fragments[i]), divsize);
        msg.resize(rsinsize + 8); // Pad with 0 to a multiple of 8


        unsigned int chunks = msg.size() / 8; // SEC-DED(72,64) is 8 bytes at once
#ifdef DEMO__
     	 cout << "---------------------CHUNK " << i << "--------------------------" <<  endl;
    	 cout << "-----------------------------------------------------" <<  endl;
#endif

        for (unsigned int j = 0; j < chunks; ++j)
        {


#ifdef DEMO__
        	 cout << "          STRING TO ENCODE " << msg.substr(j*8,8) << endl;
        	 cout << "-----------------------------------------------------" <<  endl;
#endif
        	std::string parityBits = hamming_encode(msg.substr(j*8,8).c_str());

           	msg.push_back(parityBits[0]); // !!! result of hamming code



#ifdef DEMO__
    system("sleep 0.5");
#endif

        	parityBits.clear();

        }



        output_messages[i] = msg;
#ifdef DEMO__
     cout << "-----------------------------------------------------" <<  endl;
   	 cout << "         HAMMING WORD ENCODED " << msg << endl;
   	 cout << "-----------------------------------------------------" <<  endl;

    system("sleep 1");
#endif

    }
#ifdef DEMO__
     cout << "************************END ENCODING CONTROL MESSAGE************************ " << endl;
    cout << endl << endl << endl;

     cout << "........................TRANSMISSION........................... " << endl;
     cout << "........................TRANSMISSION........................... " << endl;
     cout << "........................TRANSMISSION........................... " << endl;
     cout << "........................TRANSMISSION........................... " << endl;
     cout << "........................TRANSMISSION........................... " << endl;
     cout << "........................TRANSMISSION........................... " << endl;
     cout << "........................TRANSMISSION........................... " << endl;


    cout << endl << endl << endl;
#endif
}

const unsigned char* fec_encode(const unsigned char* chunk, unsigned int* len, int index)  
{
	if (index == 0) fec_setup(chunk, *len);
    if (index == (int)output_messages.size()) return 0;

    *len = output_messages[index].size();
    return reinterpret_cast<const unsigned char*>(output_messages[index].data());
}

