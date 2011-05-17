// Similation of real WR network interface with hardware timestamping. Supports only raw ethernet now.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ptpd_netif.h"

char *format_mac_addr(mac_addr_t mac)
{
  char buf[32];
  snprintf(buf,32,"%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return strdup(buf);
}



char *format_wr_timestamp(wr_timestamp_t ts)
{
  char buf[64];

  snprintf(buf,64, "rising: %09d nsec, failling: %03d nsec", ts.wr.r_cntr * 8, ts.wr.f_cntr * 8);
  return strdup(buf);
}

// peer delay multicast address
const mac_addr_t PTP_MULTICAST_PDELAY[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e};

main(int argc, char *argv[])
{
  wr_sockaddr_t bindaddr, to_addr, from_addr;
  wr_socket_t *sock;
  char buf[1518];
  int len;
    
  if(argc < 2)
    {
      printf("usage: %s network_interface. Run me as root!\n\n", argv[0]);
      return -1;
    }


  // Initialize the netif library
  ptpd_netif_init();

  // Create a PTP socket:
  bindaddr.if_name = strdup(argv[1]);		// network intarface
  bindaddr.family = PTPD_SOCK_RAW_ETHERNET;	// socket type
  bindaddr.ethertype = 0x88f7; 		// PTPv2
  memset(bindaddr.mac, 0, 6); 		// bind to any address (e.g. accepting everything which has 88f7 type)

  // Create the socket
  sock = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET, 0, &bindaddr);

  // Set destination address
  memcpy(to_addr.mac, PTP_MULTICAST_PDELAY, 6);

  // and ethertype
  to_addr.ethertype = 0x88f7;
    
  for(;;)
    {
      wr_frame_tag_t tag;
      wr_timestamp_t tx_ts, rx_ts;

      // Send a frame and get its TX timestamp
      ptpd_netif_sendto(sock, &to_addr, buf, 64, &tag);

      if(ptpd_netif_poll_tx_timestamp(sock, tag, &tx_ts) == PTPD_NETIF_OK) {
	fprintf(stderr, "TX timestamp: %s\n", format_wr_timestamp(tx_ts));
      }
      sleep(1);

      // receive any incoming frames and their timestamps	
      while( (len = ptpd_netif_recvfrom(sock, &from_addr, buf, 1518, &rx_ts)) > 0)
	{
	  printf("RX frame: from=%s, to=%s, type=0x%04x, timestamp: %s\n", 
		 format_mac_addr(from_addr.mac), 
		 format_mac_addr(from_addr.mac_dest),
		 from_addr.ethertype, 
		 format_wr_timestamp(rx_ts));
	}

    }
    
  return 0;
}
