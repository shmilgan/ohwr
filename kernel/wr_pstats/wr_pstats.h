#ifndef __WR_PSTATS_H__
#define __WR_PSTATS_H__

#define NPORTS 18 	/* how many eth ports are in the switch */
#define CNT_PP 17	/* how many counters per port */
#define ADR_PP ((CNT_PP+3)/4)	/* how many address words are there per counter 
				 * (each of them stores the state of 4 counters) */
#define CNT_LSB	16	/*how many bits are stored in hw*/
#define CNT_LSB_MSK 0x0000ffff
#define CNT_MSB_MSK 0xffff0000
#define IRQS_BUFSZ  16

#define FPGA_BASE_PSTATS 0x10057000
#define WR_PSTATS_IRQ	3
#define WRVIC_BASE_IRQ  (NR_AIC_IRQS + (5 * 32))

#define MSK_ALLPORTS 0xffffffff
//#define MSK_8PORTS   0xff
//#define MSK_18PORTS  0x3ffff

//#define MSK_PORTS(port) (2^(port) - 1)


static int pstats_rd_cntrs(uint8_t port);

const uint8_t portnums[18] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
const char* portnames[18]  = {"port0", "port1", "port2", "port3", "port4", "port5", "port6", 
	"port7", "port8", "port9", "port10", "port11", "port12", "port13", "port14",
	"port15", "port16", "port17"};


#endif
