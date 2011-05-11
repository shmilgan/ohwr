#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>

#include "swflood.h"

static int recv_and_send(struct swf_status *status, int sender,
			 unsigned char *packet, int plen);

/* Allocate a list of interfaces, with associated sockets */
static int iface_verify(struct swf_line *line)
{
	struct ifreq ifr;
	int i, sock, err = 0;

	/* To verify the command we must ensure interfaces exist */
	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sock < 0) {
		fprintf(stderr, "%s: socket: %s\n",
			line->argv[0], strerror(errno));
		return -errno;
	}
	for (i = 0; i < line->argc - 1; i++) {
		memset (&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, line->argv[i+1], IF_NAMESIZE);
		if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
			fprintf(stderr, "%s: no such interface \"%s\"\n",
				line->argv[0], line->argv[i]);
			err++;
		}
	}
	if (err) return -EINVAL;
	return 0;
}

static struct swf_line *iface_execute(struct swf_line *line,
				     struct swf_status *status)
{
	struct ifreq ifr;
	struct packet_mreq req;
	struct swf_if *iface;
	struct sockaddr_ll addr;
	int i, sock;

	/* We are sure interface names do exist */
	for (i = 0; i < status->n_iface; i++) {
		close(status->iface[i].socket);
	}
	if (status->iface)
		free(status->iface);
	iface = calloc(line->argc - 1, sizeof(*iface));
	if (!iface) {
		fprintf(stderr, "%s: out of memory\n", line->argv[0]);
		return line->next;
	}
	status->iface = iface;
	for (i = 0; i < line->argc - 1; i++, iface++) {
		/* one socket per interface */
		sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
		if (sock <= 0) {
			fprintf(stderr, "%s: socket(): %s\n",
				line->argv[0], strerror(errno));
			return line->next;
		}
		/* one ifindex per inteface */
		memset (&ifr, 0, sizeof(ifr));
		iface->name = line->argv[i+1];
		strncpy(ifr.ifr_name, iface->name, IF_NAMESIZE);
		if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
			fprintf(stderr, "%s: getindex(%s): %s\n",
				line->argv[0], iface->name, strerror(errno));
			return line->next;
		}
		/* one promiscuous per inteface */
		memset(&req, 0, sizeof(req));
		req.mr_ifindex = ifr.ifr_ifindex;
		req.mr_type = PACKET_MR_PROMISC;
		if (setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
			       &req, sizeof(req)) < 0) {
			fprintf(stderr, "%s: set_promiscuous(%s): %s\n",
				line->argv[0], iface->name, strerror(errno));
			return line->next;
		}
		/* one bind per inteface */
		addr.sll_family = AF_PACKET;
		addr.sll_protocol = htons(ETH_P_ALL);
		addr.sll_ifindex = ifr.ifr_ifindex;
		if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			fprintf(stderr, "%s: bind(%s): %s\n",
				line->argv[0], iface->name, strerror(errno));
			return line->next;
		}
		iface->socket = sock;
	}
	status->n_iface = i;
	return line->next;
}

static struct swf_command cmd_iface = {
	.name = "iface",
	.verify = iface_verify,
	.execute = iface_execute,
};
declare_command(cmd_iface);

/* flush is trivial */
static int flush_verify(struct swf_line *line)
{
	if (line->argc != 1) {
		fprintf(stderr, "%s: wrong number of arguments\n",
			line->argv[0]);
		return -EINVAL;
	}
	return 0;
}

static struct swf_line *flush_execute(struct swf_line *line,
				      struct swf_status *status)
{
	int i;

	usleep(10*1000);
	recv_and_send(status, 0, NULL, 0);

	fprintf(status->logfile, "flush:");
	for (i = 0; i < status->n_iface; i++) {
		fprintf(status->logfile, " %i", status->iface[i].stat);
		status->iface[i].stat = 0;
	}
	putc('\n', status->logfile);
	return line->next;
}

static struct swf_command cmd_flush = {
	.name = "flush",
	.verify = flush_verify,
	.execute = flush_execute,
};
declare_command(cmd_flush);

