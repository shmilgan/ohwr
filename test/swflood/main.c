/* Alessandro Rubini for CERN and White Rabbit, 2011 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "swflood.h"

static void swf_dump(struct swf_line *line)
{
	int i;

	fprintf(stderr, "%s:%i: (%i) =", line->fname, line->lineno,
		line->argc);
	for (i = 0; i < line->argc; i++)
		fprintf(stderr, " \"%s\"", line->argv[i]);
	putc('\n', stderr);
}

int main(int argc, char **argv)
{
	struct swf_status *status;
	struct swf_line *line, *lastline = NULL;
	FILE *f;
	char s[256];
	int i, ret, lineno, err = 0;

	status = calloc(1, sizeof(*status));
	if (!status) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		exit(1);
	}
	status->logfile = fopen("/dev/null", "w");

	if (argc == 1) {
		fprintf(stderr, "%s: Use \"%s <cfg> [<cfg> ...]\n",
			argv[0], argv[0]);
		exit(1);
	}

	/* Do the parsing */
	for (i = 1; i < argc; i++) {
		lineno = 0;
		f = fopen(argv[i], "r");
		if (!f) {
			fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i],
				strerror(errno));
			err++;
			continue;
		}
		while (fgets(s, sizeof(s), f)) {
			lineno++;
			line = calloc(1, sizeof(*line));
			if (!line) {
				fprintf(stderr, "%s: %s\n", argv[0],
					strerror(errno));
				exit(1);
			}
			line->fname = argv[i];
			line->lineno = lineno;
			ret = swf_parse(s, line);
			if (ret == -ENODATA) {
				free(line);
				continue;
			}
			if (ret < 0) {
				fprintf(stderr, "%s:%i: %s in \"%s\"\n",
					argv[i], lineno,
					strerror(-ret), s);
				err++;
				continue;
			}
			/* enqueue this line */
			if (lastline) {
				lastline->next = line;
				line->prev = lastline;
			} else {
				status->line = line;
			}
			lastline = line;
			if (line->cmd->verify_late
			    && line->cmd->verify_late(line)) {
				err++;
				continue;
			}
			if (getenv("VERBOSE"))
				swf_dump(line);
		}
	}
	if (err) exit(1);

	/* Now run the commands */
	for (line = status->line; line;
	     line = line->cmd->execute(line, status)) {
		if (getenv("VERBOSE") && atoi(getenv("VERBOSE")) > 10)
			fprintf(stderr, "run %-10s (%s:%i -- %i)\n",
				line->argv[0], line->fname, line->lineno,
				line->repcount);
	}
	/* done, it was easy, wasn't it? */
	return 0;
}
