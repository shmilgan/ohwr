// Similation of real WR network interface with hardware timestamping. Supports only raw ethernet now.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/net_tstamp.h>
#include <linux/errqueue.h>
#include <linux/sockios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <fcntl.h>
#include <errno.h>

#include <asm/socket.h>

#include "ptpd_netif.h"

const uint8_t tag_trailer_id[] = {0xde, 0xad, 0xbe, 0xef};

#define ETHER_MTU 1518

#define TX_TS_KEEPALIVE 1000000ULL
#define TX_TS_QUEUE_SIZE 128

const mac_addr_t PTP_MULTICAST_MAC[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e};


struct scm_timestamping {
  struct timespec systime;
  struct timespec hwtimetrans;
  struct timespec hwtimeraw;
};


PACKED struct etherpacket {
    struct ethhdr ether;
    char data[ETHER_MTU];
}; 

struct tx_timestamp {
  int valid;
  wr_timestamp_t ts;
  uint32_t tag;
  uint64_t t_acq;
};

struct my_socket {
  int fd;
  wr_sockaddr_t bind_addr;
  mac_addr_t local_mac;
  int if_index;
  uint32_t current_tag;
  struct tx_timestamp tx_ts_queue[TX_TS_QUEUE_SIZE];
};

static uint64_t get_tics()
{
  struct timezone tz = {0, 0};
  struct timeval tv;
  gettimeofday(&tv, &tz);

  return (uint64_t) tv.tv_sec * 1000000ULL + (uint64_t) tv.tv_usec;
}

int ptpd_netif_init()
{
    return PTPD_NETIF_OK;
}

wr_socket_t *ptpd_netif_create_socket(int sock_type, int flags, wr_sockaddr_t *bind_addr)
{
    struct my_socket *s;
    struct sockaddr_ll sll;
    int fd;
    
    if(sock_type != PTPD_SOCK_RAW_ETHERNET)
	return NULL;

    struct ifreq f;

    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if(fd < 0) 
    {
	perror("socket()");
	return NULL;
    }

    fcntl(fd, F_SETFL, O_NONBLOCK);

// Put the controller in promiscious mode, so it receives everything
    strcpy(f.ifr_name, bind_addr->if_name);
    if(ioctl(fd, SIOCGIFFLAGS,&f) < 0) { perror("ioctl()"); return NULL; }
    f.ifr_flags |= IFF_PROMISC;
    if(ioctl(fd, SIOCSIFFLAGS,&f) < 0) { perror("ioctl()"); return NULL; }

// Find the inteface index
    strcpy(f.ifr_name, bind_addr->if_name);
    ioctl(fd, SIOCGIFINDEX, &f);


    sll.sll_ifindex = f.ifr_ifindex; 
    sll.sll_family   = AF_PACKET;
    sll.sll_protocol = htons(bind_addr->ethertype);
    sll.sll_halen = 6;
    
    memcpy(sll.sll_addr, bind_addr->mac, 6);
   
    if(bind(fd, (struct sockaddr *)&sll, sizeof(struct sockaddr_ll)) < 0)
    {
	close(fd);
	perror("bind()");
	return NULL;
    }

    // timestamping stuff:
    
    int so_timestamping_flags = SOF_TIMESTAMPING_TX_HARDWARE | 
      SOF_TIMESTAMPING_RX_HARDWARE | 
      SOF_TIMESTAMPING_RAW_HARDWARE;

    struct ifreq ifr;
    struct hwtstamp_config hwconfig;

    strncpy(ifr.ifr_name, bind_addr->if_name, sizeof(ifr.ifr_name));


    hwconfig.tx_type = HWTSTAMP_TX_ON;
    hwconfig.rx_filter = HWTSTAMP_FILTER_PTP_V2_L2_EVENT;

    ifr.ifr_data = &hwconfig;

    if (ioctl(fd, SIOCSHWTSTAMP, &ifr) < 0) 
      {
	perror("SIOCSHWTSTAMP");
	return NULL;
      }
    
    if(setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, &so_timestamping_flags, sizeof(int)) < 0)
      {
	perror("setsockopt(SO_TIMESTAMPING)");
	return NULL;
      }
    
    s=malloc(sizeof(struct my_socket));
    memset(s, 0, sizeof(struct my_socket));

    s->if_index = f.ifr_ifindex;

