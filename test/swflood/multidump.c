#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/select.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <net/if_arp.h>

#include <netpacket/packet.h>

static int dumpone(char *name, struct timeval *tv,
		   unsigned char *packet, int plen)
{
	int i;
        static char sep[]="::::: ::::: . "   "... ... ... ...\n";
	char *s = sep;

	printf("%-8s %li.%06li   ", name, tv->tv_sec, tv->tv_usec);
	for (i = 0; i < plen && *s; i++)
		printf("%02x%c", packet[i], *(s++));
	return 0;
}

int main(int argc, char **argv)
{
	int i, len;
	int sock = -1, *socks;
	fd_set fullset, set;
	struct ifreq    ifr;
	struct packet_mreq req;
	struct sockaddr_ll addr;
	unsigned char packet[128];

	if (argc == 1) {
		fprintf(stderr, "%s: pass a list of eth addresses\n",
			argv[0]);
		exit(1);
	}

	socks = malloc(sizeof(socks[0]) * (argc-1));
	FD_ZERO(&fullset);
	for (i = 0; i < argc-1; i++) {
		/* one socket per interface */
		sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
		if (sock < 0) {
			fprintf(stderr, "%s: socket(): %s\n",
				argv[0], strerror(errno));
			exit(1);
		}
		/* one ifindex per inteface */
		memset (&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, argv[i+1], IF_NAMESIZE);
		if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
			fprintf(stderr, "%s: getindex(%s): %s\n",
				argv[0], argv[i+1], strerror(errno));
			exit(1);
		}
		/* one promiscuous per inteface */
		memset(&req, 0, sizeof(req));
		req.mr_ifindex = ifr.ifr_ifindex;
		req.mr_type = PACKET_MR_PROMISC;
		if (setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
			       &req, sizeof(req)) < 0) {
			fprintf(stderr, "%s: set_promiscuous(%s): %s\n",
				argv[0], argv[i+1], strerror(errno));
			exit(1);
		}
		/* one bind per inteface */
		addr.sll_family = AF_PACKET;
		addr.sll_protocol = htons(ETH_P_ALL);
		addr.sll_ifindex = ifr.ifr_ifindex;
		if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			fprintf(stderr, "%s: bind(%s): %s\n",
				argv[0], argv[i+1], strerror(errno));
			exit(1);
		}
		/* save for later */
		socks[i] = sock;
		FD_SET(sock, &fullset);
	}
	/* end of loop: "sock" is the largest one */

	while(1) {
		struct timeval tv;

		set = fullset;
		i = select(sock+1, &set, NULL, NULL, NULL);
		gettimeofday(&tv, NULL);
		if (i < 0 && errno == EINTR)
			continue;
		if (i <= 0)
			exit(1);
		/* retrieve data */
		for (i = 0; i < argc-1; i++) {
			if (!FD_ISSET(socks[i], &set))
				continue;
			while ( (len = recv(socks[i], packet, sizeof(packet),
					    MSG_DONTWAIT | MSG_TRUNC)) >= 0)
				dumpone(argv[i+1], &tv, packet, len);
			if (errno != EAGAIN) {
				fprintf(stderr, "%s: revb(%s): %sn",
				argv[0], argv[i+1], strerror(errno));
			}
		}
		putchar('\n');
	}
}
