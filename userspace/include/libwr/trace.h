#ifndef __LIBWR_TRACE_H
#define __LIBWR_TRACE_H

#define TRACE_INFO 0x10
#define TRACE_INFO_L1 0x11
#define TRACE_INFO_L2 0x12

#define TRACE_ERROR 2
#define TRACE_FATAL 3

#ifdef DEBUG
#define TRACE(...) trace_printf(__FILE__, __LINE__, __VA_ARGS__)
#else
#define TRACE(...)
#endif

void trace_log_stderr();
void trace_log_file(const char *filename);
void trace_printf(const char *fname, int lineno, int level, const char *fmt, ...);

#endif /* __LIBWR_TRACE_H */