// get interface MAC address 
    if (ioctl(fd, SIOCGIFHWADDR, &f) < 0) { perror("ioctl()"); return NULL; }

    memcpy(s->local_mac, f.ifr_hwaddr.sa_data, 6);
    memcpy(&s->bind_addr, bind_addr, sizeof(wr_sockaddr_t));

    s->fd = fd;
    s->current_tag = 0;

    return (wr_socket_t*)s;
}

int ptpd_netif_sendto(wr_socket_t *sock, wr_sockaddr_t *to, void *data, size_t data_length, wr_frame_tag_t *tag)
{
    struct etherpacket pkt;
    struct my_socket *s = (struct my_socket *)sock;
    struct sockaddr_ll sll;
    uint32_t our_tag;
    int rval;
    char buf[ETHER_MTU+8];

    if(s->bind_addr.family != PTPD_SOCK_RAW_ETHERNET)
	return -ENOTSUP;
	
    if(data_length > ETHER_MTU-8) return -EINVAL;
    
    if(tag) // user wants the frame tag for retreiving the timestamp: put it at the end of the frame data (ugly hack, but the current linux Timestamping API sucks anyway....)
      {
	memcpy(pkt.data + data_length, tag_trailer_id, 4);
	memcpy(pkt.data + data_length + 4, &s->current_tag, 4);
	our_tag = s->current_tag++;
      };

    memcpy(pkt.ether.h_dest, to->mac, 6);
    memcpy(pkt.ether.h_source, s->local_mac, 6);
    pkt.ether.h_proto =htons(to->ethertype);

    memcpy(pkt.data, data, data_length);

    size_t len = data_length + sizeof(struct ethhdr) + (tag ? 8 : 0);

    memset(&sll, 0, sizeof(struct sockaddr_ll));

    sll.sll_ifindex = s->if_index;
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(to->ethertype);
    sll.sll_halen = 6;

    rval =  sendto(s->fd, &pkt, len, 0, (struct sockaddr *)&sll, sizeof(struct sockaddr_ll));
    
    if(rval > 0 && tag) {
      *tag = our_tag;
    }
    return rval;
}

void age_ts_queue(struct my_socket *s)
{
  int i;
  uint64_t t = get_tics();

  for(i=0; i<TX_TS_QUEUE_SIZE; i++)
    if(s->tx_ts_queue[i].valid && (t - s->tx_ts_queue[i].t_acq) > TX_TS_KEEPALIVE)
      s->tx_ts_queue[i].valid = 0;
}

static void hdump(uint8_t *buf, int size)
{
  int i;
  printf("Dump: ");
  for(i=0;i<size;i++) printf("%02x ", buf[i]);
  printf("\n");
}


static void add_tx_timestamp(struct my_socket *s, uint32_t rtag, struct scm_timestamping *hwts, struct sock_extended_err *serr)
{
  uint32_t raw_ts =* (uint64_t *) &hwts->hwtimeraw >> 32; // this is white rabbit raw timestamp. NOT a TIMESPEC!

  wr_timestamp_t ts;
  int done = 0;
  int i;

  ts.wr.r_cntr = raw_ts & 0xfffffff;
  ts.wr.f_cntr = (raw_ts>>28) & 0xf;

  for(i=0;i<TX_TS_QUEUE_SIZE;i++)
    {
      struct tx_timestamp *tt = &s->tx_ts_queue[i];
      if(!tt->valid)
	{
	  tt->t_acq = get_tics();
	  tt->valid = 1;
	  tt->tag = rtag;
	  tt->ts = ts;
	  done = 1;
	  break;
	}
    }
  
  if(!done)
    fprintf(stderr,"WARNING: TX timestamp queue full\n!");
}


