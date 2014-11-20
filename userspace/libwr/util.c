#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <libwr/trace.h>
#include <libwr/util.h>

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


/**
 * \brief Helper function to quickly display byte into binary code.
 * WARNING: this returns static storage
 */
const char *shw_2binary(uint8_t x)
{
    static char b[9];
    int z;
    char *p=b;

    for (z=0x80; z > 0; z >>= 1)
    {
       *p++=(((x & z) == z) ? '1' : '0');
    }
    *p='\0';

    return b;
}

