// Network API for WR-PTPd

#ifndef __LIBWR_PTPD_NETIF_H
#define __LIBWR_PTPD_NETIF_H

#include <stdio.h>
#include <net/ethernet.h>
#include <libwr/hal_shmem.h>
#include <libwr/hal_client.h>

#define PTPD_SOCK_RAW_ETHERNET 	1
#define PTPD_SOCK_UDP 		2

struct wr_socket; /* opaque to most users -- see LIBWR_INTERNAL */

// Socket address for ptp_netif_ functions
struct wr_sockaddr {
// Network interface name (eth0, ...)
	char if_name[16];
// Socket family (RAW ethernet/UDP)
	int family;
// MAC address
	uint8_t mac[ETH_ALEN];
// UDP port
	uint16_t port;
// RAW ethertype
	uint16_t ethertype;
// physical port to bind socket to
	uint16_t physical_port;
};

#ifdef LIBWR_INTERNAL /* The following two used to be in ptpd_netif.c */
struct wr_tmo { /* timeout */
	uint64_t start_tics;
	uint64_t timeout;
};

struct wr_socket {
	int fd;
	struct wr_sockaddr bind_addr;
	uint8_t local_mac[ETH_ALEN];
	int if_index;

	// parameters for linearization of RX timestamps
	uint32_t clock_period;
	uint32_t phase_transition;
	uint32_t dmtd_phase;
	int dmtd_phase_valid;
	struct wr_tmo dmtd_update_tmo;
};
#endif

struct wr_tstamp {

	// Seconds
	int64_t sec;

	// Nanoseconds
	int32_t nsec;

	// Phase (in picoseconds), linearized for receive timestamps, zero for send timestamps
	int32_t phase;		// phase(picoseconds)

	/* Raw time (non-linearized) for debugging purposes */
	int32_t raw_phase;
	int32_t raw_nsec;
	int32_t raw_ahead;

	// correctness flag: when 0, the timestamp MAY be incorrect (e.g. generated during timebase adjustment)
	int correct;
	//int cntr_ahead;
};

/* OK. These functions we'll develop along with network card driver. You can write your own UDP-based stubs for testing purposes. */

// Initialization of network interface:
// - opens devices
// - does necessary ioctls()
// - initializes connection with the mighty HAL daemon
int ptpd_netif_init(void);

// Creates UDP or Ethernet RAW socket (determined by sock_type) bound to bind_addr. If PTPD_FLAG_MULTICAST is set, the socket is
// automatically added to multicast group. User can specify physical_port field to bind the socket to specific switch port only.
struct wr_socket *ptpd_netif_create_socket(int sock_type, int flags,
				      struct wr_sockaddr *bind_addr,
				      struct hal_port_state *port);

// Sends a UDP/RAW packet (data, data_length) to address provided in wr_sockaddr
// For raw frames, mac/ethertype needs to be provided, for UDP - ip/port.
// Every transmitted frame has assigned a tag value, stored at tag parameter. This value is later used
// for recovering the precise transmit timestamp. If user doesn't need it, tag parameter can be left NULL.

int ptpd_netif_sendto(struct wr_socket *sock, struct wr_sockaddr *to, void *data,
		      size_t data_length, struct wr_tstamp *tx_ts);

// Receives an UDP/RAW packet. Data is written to (data) and length is returned. Maximum buffer length can be specified
// by data_length parameter. Sender information is stored in structure specified in 'from'. All RXed packets are timestamped and the timestamp
// is stored in rx_timestamp (unless it's NULL).
int ptpd_netif_recvfrom(struct wr_socket *sock, struct wr_sockaddr *from, void *data,
			size_t data_length, struct wr_tstamp *rx_timestamp,
			struct hal_port_state *port);

// Closes the socket.
int ptpd_netif_close_socket(struct wr_socket *sock);

int ptpd_netif_poll(struct wr_sockaddr *);

/*
 * Function detects external source lock,
 *
 * return:
 * HEXP_EXTSRC_STATUS_LOCKED 	0
 * HEXP_LOCK_STATUS_BUSY  	1
 * HEXP_EXTSRC_STATUS_NOSRC  	2
 */

/* Timebase adjustment functions - the servo should not call the HAL directly */
int ptpd_netif_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec);
int ptpd_netif_get_dmtd_phase(int32_t *phase, struct hal_port_state *p);
void ptpd_netif_linearize_rx_timestamp(struct wr_tstamp *ts, int32_t dmtd_phase,
				       int cntr_ahead, int transition_point,
				       int clock_period);

#endif /* __LIBWR_PTPD_NETIF_H */
