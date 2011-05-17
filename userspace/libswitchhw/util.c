#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <hw/trace.h>
#include <hw/util.h>

void shw_udelay(uint32_t microseconds)
{
    uint64_t t_start, t_cur;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    t_start = (uint64_t) tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;

    do {
    gettimeofday(&tv, NULL);
    t_cur = (uint64_t) tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;


    } while(t_cur <= t_start + (uint64_t) microseconds);
}

void *shw_malloc(size_t nbytes)
{
        void *p  = malloc(nbytes);

        if(!p)
        {
                TRACE(TRACE_FATAL, "malloc(%d) failed!", nbytes);
                exit(-1);
        }
        return p;
}

void shw_free(void *ptr)
{
        free(ptr);
}

uint64_t shw_get_tics()
{
  struct timeval tv;
  struct timezone tz={0,0};
  gettimeofday(&tv, &tz);
  return (uint64_t)tv.tv_usec + (uint64_t)tv.tv_sec * 1000000ULL;
}
