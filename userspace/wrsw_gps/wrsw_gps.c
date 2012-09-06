


//Serial acquisition based on the MArj Zehner/Peter Baumann 1999 based

#include <string.h> /* memset */
#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>       /* for mktime()       */
#include <sys/time.h>   /* for settimeofday() */

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS2"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE; 
void signal_handler_IO (int status);   
int wait_flag=TRUE;      
int verbose=0;
int daem=0;
int tick=10;
char *maxtime="10";

int main(int argc, char **argv)
{
    int fd, res,arg;
    struct termios oldtio,newtio;
    struct sigaction saio;  
    char buf[64];   
    extern char *optarg;

    while((arg = getopt(argc,argv,"dhvt:")) != -1){  
        switch(arg){
            case 'v':   
                verbose =1;
                break;
            case 't':
                maxtime = optarg;
                tick = atoi(maxtime);
                break;
            case 'd':
                daem = 1;
                break;
            case 'h':
                printf("Usage: wrsw_gps [options] \n");
                printf("Reads the serial port of a GPS");
                printf(" to actualize the date and time of the WR Switch.\n");
                printf("Uses NMEA protocol and a baud rate of 9600.\n");
                printf("Options:\n");
                printf("-v  \t \t verbose\n");
                printf("-d  \t \t run as a daemon \n");
                printf("-t=time \t update rate (in sec) of the WR Switch date and time info\n");
                printf("-h  \t  \t help\n");
                exit(0);
             case '?':
                exit(1);
            default:
                break;
        }
    }
    if(daem)
    {
        demonize();
    }

// open the device to be non-blocking (read will return immediatly)
    fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd <0) {perror(MODEMDEVICE); return -1; }

    // install the signal handler before making the device asynchronous
    saio.sa_handler = signal_handler_IO;
    sigemptyset(&saio.sa_mask); 
    saio.sa_flags = 0;
    saio.sa_restorer = NULL;
    sigaction(SIGIO,&saio,NULL);

    // allow the process to receive SIGIO 
    fcntl(fd, F_SETOWN, getpid());

    fcntl(fd, F_SETFL, FASYNC);

    tcgetattr(fd,&oldtio); // save current port settings

    newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR | ICRNL;
    newtio.c_oflag = 0;
    newtio.c_lflag = ICANON;
    newtio.c_cc[VMIN]=1;
    newtio.c_cc[VTIME]=0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);

    while (STOP==FALSE) {
        usleep(1000);
        // after receiving SIGIO, wait_flag = FALSE, input is available
        if (wait_flag==FALSE) { 
            res = read(fd,buf,64);
            if(verbose)printf("%s",buf);
            buf[res]=0;
            if(buf[0]=='$')
            {
                if(setgpstime(buf))
                {
                    if(verbose)printf("Error setting time\n");
                    //STOP=TRUE; 
                }
                sleep(atoi(maxtime));
            }
            wait_flag = TRUE;      // wait for new input
        }
    }
    tcsetattr(fd,TCSANOW,&oldtio);
    return 0;
}

void signal_handler_IO (int status)
{
    if(tick<atoi(maxtime))
        tick++;
    else
    {
        tick=0;
        wait_flag = FALSE;
    }   
    if(verbose)printf("Signal from GPS \n");
}

void demonize()
{
    pid_t pid,sid;
    if ( getppid() == 1 ) return;

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }   
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }   

    umask(0);

    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }   

    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }   

    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);

}

//Format NMEA string
// $GPRMC,hhmmss.ss,A,bbbb.bb,n,lllll.ll,e,0.0,0.0,ddmmyy,0.0,a*hh<CR><LF>
// $            -> Start character
// 7 hhmmss.ss    -> Hours minutes seconds fraction of seconds
// A            -> A.- time data valid V.- time data not valid
// bbbb.b       -> Latitude, leading sings 0x20
// n            -> Latitude N/S
// lllll.l      -> longitude, leading sings 0x20
// e            -> Longitude E/W
// 32 ddmmyy       -> day/month/year
// a            -> magnetic variation
// hh           -> checksum 
// <CR>         -> carriage-return 0x0D
// <LF>         -> line-feed 0x0A
//




int setgpstime(char * gps)
{
  int            result; 
  time_t         new_time;  
  struct tm      gps_time; 
  struct timeval val_time; 
  char tmp[2];

  
  memcpy(tmp,gps+11,2); 
  gps_time.tm_sec = atoi(tmp); // sec

  if(verbose)printf("Sec %d ",atoi(tmp));

  memcpy(tmp,gps+9,2); 
  if(verbose)printf("Min %d ",atoi(tmp));
  gps_time.tm_min = atoi(tmp); // min

  memcpy(tmp,gps+7,2); 
  if(verbose)printf("Hr %d ",atoi(tmp));
  gps_time.tm_hour = atoi(tmp); // hour
  
  memcpy(tmp,gps+48,2);
  if(verbose)printf("Day %d ",atoi(tmp));
  gps_time.tm_mday =atoi(tmp); // day

  memcpy(tmp,gps+50,2);
  if(verbose)printf("Mont %d ",atoi(tmp));
  gps_time.tm_mon = atoi(tmp)-1; // month 
  
  memcpy(tmp,gps+52,2);
  if(verbose)printf("YEAR %d \n",atoi(tmp) + 2000 - 1900);
  gps_time.tm_year = atoi(tmp) + 2000 - 1900; // year

  new_time=mktime(&gps_time);

  val_time.tv_sec =new_time;
  val_time.tv_usec=0;

  result=settimeofday(&val_time,NULL);

  if(result<0)
  {
      return 1;
  }

  return 0;

}
