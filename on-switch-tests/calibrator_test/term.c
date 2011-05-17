#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
  struct termios oldkey, newkey;

int term_restore()
{
      tcsetattr(STDIN_FILENO,TCSANOW,&oldkey);
}

void term_init()
{
  
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    tcgetattr(STDIN_FILENO,&oldkey);
	memcpy(&newkey, &oldkey, sizeof(struct termios));
      newkey.c_cflag = B9600 | CRTSCTS | CS8 | CLOCAL | CREAD;
      newkey.c_iflag = IGNPAR;
      newkey.c_oflag = OPOST |ONLCR;
      newkey.c_lflag = 0;
      newkey.c_cc[VMIN]=1;
      newkey.c_cc[VTIME]=0;
      tcflush(STDIN_FILENO, TCIFLUSH);
      tcsetattr(STDIN_FILENO,TCSANOW,&newkey);
      atexit(term_restore);
}


int term_poll()
{
	struct pollfd pfd;  
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN | POLLPRI;    

    if(poll(&pfd,1,0)>0)return 1;

    return 0;
}


int term_get()
{
  unsigned char c;
  int q;

  if(read(STDIN_FILENO, &c, 1 ) == 1)
  {
	q=c;
  } else q=-1;
  
  
    return q;
}

void clear_screen()
{
    printf("\033[2J\033[1;1H");
    }
    