#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include <signal.h>

#include <hw/switch_hw.h>
#include <hw/clkb_io.h>
#include <hw/minic_regs.h>
#include <hw/endpoint_regs.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
//#include <net/if.h>

#include <asm/types.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <linux/errqueue.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <errno.h>

#define MINIC_PBUF_SIZE_LOG2 (12)
#define MINIC_PBUF_SIZE (1<<MINIC_PBUF_SIZE_LOG2)

#define MINIC_BASE_ENDPOINT (1<<(MINIC_PBUF_SIZE_LOG2+2))



static inline void _ep_writel(uint32_t reg, uint32_t value)
{
  _fpga_writel(FPGA_BASE_MINIC_UP1 + MINIC_BASE_ENDPOINT + reg, value);
}

static inline uint32_t _ep_readl(uint32_t reg)
{
  return _fpga_readl(FPGA_BASE_MINIC_UP1 + MINIC_BASE_ENDPOINT + reg);
}

void endpoint_init()
{
  // printf("Endpoint_init, base addr = %x\n", MINIC_BASE_ENDPOINT);

    _ep_writel(EP_REG_ECR, 0); // ECR = 0?
  _ep_writel(EP_REG_TCR, EP_TCR_EN_FRA | EP_TCR_EN_PCS); // enable TX framer + PCS
  _ep_writel(EP_REG_RCR, EP_RCR_EN_FRA | EP_RCR_EN_PCS); // enable RX framer + PCS
  _ep_writel(EP_REG_RFCR, 3 << EP_RFCR_QMODE_SHIFT); // QMODE = UNQUALIFIED
  _ep_writel(EP_REG_MACH, 0xaabb);  // assign a dummy MAC address
  _ep_writel(EP_REG_MACL, 0xccddeeff);
  _ep_writel(EP_REG_TSCR, 0);
  _ep_writel(EP_REG_PHIO, EP_PHIO_ENABLE | EP_PHIO_SYNCEN); // enable the PHY

  

  shw_pio_set0(PIN_up1_sfp_tx_disable); // enable the SFP
  shw_pio_set0(PIN_up0_sfp_tx_disable); // enable the SFP


    	fprintf(stderr,"EP: waiting for sync...\n");
	//  while(!(_ep_readl(EP_REG_RCR) & EP_RCR_SYNCED));

//  _ep_writel(EP_REG_TCR, EP_TCR_TX_CAL); // enable TX framer + PCS
 
// printf("LCW: %x\n", ep_rx_lcw(0));
// printf("LCW: %x\n", ep_rx_lcw(0));
 
   

  return ;

}

int shw_ad9516_set_output_delay(int output, float delay_ns, int bypass);

void dump_counters()
{
 const char *cntr_names[] = {
 "0x0 : TX PCS buffer underruns",
 "0x4 : RX PCS invalid 8b10b codes",
 "0x8 : RX PCS sync lost events",
 "0xc : RX PCS buffer overruns",
 "0x10: RX CRC errors",
 "0x14: RX valid frames",
 "0x18: RX runt frames",
 "0x1c: RX giant frames",
 "0x20: RX PCS errors",
 "0x24: RX dropped frames"
 };


 int i;
 
 for(i=0;i<10;i++) printf("%-30s: %d\n", cntr_names[i], _ep_readl(0x80 + 4*i));
}

uint64_t get_tics()
{
	struct timezone tz = {0,0};
	struct timeval tv;
	
	gettimeofday(&tv, &tz);

	return (uint64_t) tv.tv_sec * 1000000ULL + (uint64_t) tv.tv_usec;
}

#define SO_TXTS_FRAME_TAG	38


static int poll_tx_timestamp(int sock, uint32_t *raw_ts)
{
  char data[2048];

  struct msghdr msg;
  struct iovec entry;
  struct sockaddr_ll from_addr;
  struct {
    struct cmsghdr cm;
    char control[512];
  } control;
  int res;

  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = &entry;
  msg.msg_iovlen = 1;
  entry.iov_base = data;
  entry.iov_len = sizeof(data);
  msg.msg_name = (caddr_t)&from_addr;
  msg.msg_namelen = sizeof(from_addr);
  msg.msg_control = &control;
  msg.msg_controllen = sizeof(control);

  res = recvmsg(sock, &msg, MSG_ERRQUEUE | MSG_DONTWAIT);

  if(res >= 0)
    {
      fprintf(stderr,"GotErrQ\n");
    }

  return 0;
}


