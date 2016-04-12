/*
 * White Rabbit SPLL debug read 
 * Copyright (C) 2016, CERN.
 *
 * Authors:     Maciej Lipinski (maciej.lipinski@cern.ch)
 *              Mattia Rizzi    (mattia.rizzi@cern.ch)
 *
 * Description:
 *              This simple code opens a socket in the proxy that you have to run
 *              on the switch (/wr/bin/spll_dbg_proxy) and interprets the data that
 *              is sent from the switch.  This data are debugs from softPLL.
 *              The received data can be printed to the screen (-p) or written to the
 *              files. The app creates two files: mPLL.txt and hPLL.txt and writes in 
 *              columnts 
 *              ID,        y,      err,      tag_reference, tag_local,   event
 * 
 * Note:
 * 1) it is not fully debugged code, it is recommended to use it in two modes only:
 *   a) ./spll_debug_read -p  to print to the screen
 *   b) ./spll_debug_read -r  to write to the files: mPLL.txt and hPLL.txt
 * 2) it works with release wr-switch-sw-v4.2
 * 3) The tags coming from helper PLL are wrong due to a small bug in softpll 
 *    (affects only debug data), other data is OK, main pll is OK.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define SERVER "10.11.34.187"
#define SERVPORT 12345

#define RING_BUFFER_ENTRIES 1048576
#define ENTRIES_PER_PACKET 128

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define DBG_Y 0
#define DBG_ERR 1
#define DBG_TAG 2
#define DBG_REF 5
#define DBG_PERIOD 3
#define DBG_EVENT 4
#define DBG_SAMPLE_ID 6

#define DBG_MAIN      0x0		/* ...          : Main PLL */
#define DBG_HELPER    0x20		/* Sample source: Helper PLL */
#define DBG_EXT       0x40		/* Sample source: External Reference PLL */


#define DBG_EVT_START 1       /* PLL has just started */
#define DBG_EVT_LOCKED 2      /* PLL has just become locked */


#define TRIG_ACQ(in,nr) (((in)==(nr))?1:0)

const char *tab_content = "        ID,        y,      err,      tag_reference, tag_local,   event\n";

char what[][20] = {{"Y         : "}, // 0
                   {"ERROR     : "}, // 1
                   {"TAG_LOCAL       : "}, // 2
                   {"DBG_PERIOD  : "}, // 3
                   {"EVENT     : "}, // 4
                   {"TAG_REFERENCE : "}, // 5
                   {"SAMPLE ID : "},  // 6 
                   {"AVG SHORT : "}};// 7 

char where[][10] = {{"mPLL  : "}, // 0
                    {"hPLL  : "}, // 1
                    {"EXT   : "}, // 2
		    };

char event[][20] = {{"non       "}, // 0
                    {"Start PLL "}, // 1
                    {"Locked    "}, // 2
                    {"          "}, // 3

                    }; 

__attribute__((packed)) struct fifo_entry {
	int value;
	uint16_t seq_id;
};
#define F_ALL 0xFF;

struct pll_stat {
	uint16_t seq_id;
	int cnt;
	int sample_id;
	int setpoint;
	int tag;
	int ref;
	int y;
	int err;
	int avg_l;
	int avg_s;
	int flags;
	int printed;
	int mark_event;
	int event;
	int record;
	int locked;
};

void usage(char *name)
{
    printf("Usage: %s [options] \n"
        "for options which store data to files, the following files are created: mPLL, hPLL, bPLL{1-3}\n"
        "\t-d N Matlab-ready (no header in files) acqusition, finish N samples after event"
	                " (switchover or holdover on)\n"
         "\t-p   print all data to stdout\n"
        "\t-r   record all data to the files\n"
        "\t-m   matlab read, i.e. no headers in the files where data is stored (only if data to files)\n"
        "\t-x   debug to hex to stdout\n"
	"\t-i server IP (default is " SERVER ", port is " TOSTRING(SERVPORT) ")\n"
        "\t-?   this info\n"
        "\t-h   this info\n",
        name);
    exit(1);
}
int term_get(void)
{
	unsigned char c;
	int q;

	if(read(STDIN_FILENO, &c, 1 ) == 1)
	{
		q=c;
	} else q=-1;

	return q;
}
struct pll_stat clear_stat={-1,0,0,0,0,0,0,0,0,0,0,0,0,0};
// int convertNumber(unsigned int value) { return (unsigned char)(-value); } 

int convertNumber(unsigned int value)
{
    int bitLength = 24;
    int signedValue = value;
    if (value >> (bitLength - 1))
        signedValue |= -1 << bitLength;
    return signedValue;
}

