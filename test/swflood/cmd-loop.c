#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "swflood.h"

/*
 * Loops are made of two commands: "repeat" and "endr"
 */
static int rep_verify(struct swf_line *line)
{
	char *t;
	int i, err = 0;

	if (line->argc != 3) {
		fprintf(stderr, "%s: wrong number of arguments\n",
			line->argv[0]);
		return -EINVAL;
	}

	if (strlen(line->argv[1]) != 1 || !isalpha(line->argv[1][0])) {
		fprintf(stderr, "%s: not a letter \"%s\"\n",
			line->argv[0], line->argv[1]);
		err++;
	}

	i = strtol(line->argv[2], &t, 0);
	if (t && *t) {
		fprintf(stderr, "%s: not a number \"%s\"\n",
			line->argv[0], line->argv[2]);
		err++;
	}
	if (err)
		return -EINVAL;
	line->repcount = i;
	return 0;
}

static struct swf_line *rep_execute(struct swf_line *line,
				     struct swf_status *status)
{
	return line->next;
}

static struct swf_command cmd_repeat = {
	.name = "repeat",
	.verify = rep_verify,
	.execute = rep_execute,
};
declare_command(cmd_repeat);


/* endr decrements and goes back or forward */
static int endr_verify(struct swf_line *line)
{
	if (line->argc != 2) {
		fprintf(stderr, "%s: wrong number of arguments\n",
			line->argv[0]);
		return -EINVAL;
	}

	if (strlen(line->argv[1]) != 1 || !isalpha(line->argv[1][0])) {
		fprintf(stderr, "%s: not a letter \"%s\"\n",
			line->argv[0], line->argv[1]);
		return -EINVAL;
	}
	return 0;
}

static int endr_verify_late(struct swf_line *line)
{
	/* Loop back looking for the letter */
	struct swf_line *other;
	int letter = line->argv[1][0];

	for (other = line->prev; other; other = other->prev) {
		if (other->cmd != &cmd_repeat)
			continue;
		if (other->argv[1][0] != letter)
			continue;
		line->data = other;
		return 0;
	}
	fprintf(stderr, "%s: can't find matching '%c'\n",
			line->argv[0], letter);
		return -EINVAL;
}

static struct swf_line *endr_execute(struct swf_line *line,
				       struct swf_status *status)
{
	struct swf_line *other = line->data;

	/* The repcount field was 0 at allocate time */
	if (!line->repcount) {
		line->repcount = other->repcount;
	}
	line->repcount--;
	if (line->repcount > 0)
		return line->data;
	return line->next;
}

static struct swf_command cmd_verbose = {
	.name = "endr",
	.verify = endr_verify,
	.verify_late = endr_verify_late,
	.execute = endr_execute,
};
declare_command(cmd_verbose);