int ptpd_netif_poll_tx_timestamp(wr_socket_t *sock, wr_frame_tag_t tag, wr_timestamp_t *tx_timestamp)
{
  char data[ETHER_MTU+8];
  
  struct my_socket *s = (struct my_socket *) sock;
  struct msghdr msg;
  struct iovec entry;
  struct sockaddr_ll from_addr;
  struct {
    struct cmsghdr cm;
    char control[1024];
  } control;
  struct cmsghdr *cmsg;
  int res;
  uint32_t rtag;
  int i;

  struct sock_extended_err *serr = NULL;
  struct scm_timestamping *sts = NULL;

  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = &entry;
  msg.msg_iovlen = 1;
  entry.iov_base = data;
  entry.iov_len = sizeof(data);
  msg.msg_name = (caddr_t)&from_addr;
  msg.msg_namelen = sizeof(from_addr);
  msg.msg_control = &control;
  msg.msg_controllen = sizeof(control);
	
  res = recvmsg(s->fd, &msg, MSG_ERRQUEUE|MSG_DONTWAIT);

  age_ts_queue(s);
  

  if(res < 8) return PTPD_NETIF_NOT_READY;

  //  hdump(&control, 128);

  if(res >= 0)
    {
      if(memcmp(data + res - 8, tag_trailer_id, 4)) 
	return PTPD_NETIF_NOT_READY;


      memcpy(&rtag, data+res-4, 4);

	for (cmsg = CMSG_FIRSTHDR(&msg);
	     cmsg;
	     cmsg = CMSG_NXTHDR(&msg, cmsg)) {
	  
	  void *dp = CMSG_DATA(cmsg);

	  //	  	  printf("cmsg %x lev %d typ %d len %d\n", cmsg, cmsg->cmsg_level, cmsg->cmsg_type, cmsg->cmsg_len);

	  if(cmsg->cmsg_level == SOL_PACKET && cmsg->cmsg_type == PACKET_TX_TIMESTAMP)
	    serr = (struct sock_extended_err *) dp;

	  if(cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING)
	    sts = (struct scm_timestamping *) dp;
	  
	  //	   printf("Serr %x sts %x\n", serr, sts);

	   if(serr && sts)
	     {
	       add_tx_timestamp(s, rtag, sts, serr);
	       break;
	     }
	}
    }


  for(i=0;i<TX_TS_QUEUE_SIZE; i++)
    {   
      struct tx_timestamp *tt = &s->tx_ts_queue[i];
      if(tt->valid && tt->tag == tag)
	{
	  *tx_timestamp = tt->ts;
	  tt->valid = 0;
	  return PTPD_NETIF_OK;
	}
    }

  return 0;
}

int ptpd_netif_recvfrom(wr_socket_t *sock, wr_sockaddr_t *from, void *data, size_t data_length, wr_timestamp_t *rx_timestamp)
{
    struct my_socket *s = (struct my_socket *)sock;
    struct etherpacket pkt;
    struct msghdr msg;
    struct iovec entry;
    struct sockaddr_ll from_addr;
    struct {
      struct cmsghdr cm;
      char control[1024];
    } control;
    struct cmsghdr *cmsg;
    struct scm_timestamping *sts;
    int res;

    size_t len = data_length + sizeof(struct ethhdr);

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &entry;
    msg.msg_iovlen = 1;
    entry.iov_base = &pkt;
    entry.iov_len = len;
    msg.msg_name = (caddr_t)&from_addr;
    msg.msg_namelen = sizeof(from_addr);
    msg.msg_control = &control;
    msg.msg_controllen = sizeof(control);

    int ret = recvmsg(s->fd, &msg, MSG_DONTWAIT);
    
    if(ret < 0 && errno==EAGAIN) return 0; // would be blocking
    if(ret == -EAGAIN) return 0;
    
    if(ret <= 0) return ret;

    memcpy(data, pkt.data, ret - sizeof(struct ethhdr));
    
    from->ethertype = ntohs(pkt.ether.h_proto);
    memcpy(from->mac, pkt.ether.h_source, 6);
    memcpy(from->mac_dest, pkt.ether.h_dest, 6);

    fprintf(stderr, "recvmsg: ret %d\n", ret);

    for (cmsg = CMSG_FIRSTHDR(&msg);
	 cmsg;
	 cmsg = CMSG_NXTHDR(&msg, cmsg)) {
	  
      void *dp = CMSG_DATA(cmsg);

      if(cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING)
	sts = (struct scm_timestamping *) dp;

    }

    if(sts && rx_timestamp)
      {
	uint32_t raw_ts =(* (uint64_t *) &sts->hwtimeraw) >> 32; // this is white rabbit raw timestamp. NOT a TIMESPEC!


	rx_timestamp->wr.r_cntr = raw_ts & 0xfffffff;
	rx_timestamp->wr.f_cntr = (raw_ts>>28) & 0xf;
      }

    return ret - sizeof(struct ethhdr);
}

