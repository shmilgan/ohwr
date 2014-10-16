#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define BSIZE 4096
char buffer[BSIZE];

int main(int argc, char **argv)
{
	int fd[argc]; /* in place 0 we put stdout */
	int i, n, err = 0;

	if (argc > 1 && argv[1][0] == '-') {
		/* --help, -h, whatever */
		fprintf(stderr, "%s: copies stdin to stdout and arguments,\n"
			"    using non-blocking open and non-blocking write\n",
			argv[0]);
		exit(0);
	}

	signal(SIGPIPE, SIG_IGN); /* we don't care about closed output pipes */

	fcntl(STDOUT_FILENO, F_SETFL, fcntl(1,F_GETFL) | O_NONBLOCK);
	fd[0] = STDOUT_FILENO;

	/* open them all */
	for (i = 1; i < argc; i++) {
		fd[i] = open(argv[i], O_WRONLY | O_NONBLOCK);

		/* A special case to be able to open fifos: see "man 7 fifo" */
		if ((fd[i] < 0) && (errno == ENXIO))
		    fd[i] = open(argv[i], O_RDWR | O_NONBLOCK);

		if (fd[i] < 0) {
			fprintf(stderr, "%s: open(%s): %s\n", argv[0], argv[i],
				strerror(errno));
			err++;
		}
	}
	if (err)
		exit(1);

	while (1) {
		n = read(0, buffer, sizeof(buffer)); /* this is blocking: ok */
		if (n < 0 && errno == EAGAIN)
			continue;
		if (n == 0)
			exit(0); /* EOF */

		for (i = 0; i < argc; i++)
			write(fd[i], buffer, n); /* ignore errors here */
	}
}
