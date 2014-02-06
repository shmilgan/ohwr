#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <shw_io.h>
#include <fpga_io.h>

#define VUART_BASE 0x10000
#define VUART_TDR 0x10
#define VUART_RDR 0x14
#define RDR_RDY 0x100
#define RDR_DAT_MSK 0xff

int vuart_tx(char *buf, int size)
{
	while(size--)
		_fpga_writel(VUART_BASE+VUART_TDR, *buf++);
}

int vuart_rx(char *buf, int size)
{
	int n_rx;
	int rdr;
	
	while(size--) {
		rdr = _fpga_readl(VUART_BASE+VUART_RDR);
		if(rdr & RDR_RDY) {
			*buf++ = (rdr & RDR_DAT_MSK);
			++n_rx;
		} else
			return n_rx;
	}

	return n_rx;
}

static int transfer_byte(int from, int is_control) {
	char c;
	int ret;
	do {
		ret = read(from, &c, 1);
	} while (ret < 0 && errno == EINTR);
	if(ret == 1) {
		if(is_control) {
			if(c == '\x01') { // C-a
				return -1;
			}
		}
		vuart_tx(&c, 1);
	} else {
		fprintf(stderr, "nothing to read. Port disconnected?\n");
		return -2;
	}
	return 0;
}


void term_main(int keep_term)
{
	struct termios oldkey, newkey;
	int need_exit = 0;

	fprintf(stderr, "[press C-a to exit]\n");

	if(!keep_term) {
		tcgetattr(STDIN_FILENO,&oldkey);
		newkey.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
		newkey.c_iflag = IGNPAR;
		newkey.c_oflag = 0;
		newkey.c_lflag = 0;
		newkey.c_cc[VMIN]=1;
		newkey.c_cc[VTIME]=0;
		tcflush(STDIN_FILENO, TCIFLUSH);
		tcsetattr(STDIN_FILENO,TCSANOW,&newkey);
	}
	while(!need_exit) {
		fd_set fds;
		int ret;
		char rx;
		struct timeval tv = {0, 10000};

		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		ret = select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
		if(ret == -1) {
			perror("select");
		} else if (ret > 0) {
			if(FD_ISSET(STDIN_FILENO, &fds)) {
				need_exit = transfer_byte(STDIN_FILENO, 1);
			}
		}

		while((vuart_rx(&rx, 1)) == 1)
			fprintf(stderr,"%c", rx);
	}

	if(!keep_term)
		tcsetattr(STDIN_FILENO,TCSANOW,&oldkey);
}


int main(void)
{
	shw_fpga_mmap_init();
	term_main(0);

	return 0;
}



