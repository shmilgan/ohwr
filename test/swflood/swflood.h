
/* Prototypes and data structures for switch flooding */

#ifndef __SWFLOOD_H__
#define __SWFLOOD_H__

#include <linux/if_ether.h>

struct swf_if {
	char *name;
	int socket;
	int stat;
	int s_incr, d_incr;
	unsigned char s_addr[ETH_ALEN], d_addr[ETH_ALEN];
};

struct swf_command;
struct swf_status;

struct swf_line {
	struct swf_command *cmd;
	int argc;
	char **argv;
	void *data;
	char *fname;
	int lineno;
	int repcount;
	struct swf_line *prev, *next;
};

struct swf_command {
	char *name;
	int (*verify)(struct swf_line *);
	int (*verify_late)(struct swf_line *);
	struct swf_line *(*execute)(struct swf_line *, struct swf_status *);
};

struct swf_status {
	struct swf_line *line;
	struct swf_if *iface;
	int n_iface;
	int verbose;
	FILE *logfile;
	int error;
};

/* each command */
#define declare_command(x) \
    static struct swf_command * \
    __attribute__((__used__)) \
    __attribute__((section(".commands"))) \
    __cmd_ptr_##x = &x

/* parse.c */
int swf_parse(char *s, struct swf_line *line);

/* commands.lds */
extern struct swf_command *__cmd_start[];
extern struct swf_command *__cmd_end[];

#endif /* __SWFLOOD_H__ */