static int expect_verify(struct swf_line *line)
{
	int i, j, err = 0;
	char *t;
	/* actually, we can't know, now, how many interfaces we have*/
	for (i = 1; i < line->argc; i++) {
		j = strtol(line->argv[i], &t, 10);
		if (t && *t) {
			fprintf(stderr, "%s: not a number \"%s\"\n",
				line->argv[0], line->argv[i]);
			err++;
		}
	}
	if (err)
		return -EINVAL;
	return 0;
}

static struct swf_line *expect_execute(struct swf_line *line,
				      struct swf_status *status)
{
	int i, j, error = 0;

	usleep(10*1000);
	recv_and_send(status, 0, NULL, 0);

	if (line->argc != status->n_iface + 1) {
		fprintf(stderr, "%s: wrong number of arguments "
			"(expected %i numbers)\n", line->argv[0],
			status->n_iface);
		return line->next;
	}

	for (i = 0; i < status->n_iface; i++) {
		j = strtol(line->argv[i+1], NULL, 10);
		if (j != status->iface[i].stat)
			error++;
	}
	if (error) {
		/* loop again and print */
		fprintf(stderr, "%s:%i: expect failed\n",
			line->fname, line->lineno);
		fprintf(status->logfile, "%s:%i: expect failed\n",
			line->fname, line->lineno);
		fprintf(status->logfile, "expected:");
		for (i = 0; i < status->n_iface; i++)
			fprintf(status->logfile, " %5li",
				strtol(line->argv[i+1], NULL, 0));
		fprintf(status->logfile, "\nreceived:");
		for (i = 0; i < status->n_iface; i++)
			fprintf(status->logfile, " %5i",
				status->iface[i].stat);
		putc('\n', status->logfile);
	} else {
		fprintf(status->logfile, "%s:%i: success\n", line->fname,
			line->lineno);
	}
	/* zero stats in any case */
	for (i = 0; i < status->n_iface; i++) {
		status->iface[i].stat = 0;
	}
	return line->next;
}

static struct swf_command cmd_expect = {
	.name = "expect",
	.verify = expect_verify,
	.execute = expect_execute,
};
declare_command(cmd_expect);

/* Send and sendas are handled by the same code, and helper function */
static int readhex(char *s, int nbyte, unsigned char *res)
{
	int i, val;
	char *t = s;

	for (i = 0; i < nbyte; ) {
		if (!s || !*s)
			return -1;
		val = strtol(s, &t, 16);
		res[i++] = val;
		if (!t || !*t)
			break;
		if (t && *t != ':')
			return -1;
		t++;
		s = t;
	}
	if (i != nbyte)
		return -1;
	return 0;
}

static int readnr(char *name, char *s)
{
	int i;
	char *t;

	i = strtol(s, &t, 10);
	if (t && *t) {
		fprintf(stderr, "%s: not a number \"%s\"\n", name, s);
		return -1;
	}
	/* negative is considered and error, that's fine */
	return i;
}

static int send_verify(struct swf_line *line)
{
	int i = 1; /* arg being checked */
	int err = 0;
	unsigned char mac[6];

	if (line->argc < 2) goto wna;
	if (!strcmp(line->argv[0], "sendas")) {
		/* as ifnum */
		if (readnr(line->argv[0], line->argv[i]) < 0)
			err++;
		i++;
	}
	/* our ifnum */
	if (line->argc < i+1) goto wna;
	if (readnr(line->argv[0], line->argv[i]) < 0)
		err++;
	i++;
	/* nr packets */
	if (line->argc < i+1) goto wna;
	if (readnr(line->argv[0], line->argv[i]) < 0)
		err++;
	i++;
	/* sender addr */
	if (line->argc < i+1) goto wna;
	if (readhex(line->argv[i], 2, mac)) {
		fprintf(stderr, "%s: expected \"xx:xx\" not \"%s\"\n",
			line->argv[0], line->argv[i]);
		err++;
	}
	i++;
	/* incr */
	if (line->argc < i+1) goto wna;
	if (line->argv[i][0] == '+') {
		if (readnr(line->argv[0], line->argv[i]+1) < 0)
			err++;
		i++;
	}
	/* dest addr */
	if (line->argc < i+1) goto wna;
	if (readhex(line->argv[i], 6, mac)) {
		fprintf(stderr, "%s: expected MAC addr not \"%s\"\n",
			line->argv[0], line->argv[i]);
		err++;
	}
	i++;
	/* incr */
	if (line->argc == i+1 && line->argv[i][0] == '+') {
		if (readnr(line->argv[0], line->argv[i]+1) < 0)
			err++;
		i++;
	}
	if (line->argc != i) goto wna;
	if (err)
		return -EINVAL;
	return 0;
 wna:
	fprintf(stderr, "%s: too few arguments (checked %i)\n",
		line->argv[0], i);
	return -EINVAL;

}