int test_tx_timestamps()
{
    uint64_t tstart;
    
    struct hwtstamp_config hwconfig;
    
    struct sockaddr_ll sll, dest_addr;
    struct ifreq f;
    const char *if_name = "wru1";
    const uint16_t if_ethertype = 0x88f7;
    int enable;
    int fd;
    int ts_flags;

    const uint8_t broadcast_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    const uint8_t my_mac[] = {0x00 ,0x50, 0xfc, 0x96, 0x9b, 0x0e};

    fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(fd < 0) { perror("socket()"); return -1; }

 
  //  strcpy(f.ifr_name, if_name);
//    if(ioctl(fd, SIOCGIFFLAGS,&f) < 0) { perror("ioctl()"); return NULL; }
//    f.ifr_flags |= IFF_PROMISC;
  //  if(ioctl(fd, SIOCSIFFLAGS,&f) < 0) { perror("ioctl()"); return NULL; }

    strcpy(f.ifr_name, if_name);
    ioctl(fd, SIOCGIFINDEX, &f);

    sll.sll_ifindex = f.ifr_ifindex; 
    sll.sll_family   = AF_PACKET;
    sll.sll_protocol = htons(if_ethertype);
    sll.sll_halen = 6;    


    if(bind(fd, (struct sockaddr *)&sll, sizeof(struct sockaddr_ll)) < 0)
    {
	close(fd);
	perror("bind()");
	return NULL;
    }
		
    fcntl(fd, F_SETFL, O_NONBLOCK);

    hwconfig.tx_type = HWTSTAMP_TX_ON;
    hwconfig.rx_filter = HWTSTAMP_FILTER_PTP_V2_L2_EVENT;


    memset(&f, 0, sizeof(struct ifreq));

    f.ifr_data = &hwconfig;
    strncpy(f.ifr_name, if_name, sizeof(f.ifr_name));

    if (ioctl(fd, SIOCSHWTSTAMP, &f) < 0) { perror("ioctl"); return -1; }

    enable = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &enable, sizeof(enable)) < 0)
      { perror("setsockopt()"); return -1; }
    if(setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPNS, &enable, sizeof(enable)) < 0)
      { perror("setsockopt()"); return -1; }

    ts_flags = SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;

    if(setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, &ts_flags, sizeof(ts_flags)) < 0)
      { perror("setsockopt()"); return -1; }

    tstart =get_tics();



    for(;;)
      {
	uint64_t t = get_tics();

	//	fprintf(stderr,"T-tstart %ld\n", t-tstart);

	if(t - tstart > 1000000ULL)
	  {
	    char buf[64];
	    fprintf(stderr, "Send!");
	    tstart=  t;

	    sll.sll_family = AF_PACKET;
	    sll.sll_protocol = htons(if_ethertype);
	    sll.sll_pkttype = PACKET_BROADCAST  ;
	    sll.sll_halen = 6;
	    
	    memcpy(buf, broadcast_addr, 6);
	    memcpy(buf+6, my_mac, 6);
	    buf[12] = if_ethertype >> 8;
	    buf[13] = if_ethertype & 0xff;


			write(fd, buf, 64);
//	    send_with_ts(fd, buf, 64,  &sll);
   		
	  }

	poll_tx_timestamp(fd, NULL);
      }
}

main()
{
  int i;
	system("/sbin/rmmod /tmp/wr-minic.ko");
	system("/sbin/rmmod /tmp/whiterabbit_vic.ko");

  trace_log_stderr();
  shw_init();
 
  xpoint_configure();
  endpoint_init();

//  shw_ad9516_set_output_delay(6, 0, 0);
  shw_ad9516_set_output_delay(9, 2, 0);


	system("/sbin/insmod /tmp/whiterabbit_vic.ko");
	system("/sbin/insmod /tmp/wr-minic.ko");
	system("/sbin/ifconfig wru1 hw ether 00:50:fc:96:9b:0e");
	system("/sbin/ifconfig wru1 up 192.168.100.100");


//	test_rxts_fifo();
	
//	test_tx_timestamps();


}



