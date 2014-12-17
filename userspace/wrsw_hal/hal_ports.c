/* Port initialization and state machine */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/if.h>

/* LOTs of hardware includes */
#include <libwr/switch_hw.h>
#include <libwr/trace.h>
#include <libwr/pio.h>
#include <libwr/sfp_lib.h>
#include <libwr/shmem.h>
#include <libwr/config.h>

#include "wrsw_hal.h"
#include "timeout.h"
#include <rt_ipc.h>
#include <hal/hal_exports.h>
#include <libwr/hal_shmem.h>
#include "driver_stuff.h"

/* Default fiber alpha coefficient (G.652 @ 1310 nm TX / 1550 nm RX) */
#define DEFAULT_FIBER_ALPHA_COEF (1.4682e-04*1.76)

#define RTS_POLL_INTERVAL 200 /* ms */
#define SFP_POLL_INTERVAL 1000 /* ms */

static void *hal_port_shmem;

/* Port table: the only item which is not "hal_port_*", as it's much used */
static struct hal_port_state *ports;

/* An fd of always opened raw sockets for ioctl()-ing Ethernet devices */
static int hal_port_fd;

/* RT subsystem PLL state, polled regularly via mini-ipc */
static struct rts_pll_state hal_port_rts_state;
static int hal_port_rts_state_valid = 0;

/* Polling timeouts (RT Subsystem & SFP detection) */
static timeout_t hal_port_tmo_rts, hal_port_tmo_sfp;
static int hal_port_nports;

int hal_port_check_lock(const char *port_name);

int hal_port_any_locked()
{
	if (!hal_port_rts_state_valid)
		return -1;
	if (hal_port_rts_state.current_ref == REF_NONE)
		return -1;

	return hal_port_rts_state.current_ref;
}

/* Resets the state variables of a port and re-starts its state machines */
static void hal_port_reset_state(struct hal_port_state * p)
{
	p->calib.rx_calibrated = 0;
	p->calib.tx_calibrated = 0;
	p->locked = 0;
	p->state = HAL_PORT_STATE_LINK_DOWN;
	p->lock_state = 0;
	p->tx_cal_pending = 0;
	p->rx_cal_pending = 0;
}

/* checks if the port is supported by the FPGA firmware */
static int hal_port_check_presence(const char *if_name)
{
	struct ifreq ifr;

	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));

	if (ioctl(hal_port_fd, SIOCGIFHWADDR, &ifr) < 0)
		return 0;

	return 1;
}

static void hal_port_enable(int port, int enable)
{
	char str[50];
	snprintf(str, sizeof(str), "/sbin/ifconfig wr%d %s", port,
		 enable ? "up" : "down");
	system(str);
}

/* Port initialization, from dot-config values */
static int hal_port_init(int index)
{
	struct hal_port_state *p = &ports[index];
	char name[128], s[128];
	int val, error;

	/* make sure the states and other variables are in their init state */
	hal_port_reset_state(p);

	p->state = HAL_PORT_STATE_DISABLED;
	p->in_use = 1;

	/* read dot-config values for this index, starting from name */
	error = libwr_cfg_convert2("PORT%02i_PARAMS", "name", LIBWR_STRING,
				   name, index);
	if (error)
		return -1;
	strncpy(p->name, name, 16);

	/* check if the port is built into the firmware, if not, we are done */
	if (!hal_port_check_presence(name))
		return -1;

	val = 18 * 800; /* magic default from previous code */
	error = libwr_cfg_convert2("PORT%02i_PARAMS", "tx", LIBWR_INT,
				   &val, index);
	if (error)
		fprintf(stderr, "port index %i (%s): no \"tx=\" specified\n",
			index, name);
	p->calib.phy_tx_min = val;

	val = 18 * 800; /* magic default from previous code */
	error = libwr_cfg_convert2("PORT%02i_PARAMS", "rx", LIBWR_INT,
				   &val, index);
	if (error)
		fprintf(stderr, "port index %i (%s): no \"rx=\" specified\n",
			index, name);
	p->calib.phy_rx_min = val;

	p->calib.delta_tx_board = 0; /* never set */
	p->calib.delta_rx_board = 0; /* never set */
	sscanf(p->name + 2, "%d", &p->hw_index);

	hal_port_enable(p->hw_index, 1);

	{
		static struct roletab { char *name; int value; } *rp, rt[] = {
			{"auto",   HEXP_PORT_MODE_WR_M_AND_S},
			{"master", HEXP_PORT_MODE_WR_MASTER},
			{"slave",  HEXP_PORT_MODE_WR_SLAVE},
			{"non-wr", HEXP_PORT_MODE_NON_WR},
			{NULL,     HEXP_PORT_MODE_NON_WR /* default */},
		};

		strcpy(s, "non-wr"); /* default if no string passed */
		p->mode = HEXP_PORT_MODE_NON_WR;
		error = libwr_cfg_convert2("PORT%02i_PARAMS", "role",
					   LIBWR_STRING, s, index);
		if (error)
			fprintf(stderr, "port index %i (%s): "
				"no \"role=\" specified\n", index, name);

		for (rp = rt; rp->name; rp++)
			if (!strcasecmp(s, rp->name))
				break;
		p->mode = rp->value;
		if (!rp->name)
			fprintf(stderr, "port index %i (%s): invalid role "
				"\"%s\" specified\n", index, name, s);

		TRACE(TRACE_INFO, "Port %s: mode %i", p->name, val);
	}
	/* Used to pre-calibrate the TX path for each port. No more in V3 */

	/* FIXME: this address should come from the driver header */
	p->ep_base = 0x30000 + 0x400 * p->hw_index;

	return 0;
}

