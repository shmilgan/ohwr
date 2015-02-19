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

#define SERVER "192.168.1.3"
#define SERVPORT 12345

#define RING_BUFFER_ENTRIES 1048576
#define ENTRIES_PER_PACKET 128


#define DBG_Y 0
#define DBG_ERR 1
#define DBG_TAG 2
#define DBG_REF 5
#define DBG_AVG_L 3
#define DBG_EVENT 4
#define DBG_SAMPLE_ID 6
#define DBG_AVG_S 7

#define DBG_MAIN      0x0		/* ...          : Main PLL */
#define DBG_HELPER    0x1		/* Sample source: Helper PLL */
#define DBG_EXT       0x2		/* Sample source: External Reference PLL */
#define DBG_BACKUP    0x4
#define DBG_BACKUP_0  0x4
#define DBG_BACKUP_1  0x5
#define DBG_BACKUP_2  0x6
#define DBG_BACKUP_3  0x7

#define DBG_EVT_START 1       /* PLL has just started */
#define DBG_EVT_LOCKED 2      /* PLL has just become locked */
#define DBG_EVT_SWITCHOVER 4
#define DBG_EVT_STARTBACKUP 8

#define TRIG_ACQ(in,nr) (((in)==(nr))?1:0)
//  c = (a > b)? a : b;
//  
//  getmax(a,b) ((a)>(b)?(a):(b))

const char *tab_content = "        ID,        y,      err,      tag, setpiont,   event,   avg_l,     avg_s\n";

char what[][20] = {{"Y         : "}, // 0
                   {"ERROR     : "}, // 1
                   {"TAG       : "}, // 2
                   {"AVG LONG  : "}, // 3
                   {"EVENT     : "}, // 4
                   {"REFERENCE : "}, // 5
                   {"SAMPLE ID : "},  // 6 
                   {"AVG SHORT : "}};// 7 

char where[][10] = {{"mPLL  : "}, // 0
                    {"hPLL  : "}, // 1
                    {"EXT   : "}, // 2
                    {"----  : "}, // 3
                    {"bPLL0 : "}, // 4
                    {"bPLL1 : "}, // 5
		     {"bPLL2 : "}, // 6
		     {"bPLL3 : "}}; // 7
                    
                    
                    


char event[][20] = {{"non       "}, // 0
                    {"Start PLL "}, // 1
                    {"Locked    "}, // 2
                    {"          "}, // 3
                    {"Switchover"}, // 4
                    {"          "}, // 5
		     {"          "}, // 6
		     {"          "}, // 7
		     {"Start bckp"}  // 8
                    }; 
                    
__attribute__((packed)) struct fifo_entry {
// 	uint32_t value;
	int value;
	uint16_t seq_id;
};
#define F_ALL 0xFF;

struct pll_stat {
	uint16_t seq_id;
	int sample_id;
	int setpoint;
	int tag;
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
    printf("Usage: %s [-dh] \n"
        "\t-d   test\n",
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

// 	if (c == 3) /* ctrl-C */
// 		exit(0);
	return q;
}
struct pll_stat clear_stat={-1,0,0,0,0,0,0,0,0,0,0,0,0};
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
//     if(((s->flags >> DBG_EVENT)     & 0x1) && s->mark_event) 
//     {
//        fprintf(f,"====================== switchover  ===================\n"); 
//        return;
//     }
//     if(((s->flags >> DBG_EVENT)     & 0x1) && s->event == DBG_EVT_STARTBACKUP) 
//     {
//        fprintf(f,"====================== star backup  ===================\n"); 
//        return;
//     }
    if(s->record == 0) return;
    
    if((s->flags >> DBG_SAMPLE_ID) & 0x1) fprintf(f,"%10d",s->sample_id); else fprintf(f,"%10d",0);
    if((s->flags >> DBG_Y)         & 0x1) fprintf(f,"%10d",s->y);         else fprintf(f,"%10d",0);
    if((s->flags >> DBG_ERR)       & 0x1) fprintf(f,"%10d",s->err);       else fprintf(f,"%10d",0);
    if((s->flags >> DBG_TAG)       & 0x1) fprintf(f,"%10d",s->tag);       else fprintf(f,"%10d",0);
    if((s->flags >> DBG_REF)       & 0x1) fprintf(f,"%10d",s->setpoint);  else fprintf(f,"%10d",0);
    if((s->flags >> DBG_EVENT)     & 0x1) fprintf(f,"%10d",s->mark_event);else fprintf(f,"%10d",0);
    if((s->flags >> DBG_AVG_L)     & 0x1) fprintf(f,"%10d",s->avg_l);     else fprintf(f,"%10d",0);
    if((s->flags >> DBG_AVG_S)     & 0x1) fprintf(f,"%10d",s->avg_s);     else fprintf(f,"%10d",0);
    if((s->flags >> DBG_Y)         & 0x1) fprintf(f,"%10d",s->locked);    else fprintf(f,"%10d",0);
//     if((s->flags >> DBG_PERIOD)    & 0x1) fprintf(f,"%10d",s->period);    else fprintf(f,"%10d",0);

