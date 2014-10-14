/* Some debug/tracing stuff */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <trace.h>

static FILE *trace_file = NULL;
static int trace_to_stderr = 0;

void trace_log_stderr()
{
        trace_to_stderr = 1;
}

void trace_log_file(const char *filename)
{
        trace_file = fopen(filename, "wb");
}

void trace_printf(const char *fname, int lineno, int level, const char *fmt, ...)
{
    char linestr[40];
    char typestr[10];
    va_list vargs;

    switch (level)
    {
            case TRACE_INFO: strcpy(typestr, "(I)"); break;
            case TRACE_ERROR: strcpy(typestr, "(E)"); break;
            case TRACE_FATAL: strcpy(typestr, "(!)"); break;
            default: strcpy(typestr, "(?)"); break;
    }

    snprintf(linestr, 40, "%s [%s:%d]", typestr, fname, lineno);

    va_start(vargs, fmt);

    if(trace_file)
    {
        fprintf(trace_file, "%-24s ", linestr);
        vfprintf(trace_file, fmt, vargs);
        fflush(trace_file);
		fprintf(trace_file,"\n");
	}

    if(trace_to_stderr)
    {
        fprintf(stderr, "%-24s ", linestr);
        vfprintf(stderr, fmt, vargs);
		fprintf(stderr,"\n");
    }

    va_end(vargs);
}
