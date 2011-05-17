#ifndef __GIGASPY_REGS_H
#define __GIGASPY_REGS_H
// GigaSpy wishbone registers

#define GSPY_REG_GSCTL 			0x0
#define GSPY_REG_GSTRIGCTL 		0x4
#define GSPY_REG_GSSTAT 		0x8
#define GSPY_REG_GSNSAMPLES 	0xc
#define GSPY_REG_GSTRIGADDR 	0x10
#define GSPY_REG_GSTESTCTL   	0x14
#define GSPY_REG_GSTESTCNT   	0x18
#define GSPY_REG_GSCLKFREQ   	0x1c

#define GSPY_GSCTL_CH0_ENABLE(x) ((x)?(1<<0):(0))
#define GSPY_GSCTL_CH1_ENABLE(x) ((x)?(1<<1):(0))
#define GSPY_GSCTL_SLAVE0_ENABLE(x) ((x)?(1<<2):(0))
#define GSPY_GSCTL_SLAVE1_ENABLE(x) ((x)?(1<<3):(0))
#define GSPY_GSCTL_LOAD_TRIG0(x) ((x)?(1<<4):(0))
#define GSPY_GSCTL_LOAD_TRIG1(x) ((x)?(1<<5):(0))
#define GSPY_GSCTL_RESET_TRIG0(x) ((x)?(1<<6):(0))
#define GSPY_GSCTL_RESET_TRIG1(x) ((x)?(1<<7):(0))

#define GSPY_GSTRIGCTL_TRIG0_VAL(k,v) (((k)?(1<<8):0) | ((v) & 0xff))
#define GSPY_GSTRIGCTL_TRIG1_VAL(k,v) (((k)?(1<<24):0) | (((v) & 0xff)<<16))

#define GSPY_GSTRIGCTL_TRIG0_EN(x) (((x)?(1<<15):0))
#define GSPY_GSTRIGCTL_TRIG1_EN(x) (((x)?(1<<31):0))

#define GSPY_GSNSAMPLES(x) ((x)&0x1fff)

#define GSPY_GSSTAT_TRIG_DONE0(reg) ((reg)&(1<<0)?1:0)
#define GSPY_GSSTAT_TRIG_DONE1(reg) ((reg)&(1<<1)?1:0)

#define GSPY_GSSTAT_TRIG_SLAVE0(reg) ((reg)&(1<<2)?1:0)
#define GSPY_GSSTAT_TRIG_SLAVE1(reg) ((reg)&(1<<3)?1:0)

#define GSPY_GSTRIGADDR_CH0(reg) ((reg)&0x1fff)
#define GSPY_GSTRIGADDR_CH1(reg) (((reg)>>16)&0x1fff)

#define GSPY_GSTESTCTL_ENABLE   0x1
#define GSPY_GSTESTCTL_RST_CNTR  0x2
#define GSPY_GSTESTCTL_CONNECT   0x4

#define GSPY_GSTESTCTL_PHYIO_ENABLE 0x100
#define GSPY_GSTESTCTL_PHYIO_SYNCEN 0x200
#define GSPY_GSTESTCTL_PHYIO_LOOPEN 0x400
#define GSPY_GSTESTCTL_PHYIO_PRBSEN 0x800

#endif
