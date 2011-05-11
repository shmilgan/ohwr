#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "swflood.h"

/* echo is always verified and is trivial to execute */
static int echo_verify(struct swf_line *line)
{
	return 0;
}

static int do_echo(FILE *f, int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++)
		fprintf(f, "%s%c", argv[i], i+1 == argc ? '\n' : ' ');
	if (argc == 1)
		putc('\n', f);
	return 0;
}


static struct swf_line *echo_execute(struct swf_line *line,
				     struct swf_status *status)
{
	do_echo(stdout, line->argc, line->argv);
	fprintf(status->logfile, "# ");
	do_echo(status->logfile, line->argc, line->argv);
	return line->next;
}

static struct swf_command cmd_echo = {
	.name = "echo",
	.verify = echo_verify,
	.execute = echo_execute,
};
declare_command(cmd_echo);


/* verbose sets status variables by possibly opening a file */
static int verbose_verify(struct swf_line *line)
{
	char *t;
	int i;

	if (line->argc < 2 || line->argc > 3) {
		fprintf(stderr, "%s: wrong number of arguments\n",
			line->argv[0]);
		return -EINVAL;
	}
	i = strtol(line->argv[1], &t, 0);
	if (t && *t) {
		fprintf(stderr, "%s: not a number \"%s\"\n",
			line->argv[0], line->argv[1]);
		return -EINVAL;
	}
	/* check of the file name is left to exectute time */
	return 0;
}

static struct swf_line *verbose_execute(struct swf_line *line,
				       struct swf_status *status)
{
	FILE *f;

	status->verbose = atoi(line->argv[1]);
	if (line->argc != 3)
		return line->next;
	f = fopen(line->argv[2], "a");
	if (!f) {
		fprintf(stderr, "%s: open(): %s\n",
			line->argv[2], strerror(errno));
		fprintf(status->logfile, "Error: %s: open(): %s\n",
			line->argv[2], strerror(errno));
		return line->next;
	}
	fclose(status->logfile);
	status->logfile = f;
	return line->next;
}

static struct swf_command cmd_verbose = {
	.name = "verbose",
	.verify = verbose_verify,
	.execute = verbose_execute,
};
declare_command(cmd_verbose);

/* sleep seconds and/or milliseconds */
static int sleep_verify(struct swf_line *line)
{
	char *t;
	int i, err = 0;

	if (line->argc != 2 && line->argc != 3) {
		fprintf(stderr, "%s: wrong number of arguments\n",
			line->argv[0]);
		return -EINVAL;
	}
	i = strtol(line->argv[1], &t, 0);
	if (t && *t) {
		fprintf(stderr, "%s: not a number \"%s\"\n",
			line->argv[0], line->argv[1]);
		err++;
		t = NULL;
	}
	if (line->argc == 3)
		i = strtol(line->argv[1], &t, 0);
	if (t && *t) {
		fprintf(stderr, "%s: not a number \"%s\"\n",
			line->argv[0], line->argv[2]);
		err++;
	}
	if (err)
		return -EINVAL;
	return 0;
}

static struct swf_line *sleep_execute(struct swf_line *line,
				       struct swf_status *status)
{
	int i;

	i = strtol(line->argv[1], NULL, 0);
	sleep(i);
	if (line->argc == 3) {
		i = strtol(line->argv[2], NULL, 0);
		usleep(1000 * i);
	}
	return line->next;
}

static struct swf_command cmd_sleep = {
	.name = "sleep",
	.verify = sleep_verify,
	.execute = sleep_execute,
};
declare_command(cmd_sleep);

