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
#define DBG_PERIOD 3
#define DBG_EVENT 4
#define DBG_SAMPLE_ID 6

#define DBG_HELPER 0x2       /* Sample source: Helper PLL */
#define DBG_EXT 0x4          /* Sample source: External Reference PLL */
#define DBG_BACKUP 0x1
#define DBG_MAIN 0x0          /* ...          : Main PLL */

#define DBG_EVT_START 1       /* PLL has just started */
#define DBG_EVT_LOCKED 2      /* PLL has just become locked */

const char *tab_content = "        ID,        y,      err,      tag, setpiont,   period\n";

char what[][20] = {{"Y         : "}, // 0
                   {"ERROR     : "}, // 1
                   {"TAG       : "}, // 2
                   {"PERIOD    : "}, // 3
                   {"EVENT     : "}, // 4
                   {"REFERENCE : "}, // 5
                   {"SAMPLE ID : "}};// 6 

char where[][10] = {{"mPLL : "}, // 0
                    {"---- : "}, // 1
                    {"hPLL : "}, // 2
                    {"---- : "}, // 1
                    {"EXT  : "}}; // 4


char event[][20] = {{"non       "}, // 0
                    {"Start PLL "}, // 1
                    {"Locked    "}}; // 2
                    
__attribute__((packed)) struct fifo_entry {
	uint32_t value;
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
	int period;
	int flags;
	int printed;
};
int term_get(void)
{
	unsigned char c;
	int q;

	if(read(STDIN_FILENO, &c, 1 ) == 1)
	{
		q=c;
	} else q=-1;

	if (c == 3) /* ctrl-C */
		exit(0);
	return q;
}
void print_pll(struct pll_stat *s,FILE *f)
{
    if((s->flags >> DBG_SAMPLE_ID) & 0x1) fprintf(f,"%10d",s->sample_id); else fprintf(f,"%10d",0);
    if((s->flags >> DBG_Y)         & 0x1) fprintf(f,"%10d",s->y);         else fprintf(f,"%10d",0);
    if((s->flags >> DBG_ERR)       & 0x1) fprintf(f,"%10d",s->err);       else fprintf(f,"%10d",0);
    if((s->flags >> DBG_TAG)       & 0x1) fprintf(f,"%10d",s->tag);       else fprintf(f,"%10d",0);
    if((s->flags >> DBG_REF)       & 0x1) fprintf(f,"%10d",s->setpoint);  else fprintf(f,"%10d",0);
    if((s->flags >> DBG_PERIOD)    & 0x1) fprintf(f,"%10d",s->period);    else fprintf(f,"%10d",0);
    fprintf(f,"\n");
}

int process(struct pll_stat *s, FILE *f, uint16_t seq_id, uint32_t value, int what)
{
    if(s->seq_id == seq_id)
        s->flags = s->flags | 0x1 << what;
    else if (seq_id >=0)
    {
        print_pll(s,f); // print previous
        s->y = 0; s->err = 0; s->tag = 0; s->setpoint = 0; s->period = 0; s->sample_id = 0;
        s->flags  = 0x1 << what;
        s->seq_id = seq_id;
    }
    else
        return -1;

    if(what == DBG_Y)         s->y         = value;
    if(what == DBG_ERR)       s->err       = value;
    if(what == DBG_TAG)       s->tag       = value;
    if(what == DBG_REF)       s->setpoint  = value;
    if(what == DBG_PERIOD)    s->period    = value;
    if(what == DBG_SAMPLE_ID) s->sample_id = value;

    printf("seq=%d | %d , val=%d , what=%d flags=0x%x [%d, %d, %d, %d, %d, %d]\n",
    seq_id,s->seq_id, value, what, s->flags,s->y, s->err, s->tag, s->setpoint, s->period,s->sample_id);
    return 0;
}

int main(int argc, char *argv[])
{
    int sd, rc, length = sizeof(int);
    struct sockaddr_in serveraddr;
    char server[255];
    char temp;
    int i;
    int totalcnt = 0;
    FILE *mPLL, *bPLL, *hPLL;
    struct pll_stat mpll_stat, bpll_stat, hpll_stat;
    struct hostent *hostp;
    
    mpll_stat.seq_id=-1; 
    bpll_stat.seq_id=-1;
    hpll_stat.seq_id=-1;


    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Client-socket() error");
        exit(-1);
    }
    else
        printf("Client-socket() OK\n");

    if(argc > 1)
    {
        strcpy(server, argv[1]);
        printf("Connecting to %s, port %d ...\n", server, SERVPORT);
    }
    else
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

    mPLL = fopen("mPLL.txt", "w");
    bPLL = fopen("bPLL.txt", "w");
    hPLL = fopen("hPLL.txt", "w");
    if(mPLL && bPLL && hPLL)
        printf("Created files OK\n");
    else 
        printf("Problem to created files\n");

    fprintf(mPLL,"%s \n", tab_content);
    fprintf(bPLL,"%s \n", tab_content);
    fprintf(hPLL,"%s \n", tab_content);

//     close(sd);
//     exit(0);
//     return 0;
    
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
           printf("Read %d byes of data\n", rc);
           for(i=0;i<ENTRIES_PER_PACKET;i++)
           {
//               printf("[ID:%8d] VAL:0x%8x | ", tx_buf[i].seq_id, tx_buf[i].value);
//               printf("%s [0x%x] %s [0x%x] :",
//                  where[0x7 & (tx_buf[i].value>>28)], (0x7 & (tx_buf[i].value>>28)),
//                  what [0xF & (tx_buf[i].value>>24)], (0xF & (tx_buf[i].value>>24)));
//               if( (0xF & (tx_buf[i].value>>24)) == DBG_EVENT) 
//                  printf("%s [0x%x] \n", 
//                     event[(0x00000f & tx_buf[i].value)],(0x00000f & tx_buf[i].value));
//               else
//                  printf("%8d \n", (0xffffff & tx_buf[i].value));
//               if(0x80000000 &  tx_buf[i].value)
//                   printf("-------------------------------------------------------------------------\n");
              if(0x7 & (tx_buf[i].value>>28) == DBG_MAIN)
                 process(&mpll_stat, mPLL, tx_buf[i].seq_id,(0xffffff & tx_buf[i].value) , (0xF & (tx_buf[i].value>>24)));
              if(0x7 & (tx_buf[i].value>>28) == DBG_HELPER)
                 process(&hpll_stat, hPLL, tx_buf[i].seq_id,(0xffffff & tx_buf[i].value) , (0xF & (tx_buf[i].value>>24)));
              if(0x7 & (tx_buf[i].value>>28) == DBG_BACKUP)
                 process(&bpll_stat, bPLL, tx_buf[i].seq_id,(0xffffff & tx_buf[i].value) , (0xF & (tx_buf[i].value>>24)));
          }
        }
        int c = term_get();
        if(c=='q') break;
    }
    close(sd);
    fclose(mPLL);
    fclose(bPLL);
    fclose(hPLL);
    exit(0);
    return 0;
}