/* Interates via all the ports defined in the config file and
 * intializes them one after another. */
int hal_port_init_all()
{
	int index;
	struct hal_shmem_header *hal_hdr;
	struct wrs_shm_head *head;

	TRACE(TRACE_INFO, "Initializing switch ports...");

	/* default timeouts */
	tmo_init(&hal_port_tmo_sfp, SFP_POLL_INTERVAL, 1);
	tmo_init(&hal_port_tmo_rts, RTS_POLL_INTERVAL, 1);

	/* Open a single raw socket for accessing the MAC addresses, etc. */
	hal_port_fd = socket(AF_PACKET, SOCK_DGRAM, 0);
	if (hal_port_fd < 0) {
		fprintf(stderr, "%s: Can't create socket: %s\n",
			__func__, strerror(errno));
		return -1;
	}
	/* Allocate the ports in shared memory, so wr_mon etc can see them */
	hal_port_shmem = wrs_shm_get(wrs_shm_hal, "wrsw_hal", WRS_SHM_WRITE);
	if (!hal_port_shmem) {
		fprintf(stderr, "%s: Can't join shmem: %s\n", __func__,
			strerror(errno));
		return -1;
	}
	head = hal_port_shmem;
	hal_hdr = wrs_shm_alloc(hal_port_shmem, sizeof(*hal_hdr));
	ports = wrs_shm_alloc(hal_port_shmem,
			      sizeof(struct hal_port_state)
			      * HAL_MAX_PORTS);
	if (!hal_hdr ||  !ports) {
		fprintf(stderr, "%s: can't allocate in shmem\n", __func__);
		return -1;
	}

	hal_hdr->ports = ports;

	for (index = 0; index < HAL_MAX_PORTS; index++)
		if (hal_port_init(index) < 0)
			break;
	hal_port_nports = index;

	TRACE(TRACE_INFO, "Number of physical ports supported in HW: %d",
	      hal_port_nports);

	/* We are done, mark things as valid */
	hal_hdr->nports = hal_port_nports;
	head->version = HAL_SHMEM_VERSION;

	/* Create a WRIPC server for HAL public API */
	return hal_init_wripc(ports);

	return 0;
}

/* Checks if the link is up on inteface (if_name). Returns non-zero if yes. */
static int hal_port_check_link(const char *if_name)
{
	struct ifreq ifr;

	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));

	if (ioctl(hal_port_fd, SIOCGIFFLAGS, &ifr) > 0)
		return -1;

	return (ifr.ifr_flags & IFF_UP && ifr.ifr_flags & IFF_RUNNING);
}

/* Port locking state machine - controls the HPLL/DMPLL.  TODO (v3):
   get rid of this code - this will all be moved to the realtime CPU
   inside the FPGA and the softpll. */
static void hal_port_locking_fsm(struct hal_port_state * p)
{
}

int hal_port_pshifter_busy()
{
	struct rts_pll_state *hs = &hal_port_rts_state;

	if (!hal_port_rts_state_valid)
		return 1;

	if (hs->current_ref != REF_NONE) {
		int busy =
		    hs->channels[hs->current_ref].
		    flags & CHAN_SHIFTING ? 1 : 0;

		if (0)
			TRACE(TRACE_INFO, "PSBusy %d, flags %x", busy,
			      hs->channels[hs->current_ref].flags);
		return busy;
	}

	return 1;
}

/* Updates the current value of the phase shift on a given
 * port. Called by the main update function regularly. */
static void poll_rts_state()
{
	struct rts_pll_state *hs = &hal_port_rts_state;

	if (tmo_expired(&hal_port_tmo_rts)) {
		hal_port_rts_state_valid = rts_get_state(hs) < 0 ? 0 : 1;
		if (!hal_port_rts_state_valid)
			printf("rts_get_state failure, weird...\n");
	}
}