    fprintf(f,"\n");
}

int process(struct pll_stat *s, FILE *f, uint16_t seq_id, uint32_t value, int what, int mark_event, int trig_acq)
{
    if(s->seq_id == seq_id)
        s->flags = s->flags | 0x1 << what;
    else if (seq_id >0)
    {
        if(s->seq_id>0) 
           print_pll(s,f); // print previous
        s->y = 0; s->err = 0; s->tag = 0; s->setpoint = 0; s->avg_l = 0; s->avg_s = 0; s->sample_id = 0; s->event=0;
        s->flags  = 0x1 << what;
        s->seq_id = seq_id;
    }
    else
        return -1;
    int event = 0;
    s->mark_event = 0;
    if(what == DBG_Y)         s->y         = value & 0x0FFFF;
    if(what == DBG_Y)         s->locked    =(value & 0x10000) ? 1 : 0 ; 
    if(what == DBG_ERR)       s->err       = convertNumber(value); 
    if(what == DBG_TAG)       s->tag       = value; 
    if(what == DBG_REF)       s->setpoint  = convertNumber(value); 
    if(what == DBG_AVG_L)     s->avg_l     = convertNumber(value); 
    if(what == DBG_AVG_S)     s->avg_s     = convertNumber(value); 
    if(what == DBG_SAMPLE_ID) s->sample_id = value; 
    if(what == DBG_EVENT)     s->event     = 0xF & value; 
    if(what == DBG_EVENT && (0xf & value) == mark_event) {s->mark_event = 1; event = 1; printf("process event\n");}
//     if(what == DBG_EVENT && (0xf & value) == DBG_EVT_STARTBACKUP && trig_acq) {s->record = 1; printf("started recording\n");}
    if(what == DBG_EVENT && (0xf & value) == DBG_EVT_STARTBACKUP) {s->record = 1; printf("started recording\n");}

    return event;
}

