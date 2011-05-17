#ifndef __GIGASPY_H
#define __GIGASPY_H

#include <stdio.h> 
#include <stdlib.h> 
#include <sys/time.h>

#include <hw/switch_hw.h> 

#define GIGASPY_CH0 0x1
#define GIGASPY_CH1 0x2

#define GIGASPY_KD(k,d) (((k)?(1<<8):0) | ((d) & 0xff))

#define GIGASPY_MAX_MEM_SIZE 16384

#define GIGASPY_DUMP_RAW 0
#define GIGASPY_DUMP_ETHER 1


#define TRIG_UNUSED -1

typedef struct {
  int buf_size;
  volatile void *base;
  int cur_trig_src;
  int cur_port;
  int cur_nsamples;
} shw_gigaspy_context_t;

shw_gigaspy_context_t *shw_gigaspy_init(uint32_t base_addr, int buffer_size);
void shw_gigaspy_configure(shw_gigaspy_context_t *gspy, int mode, int trig_source, int trig0, int trig1, int num_samples);
void shw_gigaspy_arm(shw_gigaspy_context_t *gspy);
int shw_gigaspy_poll(shw_gigaspy_context_t *gspy);
void shw_gigaspy_dump(shw_gigaspy_context_t *gspy, int pretrigger, int num_samples, int mode, int channels);

#endif