static uint32_t pcs_readl(struct hal_port_state * p, int reg)
{
	struct ifreq ifr;
	uint32_t rv;

	strncpy(ifr.ifr_name, p->name, sizeof(ifr.ifr_name));

	rv = NIC_READ_PHY_CMD(reg);
	ifr.ifr_data = (void *)&rv;
//      printf("raw fd %d name %s\n", hal_port_fd, ifr.ifr_name);
	if (ioctl(hal_port_fd, PRIV_IOCPHYREG, &ifr) < 0) {
		fprintf(stderr, "ioctl failed\n");
	};

//      printf("PCS_readl: reg %d data %x\n", reg, NIC_RESULT_DATA(rv));
	return NIC_RESULT_DATA(rv);
}

static int hal_port_link_down(struct hal_port_state * p, int link_up)
{
	/* If, at any moment, the link goes down, reset the FSM and
	 * the port state structure. */
	if (!link_up && p->state != HAL_PORT_STATE_LINK_DOWN
	    && p->state != HAL_PORT_STATE_DISABLED) {
		if (p->locked) {
			TRACE(TRACE_INFO,
			      "switching RTS to use local reference");
			if (hal_get_timing_mode()
			    != HAL_TIMING_MODE_GRAND_MASTER)
				rts_set_mode(RTS_MODE_GM_FREERUNNING);
		}

		shw_sfp_set_led_link(p->hw_index, 0);
		p->state = HAL_PORT_STATE_LINK_DOWN;
		hal_port_reset_state(p);

		rts_enable_ptracker(p->hw_index, 0);
		TRACE(TRACE_INFO, "%s: link down", p->name);

		return 1;
	}
	return 0;
}

/* Main port state machine */
static void hal_port_fsm(struct hal_port_state * p)
{
	struct rts_pll_state *hs = &hal_port_rts_state;
	int link_up = hal_port_check_link(p->name);

	if (hal_port_link_down(p, link_up))
		return;
	/* handle the locking part */
	hal_port_locking_fsm(p);

	switch (p->state) {

	case HAL_PORT_STATE_DISABLED:
		p->calib.tx_calibrated = 0;
		p->calib.rx_calibrated = 0;
		break;

		/* Default state - wait until the link goes up */
	case HAL_PORT_STATE_LINK_DOWN:
		{
			if (link_up) {
				p->calib.tx_calibrated = 1;
				p->calib.rx_calibrated = 1;
				/* FIXME: use proper register names */
				TRACE(TRACE_INFO, "Bitslide: %d",
				      ((pcs_readl(p, 16) >> 4) & 0x1f));
				p->calib.delta_rx_phy =
				    p->calib.phy_rx_min +
				    ((pcs_readl(p, 16) >> 4) & 0x1f) * 800;
				p->calib.delta_tx_phy = p->calib.phy_tx_min;

				if (0)
					TRACE(TRACE_INFO,
					      "Bypassing calibration for "
					      "downlink port %s [dTx %d, dRx %d]",
					      p->name, p->calib.delta_tx_phy,
					      p->calib.delta_rx_phy);

				p->tx_cal_pending = 0;
				p->rx_cal_pending = 0;

				shw_sfp_set_led_link(p->hw_index, 1);
				TRACE(TRACE_INFO, "%s: link up", p->name);
				p->state = HAL_PORT_STATE_UP;
			}
			break;
		}

		/* Default "on" state - just keep polling the phase value. */
	case HAL_PORT_STATE_UP:
		if (hal_port_rts_state_valid) {
			p->phase_val =
			    hs->channels[p->hw_index].phase_loopback;
			p->phase_val_valid =
			    hs->channels[p->hw_index].
			    flags & CHAN_PMEAS_READY ? 1 : 0;
			//hal_port_check_lock(p->name);
			//p->locked =
		}

		break;

		/* Locking state (entered on calling hal_port_start_lock()). */
	case HAL_PORT_STATE_LOCKING:

		/* Once the locking FSM is done, go back to the "UP" state. */

		p->locked = hal_port_check_lock(p->name);

		if (p->locked) {
			TRACE(TRACE_INFO, "[main-fsm] Port %s locked.",
			      p->name);
			p->state = HAL_PORT_STATE_UP;
		}

		break;

		/* Calibration state (entered by starting the
		 * calibration with halexp_calibration_cmd()) */
	case HAL_PORT_STATE_CALIBRATION:

		/* Calibration still pending - if not anymore, go back
		 * to the "UP" state */
		if (p->rx_cal_pending || p->tx_cal_pending) {
		}		//calibration_fsm(p);
		else
			p->state = HAL_PORT_STATE_UP;

		break;
	}
}

