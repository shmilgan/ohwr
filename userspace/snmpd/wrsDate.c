/*
 *  White Rabbit Switch date.  This is two changing values
 *
 *  Alessandro Rubini for CERN, 2014
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/auto_nlist.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wrsSnmp.h"
/* defines for nic-hardware.h */
#define WR_SWITCH
#define WR_IS_NODE 0
#define WR_IS_SWITCH 1
#include "../../kernel/wr_nic/nic-hardware.h"
#include "../../kernel/wbgen-regs/ppsg-regs.h"

static struct PPSG_WB *pps;
static uint64_t wrs_d_current_64;
static char wrs_d_current_string[32];

/* FIXME: this is copied from wr_date, should be librarized */
void *create_map(unsigned long address, unsigned long size)
{
	unsigned long ps = getpagesize();
	unsigned long offset, fragment, len;
	void *mapaddr;
	int fd;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0)
		return NULL;

	offset = address & ~(ps -1);
	fragment = address & (ps -1);
	len = address + size - offset;

	mapaddr = mmap(0, len, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fd, offset);
	close(fd);
	if (mapaddr == MAP_FAILED)
		return NULL;
	return mapaddr + fragment;
}

static void wrs_d_get(void)
{
	unsigned long utch, utcl, tmp1, tmp2;
	time_t t;
	struct tm tm;

	if (!pps) /* first time, map the fpga space */
		pps = create_map(FPGA_BASE_PPSG, sizeof(*pps));
	if (!pps) {
		wrs_d_current_64 = 0;
		strcpy(wrs_d_current_string, "0000-00-00-00:00:00 (failed)");
		return;
	}

	do {
		utch = pps->CNTR_UTCHI;
		utcl = pps->CNTR_UTCLO;
		tmp1 = pps->CNTR_UTCHI;
		tmp2 = pps->CNTR_UTCLO;
	} while((tmp1 != utch) || (tmp2 != utcl));

	wrs_d_current_64 = (uint64_t)(utch) << 32 | utcl;
	t = wrs_d_current_64;
	localtime_r(&t, &tm);
	strftime(wrs_d_current_string,
		 sizeof(wrs_d_current_string), "%Y-%m-%d-%H:%M:%S", &tm);
}

static int date_group(netsnmp_mib_handler          *handler,
			 netsnmp_handler_registration *reginfo,
			 netsnmp_agent_request_info   *reqinfo,
			 netsnmp_request_info         *requests)
{
	oid obj; /* actually, an integer, i.e. the final index */

	switch (reqinfo->mode) {
	case MODE_GET:

		wrs_d_get();

		/*
		 * WARNING: the current snmpd is bugged: it has
		 * endianness problems with 64 bit, and the two
		 * halves are swapped. So pre-swap them here
		 */
		wrs_d_current_64 =
			(wrs_d_current_64 << 32) | (wrs_d_current_64 >> 32);

		/* "- 2" because last is 0 for all scalars, I suppose */
		obj = requests->requestvb->name[
			requests->requestvb->name_length - 2
			];

		if (obj == 1) /* number */
			snmp_set_var_typed_value(requests->requestvb,
						 ASN_COUNTER64,
						 &wrs_d_current_64,
						 sizeof(wrs_d_current_64));
		else /* string */
			snmp_set_var_typed_value(requests->requestvb,
						 ASN_OCTET_STR,
						 wrs_d_current_string,
						 strlen(wrs_d_current_string));
		break;
	default:
		snmp_log(LOG_ERR, "unknown mode (%d) in wrs date group\n",
			 reqinfo->mode);
		return SNMP_ERR_GENERR;
	}
	return SNMP_ERR_NOERROR;
}


void
init_wrsDate(void)
{

	const oid wrsDate_oid[] = {  WRS_OID, 5 };
	netsnmp_handler_registration *hreg;

	/* do the registration */
	hreg = netsnmp_create_handler_registration(
		"wrsDate", date_group,
		wrsDate_oid, OID_LENGTH(wrsDate_oid),
		HANDLER_CAN_RONLY);
	netsnmp_register_scalar_group(
		hreg, 1 /* min */, 2 /* max */);
}
