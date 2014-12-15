/* Alessandro Rubini for CERN, 2014 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <libwr/config.h>

/* All strings here are strdup'd and then split; you can't free(3) them */
struct cfg_item {
	char *name;
	char *value;
	struct cfg_item *subcfg;
	struct cfg_item *next;
};

static struct cfg_item *libwr_cfg;

static int libwr_subcfg(struct cfg_item *c)
{
	struct cfg_item *subc;
	c->subcfg = NULL;
	char *rest, *s;

	if (!strchr(c->value, '='))
		return 0;

	/* Very lazily, strdup this again, and then modify in place */
	rest = strdup(c->value);
	if (!rest)
		return -1;
	do {
		subc = calloc(1, sizeof(*c));
		if (!subc)
			return -1;
		subc->name = rest;

		/* Trim this and save trailing text */
		s = strchr(rest, ',');
		if (s) {
			rest = s + 1;
			*s = '\0';
		} else {
			rest = NULL;
		}

		/* split between name and value */
		s = strchr(subc->name, '=');
		if (s) {
			subc->value = s + 1;
			*s = '\0';
		}

		/* store and continue, if more is there */
		subc->next = c->subcfg;
		c->subcfg = subc;
	} while (rest);
	return 0;
}

/* Parse a line into name=value and subcfg. Assume dot-config format */
static int libwr_cfg_line(char *line)
{
	struct cfg_item *c;
	char name[512], value[512];

	if (line[0] == '#')
		return 0;
	if (strlen(line) < 2)
		return 0;

	if (sscanf(line, "CONFIG_%[^=]=%[^\n]", name, value) != 2) {
		errno = EINVAL;
		return -1;
	}

	c = malloc(sizeof(*c));
	if (!c)
		return -1;
	c->subcfg = NULL;
	c->name = strdup(name);
	c->value = strdup(value);
	if (!c->name || !c->value)
		return -1;

	/* Strip quotes */
	if (c->value[0] == '"')
		c->value++;
	if (c->value[strlen(c->value) - 1] == '"')
		c->value[strlen(c->value) - 1] = '\0';

	libwr_subcfg(c);

	c->next = libwr_cfg;
	libwr_cfg = c;
	return 0;
}

int libwr_cfg_read_file(char *dotconfig)
{
	char s[512];
	FILE *f;
	int lineno = 0;

	f = fopen(dotconfig, "r");
	if (!f)
		return -1;

	while (fgets(s, sizeof(s), f)) {
		lineno++;
		if (libwr_cfg_line(s) < 0)
			return -lineno;
	}
	fclose(f);
	return 0;
}

int libwr_cfg_dump(FILE *output)
{
	struct cfg_item *c = libwr_cfg;
	struct cfg_item *subc = libwr_cfg;

	while (c) {
		fprintf(output, "name=\"%s\"\n  value=\"%s\"\n",
			c->name, c->value);
		subc = c->subcfg;
		while (subc) {
			fprintf(output, "    name=\"%s\"\n"
				"      value=\"%s\"\n",
				subc->name, subc->value);
			subc = subc->next;
		}
		c = c->next;
	}
	return 0;
}

int libwr_cfg_read_verify_file(char *dotconfig, char *kconfig)
{
	FILE *f;
	int errors = 0;
	char s[256], name[256];
	struct cfg_item *c;

	struct kc {
		char *name;
		struct kc *next;
	} *all_configs = NULL, *kc, *next;

	if (libwr_cfg_read_file(dotconfig))
		return -1;

	/* Read Kconfig and store all config names */
	f = fopen(kconfig, "r");
	if (!f)
		return -1;
	while (fgets(s, sizeof(s), f)) {
		if (sscanf(s, "config %s", name) != 1)
			continue;
		kc = malloc(sizeof(*kc));
		if (!kc)
			return -1;
		kc->name = strdup(name);
		kc->next = all_configs;
		all_configs = kc;
	}
	fclose(f);

	/* Verify all configs, complain if missing */
	for (c = libwr_cfg; c; c = c->next) {
		for (kc = all_configs; kc; kc = kc->next)
			if (!strcmp(c->name, kc->name))
				break;
		if (!kc) {
			fprintf(stderr, "Configuration \"%s\" not found\n",
				c->name);
			errors++;
		}
	}

	/* Free all kconfig allocs */
	for (kc = all_configs; kc; kc = next) {
		next = kc->next;
		free(kc->name);
		free(kc);
	}
	if (errors) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}

char *libwr_cfg_get(char *cfgname)
{
	struct cfg_item *c;

	for (c = libwr_cfg; c; c = c->next)
		if (!strcmp(cfgname, c->name))
			return c->value;
	return NULL;
}

char *libwr_cfg_get2(char *cfgname, char *subname)
{
	struct cfg_item *c, *subc;

	for (c = libwr_cfg; c; c = c->next)
		if (!strcmp(cfgname, c->name)) {
			for (subc = c->subcfg; subc; subc = subc->next)
				if (!strcmp(subname, subc->name))
					return subc->value;
		}
	return NULL;
}

int libwr_cfg_convert2(char *cfgname, char *subname,
			 enum libwr_convert conv, void *ret, ...)
{
	double *dptr = ret;
	int *iptr = ret;
	char real_cfgname[128];
	char *value;
	va_list args;
	char ch;

	va_start(args, ret);
	vsprintf(real_cfgname, cfgname, args);
	va_end(args);

	value = libwr_cfg_get2(real_cfgname, subname);
	if (!value) {
		errno = ENOENT;
		return -1;
	}
	errno = EINVAL;
	switch(conv) {
	case LIBWR_INT:
		if (sscanf(value, "%i%c", iptr, &ch) != 1)
			return -1;
		return 0;
	case LIBWR_STRING:
		strcpy(ret, value);
		return 0;
	case LIBWR_DOUBLE:
		if (sscanf(value, "%lf%c", dptr, &ch) != 1)
			return -1;
		return 0;
	}
	return -1; /* never */
}