static void hal_port_insert_sfp(struct hal_port_state * p)
{
	struct shw_sfp_header shdr;
	struct shw_sfp_caldata *cdata;

	if (shw_sfp_read_verify_header(p->hw_index, &shdr) < 0) {
		TRACE(TRACE_ERROR, "Failed to read SFP configuration header");
		return;
	}

	TRACE(TRACE_INFO,
	      "SFP Info: Manufacturer: %.16s P/N: %.16s, S/N: %.16s",
	      shdr.vendor_name, shdr.vendor_pn, shdr.vendor_serial);
	cdata = shw_sfp_get_cal_data(p->hw_index);
	if (cdata) {
		TRACE(TRACE_INFO, "SFP Info: (%s) deltaTx %d "
		      "delta Rx %d alpha %.3f (* 1e6)",
		      cdata->flags & SFP_FLAG_CLASS_DATA
		      ? "class-specific" : "device-specific",
		      cdata->delta_tx, cdata->delta_rx,
		      cdata->alpha * 1e6);

		memcpy(&p->calib.sfp, cdata,
		       sizeof(struct shw_sfp_caldata));
	} else {
		TRACE(TRACE_ERROR, "WARNING! SFP on port %s is "
		      "NOT registered in the DB (using default "
		      "delta & alpha values). This may cause "
		      "severe timing performance degradation!",
		      p->name);
		p->calib.sfp.delta_tx = 0;
		p->calib.sfp.delta_rx = 0;
		p->calib.sfp.alpha = DEFAULT_FIBER_ALPHA_COEF;
	}

	p->state = HAL_PORT_STATE_LINK_DOWN;
	shw_sfp_set_tx_disable(p->hw_index, 0);
}

static void hal_port_remove_sfp(struct hal_port_state * p)
{
	hal_port_link_down(p, 0);
	p->state = HAL_PORT_STATE_DISABLED;
}

/* detects insertion/removal of SFP transceivers */
static void hal_port_poll_sfp()
{
	if (tmo_expired(&hal_port_tmo_sfp)) {
		uint32_t mask = shw_sfp_module_scan();
		static int old_mask = 0;

		if (mask != old_mask) {
			int i, hw_index;
			for (i = 0; i < HAL_MAX_PORTS; i++) {
				hw_index = ports[i].hw_index;

				if (ports[i].in_use
				    && (mask ^ old_mask) & (1 << hw_index)) {
					int insert = mask & (1 << hw_index);
					TRACE(TRACE_INFO, "Detected SFP %s "
					      "on port %s.",
					      insert ? "insertion" : "removal",
					      ports[i].name);
					if (insert)
						hal_port_insert_sfp(&ports[i]);
					else
						hal_port_remove_sfp(&ports[i]);
				}
			}
		}
		old_mask = mask;
	}
}

/* Executes the port FSM for all ports. Called regularly by the main loop. */
void hal_port_update_all()
{
	int i;

	poll_rts_state();
	hal_port_poll_sfp();

	for (i = 0; i < HAL_MAX_PORTS; i++)
		if (ports[i].in_use)
			hal_port_fsm(&ports[i]);
}

int hal_port_enable_tracking(const char *port_name)
{
	const struct hal_port_state *p = hal_port_lookup(ports, port_name);

	if (!p)
		return -1;

	return rts_enable_ptracker(p->hw_index, 1); /* 0 or -1 already */
}

/* Triggers the locking state machine, called by the PTPd during the
 * WR link setup phase. */
int hal_port_start_lock(const char *port_name, int priority)
{
	struct hal_port_state *p = hal_port_lookup(ports, port_name);

	if (!p)
		return -1;

	/* can't lock to a disconnected port */
	if (p->state != HAL_PORT_STATE_UP)
		return -1;

	/* fixme: check the main FSM state before */
	p->state = HAL_PORT_STATE_LOCKING;

	TRACE(TRACE_INFO, "Locking to port: %s", port_name);

	rts_set_mode(RTS_MODE_BC);

	return rts_lock_channel(p->hw_index, 0); /* 0 or -1 already */
}

/* Returns 1 if the port is locked */
int hal_port_check_lock(const char *port_name)
{
	const struct hal_port_state *p = hal_port_lookup(ports, port_name);
	struct rts_pll_state *hs = &hal_port_rts_state;

	if (!p)
		return 0; /* was -1, but it would confuse the caller */

	if (!hal_port_rts_state_valid)
		return 0;

	if (hs->delock_count > 0)
		return 0;

	return (hs->current_ref == p->hw_index &&
		(hs->flags & RTS_DMTD_LOCKED) &&
		(hs->flags & RTS_REF_LOCKED));
}

/* Public function for querying the state of a particular port (DMTD
 * phase, calibration deltas, etc.) */