int main(int argc, char *argv[])
{
    
    int mark_event=DBG_EVT_SWITCHOVER;
    int op;
    int backup_p = 0;
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
    int debug_to_hex = 0;
    FILE *mPLL, *bPLL, *hPLL, *xPLL[4];
    struct pll_stat mpll_stat, bpll_stat[4], hpll_stat;
    mpll_stat=clear_stat;
    bpll_stat[0] =clear_stat;
    bpll_stat[1] =clear_stat;
    bpll_stat[2] =clear_stat;
    bpll_stat[3] =clear_stat;
    hpll_stat=clear_stat;
    struct hostent *hostp;
    
    mpll_stat.seq_id=0; 
    bpll_stat[0].seq_id=0;
    bpll_stat[1].seq_id=0;
    bpll_stat[2].seq_id=0;
    bpll_stat[3].seq_id=0;
    hpll_stat.seq_id=0;

    if (argc > 1) 
    {
        // Strip out path from argv[0] if exists, and extract command name
        for (name = s = argv[0]; s[0]; s++) 
            if (s[0] == '/' && s[1]) 
                name = &s[1];
        optstring = "?xmrd:b:p";
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
                case 'b':
                   backup_p = atol(optarg);
                   printf("\tBackup port number %d to that triggers start of the acquisition"
                   " - this is to align nicely data for Matlab\n",backup_p);
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
                case '?':
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

//     if(argc > 1)
//     {
//         strcpy(server, argv[1]);
//         printf("Connecting to %s, port %d ...\n", server, SERVPORT);
//     }
//     else
    strcpy(server, SERVER);

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
//     bPLL    = fopen("bPLL.txt", "w");
    /// multibackup ////////////////////////////////////////////////////////////////
    xPLL[0] = fopen("bPLL.txt", "w");
    xPLL[1] = fopen("bPLL1.txt", "w");
    xPLL[2] = fopen("bPLL2.txt", "w");
    xPLL[3] = fopen("bPLL3.txt", "w");
    /// ///////////////////////////////////////////////////////////////////////////
    
    if(mPLL && hPLL && xPLL[0] && xPLL[1] && xPLL[2] && xPLL[3])
        printf("Created files OK\n");
    else 
        printf("Problem to created files\n");

    if(finish_after_marker == 0 && matlab_read == 0)
    {
        fprintf(mPLL,   "%s \n", tab_content);
//         fprintf(bPLL,   "%s \n", tab_content);
        fprintf(hPLL,   "%s \n", tab_content);
        /// multibackup ////////////////////////////////////////////////////////////////
        fprintf(xPLL[0],"%s \n", tab_content);
        fprintf(xPLL[1],"%s \n", tab_content);
        fprintf(xPLL[2],"%s \n", tab_content);
        fprintf(xPLL[3],"%s \n", tab_content);
        /// ///////////////////////////////////////////////////////////////////////////
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
           if((rc>>3) != ENTRIES_PER_PACKET)
               printf("rc != ENTRIES_PER_PACKET:  %d \n", rc);
           for(i=0;i<ENTRIES_PER_PACKET;i++)
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
              if((0x7 & (tx_buf[i].value>>28)) == DBG_MAIN)
                 got_marker =+ process(&mpll_stat, mPLL, tx_buf[i].seq_id,value, (0xF & (tx_buf[i].value>>24)),mark_event, 0); else
              if((0x7 & (tx_buf[i].value>>28)) == DBG_HELPER)
                 got_marker =+ process(&hpll_stat, hPLL, tx_buf[i].seq_id,value, (0xF & (tx_buf[i].value>>24)),mark_event,0); else
              if((0x7 & (tx_buf[i].value>>28)) == DBG_BACKUP)
                 got_marker =+ process(&bpll_stat[0], xPLL[0], tx_buf[i].seq_id,value, (0xF & (tx_buf[i].value>>24)),mark_event, TRIG_ACQ(backup_p,0)); else

//               /// multibackup ////////////////////////////////////////////////////////////////
//               if((0x7 & (tx_buf[i].value>>28)) == DBG_BACKUP_0)
//                  got_marker =+ process(&bpll_stat, xPLL[0], tx_buf[i].seq_id,value, (0xF & (tx_buf[i].value>>24)),mark_event);
              if((0x7 & (tx_buf[i].value>>28)) == DBG_BACKUP_1)
                 got_marker =+ process(&bpll_stat[1], xPLL[1], tx_buf[i].seq_id,value, (0xF & (tx_buf[i].value>>24)),mark_event, TRIG_ACQ(backup_p,1)); else
              if((0x7 & (tx_buf[i].value>>28)) == DBG_BACKUP_2)
                 got_marker =+ process(&bpll_stat[2], xPLL[2], tx_buf[i].seq_id,value, (0xF & (tx_buf[i].value>>24)),mark_event, TRIG_ACQ(backup_p,2)); else
              if((0x7 & (tx_buf[i].value>>28)) == DBG_BACKUP_3)
                 got_marker =+ process(&bpll_stat[3], xPLL[3], tx_buf[i].seq_id,value, (0xF & (tx_buf[i].value>>24)),mark_event, TRIG_ACQ(backup_p,3));
              /// ///////////////////////////////////////////////////////////////////////////

              if((got_marker>0 && finish_after_marker>0) || after_marker_cnt>0 )
              {
                 if(finish_after_marker == after_marker_cnt)
                    break;
                 else if ((0x80000000 &  tx_buf[i].value) && ((0x7 & (tx_buf[i].value>>28)) == DBG_MAIN))
                    after_marker_cnt++;
              }
          }
       }
//        int c = term_get();
//        if(c=='q') break;
       if(finish_after_marker > 0 && finish_after_marker == after_marker_cnt) break;
    }
    close(sd);
    fclose(mPLL);
//     fclose(bPLL);
    fclose(hPLL);
    /// multibackup ////////////////////////////////////////////////////////////////
    fclose(xPLL[0]);
    fclose(xPLL[1]);
    fclose(xPLL[2]);
    fclose(xPLL[3]);
    /// ///////////////////////////////////////////////////////////////////////////
    exit(0);
    return 0;
}
