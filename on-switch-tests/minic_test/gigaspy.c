#include <stdio.h> 
#include <stdlib.h> 
#include <sys/time.h>

#include <hw/switch_hw.h> 
#include <hw/gigaspy_regs.h> 

#include "gigaspy.h"


static inline void _gspy_writel(shw_gigaspy_context_t *gspy, uint32_t reg, uint32_t val)
{
  *(volatile uint32_t *) (gspy->base + reg) = val;
}

static inline uint32_t _gspy_readl(shw_gigaspy_context_t *gspy, uint32_t reg)
{
  uint32_t tmp = *(volatile uint32_t *)(gspy->base+reg);
  return tmp;
}

shw_gigaspy_context_t *shw_gigaspy_init(uint32_t base_addr, int buffer_size)
{
  shw_gigaspy_context_t *ctx;
  
  ctx = shw_malloc(sizeof(shw_gigaspy_context_t));

  ctx->base = (void*)_fpga_base_virt + base_addr;
  ctx->buf_size = buffer_size;
  return ctx;
}


void shw_gigaspy_configure(shw_gigaspy_context_t *gspy, int mode, int trig_source, int trig0, int trig1, int num_samples)
{
  uint32_t gsctl = 0;

  _gspy_writel(gspy, GSPY_REG_GSNSAMPLES, num_samples);

  gspy->cur_nsamples = num_samples;

  if(trig0 == TRIG_UNUSED && trig1 == TRIG_UNUSED)
    _gspy_writel(gspy, GSPY_REG_GSTRIGCTL,  0);  // no trigger (free-running mode)
  else if (trig1 == TRIG_UNUSED)
    _gspy_writel(gspy, GSPY_REG_GSTRIGCTL,  trig0 | GSPY_GSTRIGCTL_TRIG0_EN(1));
  else
    _gspy_writel(gspy, GSPY_REG_GSTRIGCTL,  
	       trig0 | GSPY_GSTRIGCTL_TRIG0_EN(1) |
	       (trig1<<16)  | GSPY_GSTRIGCTL_TRIG1_EN(1));

  gsctl = GSPY_GSCTL_CH0_ENABLE(1) | GSPY_GSCTL_CH1_ENABLE(1);
  
  if(trig_source == GIGASPY_CH0)
	gsctl |= GSPY_GSCTL_LOAD_TRIG0(1);// | GSPY_GSCTL_SLAVE0_ENABLE(1);
  else
  	gsctl |= GSPY_GSCTL_LOAD_TRIG1(1);// | GSPY_GSCTL_SLAVE1_ENABLE(1);


  gspy->cur_trig_src = trig_source;
  
  printf("gsctl = %x\n", gsctl);

  printf("trigctl = %x\n", _gspy_readl(gspy, GSPY_REG_GSTRIGCTL));

  _gspy_writel(gspy, GSPY_REG_GSCTL, gsctl);
}


void shw_gigaspy_arm(shw_gigaspy_context_t *gspy)
{
  uint32_t gsctl = _gspy_readl(gspy, GSPY_REG_GSCTL);

  gsctl |= GSPY_GSCTL_RESET_TRIG0(1) | GSPY_GSCTL_RESET_TRIG1(1);

  _gspy_writel(gspy, GSPY_REG_GSCTL, gsctl);  
}

int shw_gigaspy_poll(shw_gigaspy_context_t *gspy)
{
  uint32_t gsstat = _gspy_readl(gspy, GSPY_REG_GSSTAT);

// printf("GSSTAT: %x\n", gsstat);

  if(gspy->cur_trig_src == GIGASPY_CH0) 
	return GSPY_GSSTAT_TRIG_DONE0(gsstat);
  else
  	return GSPY_GSSTAT_TRIG_DONE1(gsstat);



}


  

static void read_gsbuf(shw_gigaspy_context_t *gspy, int ch, uint32_t addr, int n, uint32_t *buf)
{
  uint32_t p, base = (ch == GIGASPY_CH0 ? (4*gspy->buf_size): (8*gspy->buf_size));
  int i;

  printf("baseaddr %x\n", base);

  for(i=0;i<n;i++)
  {
	p = addr+i*4;
	p &= ((4*gspy->buf_size)-1);
	buf[i]=_gspy_readl(gspy, base+p);
  }
}


