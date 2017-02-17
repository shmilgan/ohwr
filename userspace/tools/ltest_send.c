/*
 * This sends frames according to the "latency test" we do in wrpc-sw.
 * We use the ptpnetif function family (that is not otherwise used)
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libwr/ptpd_netif.h>
#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>

/* Same structure as in wrpc-sw; their wr_timestpam is our wr_tstamp */
struct latency_frame {
	uint32_t type;   /* 1, 2, 3 */
	uint32_t sequence;
	struct wr_tstamp ts[2];
};

/* Same asn in wrpc-sw, again. Avoid standard names for collisions */
struct wr_ethhdr_vlan {
	uint8_t dstmac[6];
	uint8_t srcmac[6];
	uint16_t ethtype;
	uint16_t tag;
	uint16_t ethtype_2;
};

/* But this is ours */
struct ltest_msg {
	struct wr_ethhdr_vlan hdr;
	struct latency_frame lat;
} frame;


struct wrs_shm_head *hal_head;
struct hal_shmem_header *hal_shmem;
struct hal_port_state *hal_ports;
int hal_nports_local;

int main(int argc, char **argv)
{
	struct wr_socket *wrsock;
	struct wr_sockaddr wraddr;
	struct hal_port_state *port;
	uint16_t ethtype = 291; /* default in wrpc-sw::Kconfig */
	int opt, msecs, count = 0; /* infinite */
	struct ltest_msg msg;
	char *ethname;

	while((opt = getopt(argc, argv, "e:c:")) != -1) {
		switch(opt) {
		case 'e':
			ethtype = atoi(optarg);
			break;
		case 'c':
			count = atoi(optarg);
			break;
		}
	}

	if (optind != argc - 2) {
		fprintf(stderr, "%s: Use: \"%s [-e <ethtype>] [-c <count>]"
			" <iface> <millisecs>\"\n", argv[0], argv[0]);
		exit(1);
	}
	ethname = argv[optind];
	msecs = atoi(argv[optind + 1]);

	/* hal connection and shared memory */
	if (ptpd_netif_init()) {
		fprintf(stderr, "%s: Can't connect to hal\n", argv[0]);
		exit(1);
	}

	opt = wrs_shm_get_and_check(wrs_shm_hal, &hal_head);
	if (opt) {
		fprintf(stderr, "%s: Can't connect to shm (%i)\n", argv[0],
			opt);
		exit(1);
	}
	if (hal_head->version != HAL_SHMEM_VERSION) {
		fprintf(stderr, "%s: Unknown hal's shm version %i "
			"(known is %i)\n", argv[0], hal_head->version,
			HAL_SHMEM_VERSION);
		exit(1);
	}
	hal_shmem = (void *)hal_head + hal_head->data_off;
	hal_nports_local = hal_shmem->nports;
	hal_ports = wrs_shm_follow(hal_head, hal_shmem->ports);
	port = hal_lookup_port(hal_ports, hal_nports_local, ethname);
	if (!port) {
		fprintf(stderr, "%s: Unknown port \"%s\"\n",
			argv[0], ethname);
		exit(1);
	}

	/* To open the wr socket we need to know the mac and more */
	strcpy(wraddr.if_name, ethname);
	wraddr.family = PTPD_SOCK_RAW_ETHERNET;
	wraddr.ethertype = htons(ethtype);
	/* wraddr.mac -- is it needed? */
	/* port unused */
	/* physical_port unused */

	wrsock = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET, 0,
					  &wraddr, port);
	if (!wrsock) {
		fprintf(stderr, "%s: Can't create socket\n", argv[0]);
		exit(1);
	}

	/* fill the fixed parts of the message (use vlan 0, but with prio */
	memset(&msg, 0, sizeof(msg));
	memset(msg.hdr.dstmac, 0xff, 6);
	memset(msg.hdr.srcmac, 0x02, 6);
	msg.hdr.ethtype = htons(0x8100);
	msg.hdr.ethtype_2 = ethtype;



	do {
		static int sequence;
		static int prios[] = {7, 6, 0};

		msg.lat.sequence = sequence++;
		msg.hdr.tag = prios[0] << 12;
		ptpd_netif_sendto(wrsock, &wraddr, &msg, sizeof(msg),
			msg.lat.ts + 0);

		msg.lat.sequence = sequence++;
		msg.hdr.tag = prios[1] << 12;
		ptpd_netif_sendto(wrsock, &wraddr, &msg, sizeof(msg),
			msg.lat.ts + 1);

		msg.lat.sequence = sequence++;
		msg.hdr.tag = prios[2] << 12;
		ptpd_netif_sendto(wrsock, &wraddr, &msg, sizeof(msg),
				 NULL);

		usleep(msecs * 1000); /* we could do better... */

	} while (!count || --count);

	exit(0);
}