static int recv_and_send(struct swf_status *status, int sender,
			  unsigned char *packet, int plen)
{
	int i;
	static uint32_t sequence;
	unsigned char rcvp[32];

	/* first receive whatever is pending on all interfaces */
	for (i = 0; i < status->n_iface; i++) {
		while (recv(status->iface[i].socket, rcvp, sizeof(rcvp),
			    MSG_DONTWAIT | MSG_TRUNC) != -1)
			status->iface[i].stat++;
		if (errno != EAGAIN) {
			fprintf(stderr, "recv(if#%i): %s\n", i,
				strerror(errno));
		}
	}
	/* then send, adding a 32-bit sequence number, in net order */
	if (!packet)
		return 0;
	if (plen >= 14+16) *(uint32_t *)(packet + 14+16-4) = htonl(sequence++);
	send(status->iface[sender].socket, packet, plen, 0);
	return 0;
}

static struct swf_line *send_execute(struct swf_line *line,
				     struct swf_status *status)
{
	/* we must parse arguments anyways... */
	unsigned char macsend[6] = {0x22, 0x00, 0x10, 0x00,};
	unsigned char macrecv[6];
	unsigned char packet[256] = {0,};
	int senderif, sendermac = -1;
	int incrsend = 0, incrrecv = 0;
	int npacket, j, i = 1;

	if (!strcmp(line->argv[0], "sendas")) {
		/* as ifnum */
		sendermac = readnr(line->argv[0], line->argv[i]);
		i++;
	}
	/* our ifnum */
	senderif = readnr(line->argv[0], line->argv[i]);
	if (sendermac == -1) sendermac = senderif;
	macsend[2] += sendermac;
	i++;
	/* nr packets */
	npacket = readnr(line->argv[0], line->argv[i]);
	i++;
	/* sender addr */
	readhex(line->argv[i], 2, macsend+4);
	i++;
	/* incr */
	if (line->argv[i][0] == '+') {
		incrsend = readnr(line->argv[0], line->argv[i]+1);
		i++;
	}
	/* dest addr */
	readhex(line->argv[i], 6, macrecv);
	i++;
	/* incr */
	if (line->argc == i+1 && line->argv[i][0] == '+')
		incrrecv = readnr(line->argv[0], line->argv[i]+1);

	/* ok, now send and account */
	for (i = 0; i < npacket; i++) {
		memcpy(packet+0, macrecv, 6);
		memcpy(packet+6, macsend, 6);
		packet[12] = packet[13] = 0x66; /* fake protocol */
		recv_and_send(status, senderif, packet, sizeof(packet));
		j = ((macsend[4] << 8) | macsend[5]) + incrsend;
		macsend[4] = j >> 8; macsend[5] = j;
		j = ((macrecv[4] << 8) | macrecv[5]) + incrrecv;
		macrecv[4] = j >> 8; macrecv[5] = j;
	}
	return line->next;
}

static struct swf_command cmd_send = {
	.name = "send",
	.verify = send_verify,
	.execute = send_execute,
};
declare_command(cmd_send);

static struct swf_command cmd_sendas = {
	.name = "sendas",
	.verify = send_verify,
	.execute = send_execute,
};
declare_command(cmd_sendas);

#if 0 /* not yet */
/* Expect some statistics */
static struct swf_command cmd_expect = {
	.name = "expect",
	.verify = expect_verify,
	.execute = expect_execute,
};
declare_command(cmd_expect);
#endif

