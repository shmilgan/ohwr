#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>

#include "term.h"

static struct termios oldkey, newkey;
static int term_usecolor;

void term_restore(void)
{
	tcsetattr(STDIN_FILENO,TCSANOW,&oldkey);
}

void term_init(int usecolor)
{

	term_usecolor = usecolor;
	if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
	    return;

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	tcgetattr(STDIN_FILENO,&oldkey);
	newkey = oldkey;
	cfmakeraw(&newkey);
	newkey.c_oflag |= OPOST; /* don't require \r in source files */
	tcflush(STDIN_FILENO, TCIFLUSH);
	tcsetattr(STDIN_FILENO,TCSANOW,&newkey);
	atexit(term_restore);
}

int term_poll(int msec_timeout)
{
	struct pollfd pfd;
	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN | POLLPRI;

	if(poll(&pfd, 1, msec_timeout) > 0) return 1;

	return 0;
}


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

void term_cprintf(int color, const char *fmt, ...)
{
	va_list ap;
	if (term_usecolor)
		printf("\033[0%d;3%dm",color & C_DIM ? 2:1, color&0x7f);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void term_pcprintf(int row, int col, int color, const char *fmt, ...)
{
	va_list ap;
	printf("\033[%d;%df", row, col);
	if (term_usecolor)
		printf("\033[0%d;3%dm",color & C_DIM ? 2:1, color&0x7f);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void term_clear(void)
{
	printf("\033[2J\033[1;1H");
}

