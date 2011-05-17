
#include <stdio.h>

#define C_DIM 0x80
#define C_WHITE 7

#define C_GREY (C_WHITE | C_DIM)

#define C_RED 1
#define C_GREEN 2
#define C_BLUE 4

int term_restore();
void term_init();
int term_poll();
int term_get();
void term_cprintf(int color, const char *fmt, ...);
void term_pcprintf(int row, int col, int color, const char *fmt, ...);
void term_clear();
