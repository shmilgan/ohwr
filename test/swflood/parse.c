/* Alessandro Rubini for CERN and White Rabbit, 2011 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "swflood.h"

#define MAXARG 15 /* lazy me */

/*
 * This can write to stderr and to the newly-allocated line.
 * It returns -ENODATA if the line is empty, 0 on success, or
 * other negative errors
 */
int swf_parse(char *s, struct swf_line *line)
{
	int i;
	struct swf_command **cmdp;

	/* First trim any leading and trailing whitespace */
	while (isspace(*s))
		s++;
	i = strlen(s);
	while(i && isspace(s[i-1]))
		s[--i] = '\0';

	/* Now, if it's empty we are done */
	if (!s[0] || s[0] == '#')
		return -ENODATA;

	/* Make a local copy for our argv, and allocate it */
	s = strdup(s);
	line->argv = malloc(sizeof(*line->argv) * (MAXARG + 1));
	if (!s || !line->argv) {
		free(s);
		free(line->argv);
		return -ENOMEM;
	}

	/* fill the argv */
	i = 0;
	while (1) {
		line->argv[i++] = s;
		while (*s && !isspace(*s))
			s++;
		if (!*s)
			break;
		*(s++) = '\0';
	}
	line->argc = i;

	/* find the command */
	for (cmdp = __cmd_start; cmdp < __cmd_end; cmdp++)
		if (!strcmp(line->argv[0], (*cmdp)->name))
			break;
	if (cmdp == __cmd_end) {
		fprintf(stderr, "   invalid command \"%s\"\n",
			line->argv[0]);
		return -EINVAL;
	}

	/* and verify it */
	line->cmd = *cmdp;
	return line->cmd->verify(line);

	return 0;
}