static void hexprint(int start_addr, uint32_t *buf, int size)
{
  int nempty = 0;
  int n=0,i=0;
  
  nempty = (start_addr % 16);

  while(n<size)
  {
	if((i%16)==0)printf("\n%03x: ", start_addr + n);
	if(nempty)
	{
	  printf("    ");
	  nempty--;
	} else {

	int pos = (n+start_addr)&0x1fff;
	  printf(" %c%02x", buf[pos]&0x100?'K':' ', buf[pos]&0xff);
	  n++;
	  
	}
	i++; 
  }
  printf("\n");
}

static void dump_packet(int offs, int n, uint32_t *buf)
{
  hexprint(offs, buf, n);
}

#define buf_get(x) buf[(offs+(x))&0x1fff]

static void dump_packet_ether(int offs, int n, uint32_t *buf)
{
  int sfd_pos=-1;
  int efd_pos=-1;
  int i;
  int fstart=0;
  for(i=0;i<n;i++)
  {
	if((buf_get(i) == 0x1fb) && (sfd_pos<0)) sfd_pos = i;
	if((buf_get(i) == 0x1fd) && (efd_pos<0)) efd_pos = i;
  }

  if(efd_pos < 0) {printf("dump_frame(): no EPD\n");return ;	}
  if(sfd_pos < 0) {printf("dump_frame(): no SFD\n");return ;	}
  if(sfd_pos >= efd_pos){printf("dump_frame(): SFD>EPD\n");return ;	}

  for(i=sfd_pos+1; i<sfd_pos+6;i++) if(buf_get(i)!=0x55) { printf("Adump_frame(): invalid preamble\n");return ;	}

  if((buf_get(sfd_pos+6) == 0x55) && (buf_get(sfd_pos+7) == 0xd5))fstart = sfd_pos+8;
  if((buf_get(sfd_pos+6) == 0xd5)) fstart = sfd_pos+7;
  
  if(!fstart) { printf("Bdump_frame(): invalid preamble\n"); return ;}

  printf("ETHER DSTMac=%02x:%02x:%02x:%02x:%02x:%02x SRCMac=%02x:%02x:%02x:%02x:%02x:%02x Ethertype=0x%04x\n", 
				buf_get(sfd_pos+8),	buf_get(sfd_pos+9),buf_get(sfd_pos+10),buf_get(sfd_pos+11),buf_get(sfd_pos+12),buf_get(sfd_pos+13),
				buf_get(sfd_pos+14),	buf_get(sfd_pos+15),buf_get(sfd_pos+16),buf_get(sfd_pos+17),buf_get(sfd_pos+18),buf_get(sfd_pos+19),
				(buf_get(sfd_pos+20) << 8) + buf_get(sfd_pos+21));



}

void shw_gigaspy_dump(shw_gigaspy_context_t *gspy, int pretrigger, int num_samples, int mode, int channels)
{
  uint32_t buf_ch0[GIGASPY_MAX_MEM_SIZE];
  uint32_t buf_ch1[GIGASPY_MAX_MEM_SIZE];

  uint32_t gstrigaddr = _gspy_readl(gspy, GSPY_REG_GSTRIGADDR);
  uint32_t addr_ch0 = GSPY_GSTRIGADDR_CH0(gstrigaddr);
  uint32_t addr_ch1 = GSPY_GSTRIGADDR_CH1(gstrigaddr);
  
  read_gsbuf(gspy, GIGASPY_CH0, 0, gspy->buf_size, buf_ch0);
  read_gsbuf(gspy, GIGASPY_CH1, 0, gspy->buf_size, buf_ch1);

  if(channels & GIGASPY_CH0)
  {
    printf("---> CH0 dump: ");
    if(mode == GIGASPY_DUMP_RAW)

//      dump_packet(0, pretrigger + num_samples, buf_ch0 );

      dump_packet((addr_ch0-gspy->cur_nsamples-pretrigger)&(gspy->buf_size-1), pretrigger+num_samples, buf_ch0 );

    else
      dump_packet_ether((addr_ch0-gspy->cur_nsamples-pretrigger)&0x1fff, pretrigger+num_samples, buf_ch0 );
  }
  
  if(channels & GIGASPY_CH1)
  {
  
    printf("---> CH1 dump: ");
    if(mode == GIGASPY_DUMP_RAW)
      dump_packet((addr_ch1-gspy->cur_nsamples-pretrigger)&0x1fff, pretrigger+num_samples, buf_ch1 );
    else
      dump_packet_ether((addr_ch1-gspy->cur_nsamples-pretrigger)&0x1fff, pretrigger+num_samples, buf_ch1 );
  }
  
}	
