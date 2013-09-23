
#include <stdio.h>

#define C_DIM 0x80
#define C_WHITE 7

#define C_GREY (C_WHITE | C_DIM)

#define C_RED 1
#define C_GREEN 2
#define C_BLUE 4

void term_restore(void);
void term_init(int usecolor);
int term_poll(int msec_timeout);
int term_get(void);
void term_cprintf(int color, const char *fmt, ...);
void term_pcprintf(int row, int col, int color, const char *fmt, ...);
void term_clear(void);
