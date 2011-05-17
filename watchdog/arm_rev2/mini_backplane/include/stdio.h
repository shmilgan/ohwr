#ifndef __STDIO_STUB_H
#define __STDIO_STUB_H

#include <stdarg.h>

#define EOF (-1)

typedef void* FILE;
typedef int size_t;

extern FILE *stdout;
extern FILE *stderr;

int snprintf(char *pString, size_t length, const char *pFormat, ...);
int vsprintf(char *pString, const char *pFormat, va_list ap);
int vfprintf(FILE *pStream, const char *pFormat, va_list ap);
int vprintf(const char *pFormat, va_list ap);
int fprintf(FILE *pStream, const char *pFormat, ...);
int printf(const char *pFormat, ...);
int sprintf(char *pStr, const char *pFormat, ...);
signed int puts(const char *pStr);

int raise(int sig);

#endif