void print_pll(struct pll_stat *s,FILE *f)
{
    if(s->record == 0) return;
    
    if((s->flags >> DBG_SAMPLE_ID) & 0x1) fprintf(f,"%10d",s->sample_id); else fprintf(f,"%10d",0);
    if((s->flags >> DBG_Y)         & 0x1) fprintf(f,"%10d",s->y);         else fprintf(f,"%10d",0);
    if((s->flags >> DBG_ERR)       & 0x1) fprintf(f,"%10d",s->err);       else fprintf(f,"%10d",0);
    if((s->flags >> DBG_TAG)       & 0x1) fprintf(f,"%10d",s->tag);       else fprintf(f,"%10d",0);
    if((s->flags >> DBG_REF)       & 0x1) fprintf(f,"%10d",s->ref);       else fprintf(f,"%10d",0);
    if((s->flags >> DBG_EVENT)     & 0x1) fprintf(f,"%10d",s->mark_event);else fprintf(f,"%10d",0);
    fprintf(f,"\n");
}

int process(struct pll_stat *s, FILE *f, uint16_t seq_id, uint32_t value, int what, int mark_event, int trig_acq, int orig)
{
    if(s->seq_id == seq_id)
    {
        s->flags = s->flags | 0x1 << what;
        s->cnt++;
    }
    else if (seq_id >0)
    {
        // TODO: seems to be a bug and some data comes with sample_id == 0 ...
        //       it should only happen for switchover events but happens in other cases
        // temp-hack: skipp unwanted entries
        if(s->seq_id > 0 && s->sample_id ==0 && s->event == 0)  
        {
           s->y = 0; s->err = 0; s->tag = 0; s->setpoint = 0; s->avg_l = 0; 
           s->avg_s = 0; s->sample_id = 0; s->event=0; s->cnt=0;
           s->flags = 0;
        }
        //merge events
        else if(s->seq_id > 0 && s->sample_id ==0 && s->event != 0)
        {
           printf("\nspecial stuff [%s]: SEQ_ID=%d [%d] | SAM_ID=%d | Y=%d | ERR=%d | EVENT=%s [%d] | CNT=%d\n",
           where[orig],s->seq_id,seq_id,s->sample_id,s->y,s->err,event[s->event],s->event,s->cnt);
           s->cnt=0;
        }
        // write only if the previous banch of data:
        // 1) has seq_id (so not the first one)
        // 2) has sample_id, if sample_id ==0, this means the data was an event, include it in 
        //    next print_pll
        else if(s->seq_id > 0 && s->sample_id !=0)  
        {
           print_pll(s,f); // print previous
           s->y = 0; s->err = 0; s->tag = 0; s->setpoint = 0; s->avg_l = 0; 
           s->avg_s = 0; s->sample_id = 0; s->event=0; s->cnt=0; s->flags = 0;
        }

        s->flags = s->flags | 0x1 << what;
        s->seq_id = seq_id;
    }
    else
        return -1;
    int event = 0;
    if(what == DBG_Y)         s->y         = convertNumber(value); //value & 0x0FFFF;
    if(what == DBG_Y)         s->locked    =(value & 0x10000) ? 1 : 0 ; 
    if(what == DBG_ERR)       s->err       = convertNumber(value); 
    if(what == DBG_TAG)       s->tag       = value; 
    if(what == DBG_REF)       s->ref  	   = value; 
    if(what == DBG_SAMPLE_ID) s->sample_id = value; 
    if(what == DBG_EVENT)     s->event     = 0xF & value; 
    if(what == DBG_EVENT && (0xf & value) == mark_event) {s->mark_event = 1; event = 1; printf("process event (above)\n\n"); s->record=1;}
    return event;
}

int main(int argc, char *argv[])
{
    
    int mark_event=DBG_EVT_START;
    int op;
    int backup_p = -1;
    int print = 0;
    char *s, *name, *optstring;
    int sd, rc, length = sizeof(int);
    struct sockaddr_in serveraddr;
    char server[255];
    char temp;
    int i;
    int totalcnt = 0;
    int finish_after_marker = 0;
    int matlab_read = 0;
    int custom_ip=0;
    int debug_to_hex = 0;
    FILE *mPLL, *hPLL;
    struct pll_stat mpll_stat, bpll_stat[4], hpll_stat;
    mpll_stat=clear_stat;
    hpll_stat=clear_stat;
    struct hostent *hostp;
    
    mpll_stat.seq_id=0; 
    hpll_stat.seq_id=0;

    if (argc > 1) 
    {
        // Strip out path from argv[0] if exists, and extract command name
        for (name = s = argv[0]; s[0]; s++) 
            if (s[0] == '/' && s[1]) 
                name = &s[1];
        optstring = "?hxmrd:b:i:p";
        printf("Startup options\n");
        while ((op = getopt(argc, argv, optstring)) != -1) 
        {
            switch(op) 
            {
                case 'd':
                   finish_after_marker = atol(optarg);
                   printf("\tMatlab-ready (no header) acqusition, finish %d samples after"
                          " switchover marker\n",finish_after_marker);
                   break;
                case 'p':
                   print = 1;
                   printf("print all data to stdout\n");
                   break;
                case 'r':
                   printf("record all\n");
                   mpll_stat.record = 1;
                   hpll_stat.record = 1;
                   break;
                case 'm':
                   printf("matlab read\n");
                   matlab_read = 1;
                   break;
                case 'x':
                   printf("debug to hex\n");
                   debug_to_hex = 1;
                   break;
                case 'i':
                   custom_ip = 1;
                   strcpy(server, optarg);
                   break;
                case '?':
                case 'h':
                default:
                   usage(name);
            }
        }
    }
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Client-socket() error");
        exit(-1);
    }
    else
        printf("Client-socket() OK\n");

    if(custom_ip==0)
        strcpy(server, SERVER);
    printf("server IP: %s\n",server); 

    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVPORT);

    if((serveraddr.sin_addr.s_addr = inet_addr(server)) == (unsigned long)INADDR_NONE)
    {
        hostp = gethostbyname(server);
        if(hostp == (struct hostent *)NULL)
        {
            printf("HOST NOT FOUND --> ");
            printf("h_errno = %d\n",h_errno);
            close(sd);
            exit(-1);
        }
        memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
    }

    if((rc = connect(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
    {
        perror("Client-connect() error");
        close(sd);
        exit(-1);
    }
    else
        printf("Connection established...\n");

    mPLL    = fopen("mPLL.txt", "w");
    hPLL    = fopen("hPLL.txt", "w");
    if(mPLL && hPLL)
        printf("Created files OK\n");
    else 
        printf("Problem to created files\n");

    if(finish_after_marker == 0 && matlab_read == 0)
    {
        fprintf(mPLL,   "%s \n", tab_content);
        fprintf(hPLL,   "%s \n", tab_content);

    }

    int got_marker=0;
    int after_marker_cnt = 0;
    int cnt_three=0;
    while(1)
    {
        struct fifo_entry tx_buf[ENTRIES_PER_PACKET];
        rc = read(sd, &tx_buf,  sizeof(tx_buf));
        if(rc < 0)
        {
            perror("Client-read() error");
            close(sd);
            exit(-1);
        }
        else if (rc == 0)
        {
            printf("Server program has issued a close()\n");
            close(sd);
            exit(-1);
        }
        else if(rc> 0)
        {
           if(print) printf("Read %d byes of data\n", rc);
           for(i=0;i<(rc>>3);i++)
           {
              int value = (0xffffff & tx_buf[i].value);
              if(debug_to_hex)
              {
                 fprintf(hPLL,"%10x",value); 
                 cnt_three++;
                 if(cnt_three == 3 )
                 {
                    fprintf(hPLL,"\n"); 
                    cnt_three = 0;
                 }
                 continue;
              }
              if(print || (0xF & (tx_buf[i].value>>24)) == DBG_EVENT)
              {
                 printf("[ID:%8d] VAL:0x%8x | ", tx_buf[i].seq_id, tx_buf[i].value);
                 printf("%s [0x%x] %s [0x%x] :",
                   where[0x7 & (tx_buf[i].value>>28)], (0x7 & (tx_buf[i].value>>28)),
                   what [0xF & (tx_buf[i].value>>24)], (0xF & (tx_buf[i].value>>24)));
                 if( (0xF & (tx_buf[i].value>>24)) == DBG_EVENT) 
                   printf("%s [0x%x] \n", 
                      event[(0x00000f & tx_buf[i].value)],(0x00000f & value));
                 else
                   printf("%8d\n", value);

                 if(0x80000000 &  tx_buf[i].value)
                    printf("-------------------------------------------------------------------------\n");
              }
              if((0x70 & (tx_buf[i].value>>24)) == DBG_MAIN)
                 got_marker =+ process(&mpll_stat, mPLL, tx_buf[i].seq_id,value, (0xF & (tx_buf[i].value>>24)),mark_event, TRIG_ACQ(backup_p,-1),(0x7 & (tx_buf[i].value>>28))); else
              if((0x70 & (tx_buf[i].value>>24)) == DBG_HELPER)
                 got_marker =+ process(&hpll_stat, hPLL, tx_buf[i].seq_id,value, (0xF & (tx_buf[i].value>>24)),mark_event, TRIG_ACQ(backup_p,-1),(0x7 & (tx_buf[i].value>>28))); else
         
              if((got_marker>0 && finish_after_marker>0) || after_marker_cnt>0 )
              {
                 if(finish_after_marker == after_marker_cnt)
                    break;
                 else if ((0x80000000 &  tx_buf[i].value) && ((0x7 & (tx_buf[i].value>>28)) == DBG_MAIN))
                    after_marker_cnt++;
              }
          }
       }
       if(finish_after_marker > 0 && finish_after_marker == after_marker_cnt) break;
    }
    close(sd);
    fclose(mPLL);
    fclose(hPLL);
    exit(0);
    return 0;
}