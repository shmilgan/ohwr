/* Port initialization and state machine */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/if.h>

/* LOTs of hardware includes */
#include <switch_hw.h>
#include <trace.h>
#include <pio.h>
#include <sfp_lib.h>

#include <fpga_io.h>
#include <regs/endpoint-regs.h>

#include "wrsw_hal.h"
#include "timeout.h"
#include "rt_ipc.h"
#include "hal_exports.h"
#include "driver_stuff.h"


/* Port modes - WR Uplink/Downlink/Non-WR-at-all */
#define HAL_PORT_MODE_WR_UPLINK 1
#define HAL_PORT_MODE_WR_DOWNLINK 2
#define HAL_PORT_MODE_NON_WR 3

/* Port state machine states */
#define HAL_PORT_STATE_DISABLED 0
#define HAL_PORT_STATE_LINK_DOWN 1
#define HAL_PORT_STATE_UP 2
#define HAL_PORT_STATE_CALIBRATION 3
#define HAL_PORT_STATE_LOCKING 4

/* Locking state machine states */
#define LOCK_STATE_NONE 0
#define LOCK_STATE_WAIT_HPLL 1
#define LOCK_STATE_WAIT_DMPLL 2
#define LOCK_STATE_LOCKED 3
#define LOCK_STATE_START 4

/* Default fiber alpha coefficient (G.652 @ 1310 nm TX / 1550 nm RX) */
#define DEFAULT_FIBER_ALPHA_COEF (1.4682e-04*1.76)

#define RTS_POLL_INTERVAL 200 /* ms */
#define SFP_POLL_INTERVAL 1000 /* ms */

/* Internal port state structure */
typedef struct {
/* non-zero: allocated */
	int in_use;
/* linux i/f name */
	char name[16];

/* MAC addr */
	uint8_t hw_addr[6];

/* ioctl() hw index */
	int hw_index;

/* file descriptor for ioctls() */
	int fd;
	int hw_addr_auto;

/* port timing mode (HAL_PORT_MODE_xxxx) */
	int mode;

/* port FSM state (HAL_PORT_STATE_xxxx) */
	int state;

/* unused */
	int index;

/* 1: PLL is locked to this port */
	int locked;

/* calibration data */
	hal_port_calibration_t calib;

/* current DMTD loopback phase (picoseconds) and whether is it valid or not */
	uint32_t phase_val;
	int phase_val_valid;
	int tx_cal_pending, rx_cal_pending;
/* locking FSM state */
	int lock_state;


/* Endpoint's base address */
	uint32_t ep_base;
} hal_port_state_t;


/* Port table */
static hal_port_state_t ports[HAL_MAX_PORTS];

/* An fd of always opened raw sockets for ioctl()-ing Ethernet devices */
static int fd_raw;


/* RT subsystem PLL state, polled regularly via mini-ipc */
static struct rts_pll_state rts_state;
static int rts_state_valid = 0;

/* Polling timeouts (RT Subsystem & SFP detection) */
static timeout_t tmo_rts, tmo_sfp;
static int num_physical_ports;

int hal_port_check_lock(const char  *port_name);

int any_port_locked()
{
    if(!rts_state_valid) return -1;
    if(rts_state.current_ref == REF_NONE) return -1;
    return rts_state.current_ref;
}

/*
 * return id of the backup channel, if available
 */
int backup_port()
{
    if(!rts_state_valid) return -1;
    if(rts_state.backup_ref == REF_NONE) return -1;
    TRACE(TRACE_INFO, "backup port: %d",rts_state.backup_ref);
    return rts_state.backup_ref;
}

int active_port()
{
	if(!rts_state_valid) return -1;
	if(rts_state.current_ref == REF_NONE) return -1;	
	return rts_state.current_ref;
}
int old_backup_port()
{
	if(!rts_state_valid) return -1;
	if(rts_state.old_ref == REF_NONE) return -1;	
	return rts_state.old_ref;
}

/* Resets the state variables of a particular port and re-starts its state machines */
static void reset_port_state(hal_port_state_t *p)
{
	p->calib.rx_calibrated = 0;
	p->calib.tx_calibrated = 0;
	p->locked = 0;
	p->state = HAL_PORT_STATE_LINK_DOWN;
	p->lock_state = LOCK_STATE_NONE;
	p->tx_cal_pending = 0;
	p->rx_cal_pending = 0;
}

#define AT_INT32 0
#define AT_DOUBLE 1

/* helper function for retreiving port parameters from the config files with type checking and defaulting
   to a given value when the parameter is not found */
static void cfg_get_port_param(const char *port_name, const char *param_name, void *rval, int param_type, ...)
{
	va_list ap;

	char str[1024];
	snprintf(str, sizeof(str), "ports.%s.%s", port_name, param_name);

	va_start(ap, param_type);

	switch(param_type)
	{
	case AT_INT32:
		if(hal_config_get_int(str, (int *) rval))
			*(int *) rval = va_arg(ap, int);
		break;

	case AT_DOUBLE:
		if(hal_config_get_double(str, (double *) rval))
			*(double *) rval = va_arg(ap, double);
		break;

	default:
		break;

	}
	va_end(ap);
}


/* checks if the port is supported by the FPGA firmware */
static int check_port_presence(const char *if_name)
{
	struct ifreq ifr;

	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));

	if(ioctl(fd_raw, SIOCGIFHWADDR, &ifr) < 0)
		return 0;

	return 1;
}

static void enable_port(int port, int enable)
{
    char str[50];
    snprintf(str, sizeof(str), "/sbin/ifconfig wr%d %s", port, enable ? "up" : "down");
    system(str);
}


/* Port initialization. Assigns the MAC address, WR timing mode, reads parameters from the config file. */
int hal_init_port(const char *name, int index)
{
	char key_name[128];
	char val[128];
	hal_port_state_t *p = &ports[index];

/* check if the port is compiled into the firmware, if not, just ignore it. */
	if(!check_port_presence(name))
	{
		reset_port_state(p);
		p->in_use = 0;
		return 0;
	}

/* make sure the states and other port variables are in their initial state */
	reset_port_state(p);

	p->state = HAL_PORT_STATE_DISABLED;
	p->in_use = 1;

	strncpy(p->name, name, 16);

/* read calibraton parameters (unwrapping and constant deltas) */
	cfg_get_port_param(name, "phy_rx_min",   &p->calib.phy_rx_min,   AT_INT32, 18*800);
	cfg_get_port_param(name, "phy_tx_min",   &p->calib.phy_tx_min,    AT_INT32, 18*800);

	cfg_get_port_param(name, "delta_tx_board",  &p->calib.delta_tx_board,  AT_INT32, 0);
	cfg_get_port_param(name, "delta_rx_board",  &p->calib.delta_rx_board,  AT_INT32, 0);


	sscanf(p->name+2, "%d", &p->hw_index);

	enable_port(p->hw_index, 1);

/* Set up the endpoint's base address (fixme: do this with the driver) */

/* FIXME: this address should come from the driver header */
	p->ep_base = 0x30000 + 0x400  *  p->hw_index;

/* Configure the port's timing role depending on the contents of the config file */
	snprintf(key_name, sizeof(key_name),  "ports.%s.mode", p->name);

	if(!hal_config_get_string(key_name, val, sizeof(val)))
	{
		if(!strcasecmp(val, "wr_m_and_s"))
			p->mode = HEXP_PORT_MODE_WR_M_AND_S;
		else if(!strcasecmp(val, "wr_master"))
			p->mode = HEXP_PORT_MODE_WR_MASTER;
		else if(!strcasecmp(val, "wr_slave"))
			p->mode = HEXP_PORT_MODE_WR_SLAVE;
		else if(!strcasecmp(val, "non_wr"))
			p->mode = HEXP_PORT_MODE_NON_WR;
		else {
			TRACE(TRACE_ERROR,"Invalid mode specified for port %s. Defaulting to Non-WR", p->name);
			p->mode = HEXP_PORT_MODE_NON_WR;
		}

		TRACE(TRACE_INFO,"Port %s: mode %s", p->name, val);
/* nothing in the config file? Disable WR mode */
	} else {
		p->mode = HEXP_PORT_MODE_NON_WR;
	}

/* Used to pre-calibrate the TX path for each port. No more in V3 */

	return 0;
}

/* Interates via all the ports defined in the config file and intializes them one after another. */
int hal_init_ports()
{
	int index = 0, i;
	char port_name[128];


	TRACE(TRACE_INFO, "Initializing switch ports...");

/* default timeouts */
	tmo_init(&tmo_sfp, SFP_POLL_INTERVAL, 1);
	tmo_init(&tmo_rts, RTS_POLL_INTERVAL, 1);

/* Open a single raw socket for accessing the MAC addresses, etc. */
	fd_raw = socket(AF_PACKET, SOCK_DGRAM, 0);
	if(fd_raw < 0) return -1;

/* Count the number of physical WR network interfaces */
	num_physical_ports = 0;
	for(i=0;i<HAL_MAX_PORTS;i++)
	{
		char if_name[16];
		snprintf(if_name, sizeof(if_name), "wr%d", i);
		if(check_port_presence(if_name)) num_physical_ports++;
	}

	TRACE(TRACE_INFO, "Number of physical ports supported in HW: %d", num_physical_ports);

	memset(ports, 0, sizeof(ports));

	for(;;)
	{
		if(!hal_config_iterate("ports", index++, port_name, sizeof(port_name))) break;
		hal_init_port(port_name, index);
	}

	return 0;
}

/* Checks if the link is up on inteface (if_name). Returns non-zero if yes. */
static int check_link_up(const char *if_name)
{
	struct ifreq ifr;

	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));

	if(ioctl(fd_raw, SIOCGIFFLAGS, &ifr) > 0) return -1;

	return (ifr.ifr_flags & IFF_UP && ifr.ifr_flags & IFF_RUNNING);
}


/* Port locking state machine - controls the HPLL/DMPLL.
   TODO (v3): get rid of this code - this will all be moved to the realtime CPU inside the FPGA
   and the softpll. */
static void port_locking_fsm(hal_port_state_t *p)
{
}

int hal_phase_shifter_busy()
{
    if(!rts_state_valid)
        return 1;

    if(rts_state.current_ref != REF_NONE)
    {
        int busy = rts_state.channels[rts_state.current_ref].flags & CHAN_SHIFTING ? 1 : 0;

//        TRACE(TRACE_INFO, "PSBusy %d, flags %x", busy, rts_state.channels[rts_state.current_ref].flags);
        return busy;
    }


    return 1;
}

/* Updates the current value of the phase shift on a given port. Called by the main update function regularly. */
static void poll_rts_state()
{
    if(tmo_expired(&tmo_rts))
    {
        rts_state_valid = rts_get_state(&rts_state) < 0 ? 0 : 1;
        if(!rts_state_valid)
            printf("rts_get_state failure, weird...\n");
    }
}

uint32_t pcs_readl(hal_port_state_t *p, int reg)
{
	struct ifreq ifr;
	uint32_t rv;

	strncpy(ifr.ifr_name, p->name, sizeof(ifr.ifr_name));

	rv = NIC_READ_PHY_CMD(reg);
	ifr.ifr_data = (void *)&rv;
//	printf("raw fd %d name %s\n", fd_raw, ifr.ifr_name);
	if(	ioctl(fd_raw, PRIV_IOCPHYREG, &ifr) < 0)
	{
		fprintf(stderr,"ioctl failed\n");
	};

//	printf("PCS_readl: reg %d data %x\n", reg, NIC_RESULT_DATA(rv));
	return NIC_RESULT_DATA(rv);
}

/* this function returns status of ports (up/down) provided by softPLL
 * This is to synchronize info about link-down betwen the information provided by 
 * check_link_up() and view of ports provided by rts_state structure. The later is 
 * updated periodically. It might happen  that check_link_up() provides info about link
 * down while the rts_state structure is not yet updated and provides state of the softPLL
 * from when the link was still up... it's a hack
 */
int link_status(int channel)
{
	TRACE(TRACE_INFO, "HW link down from SoftPLL: port_%d = %d [0x%x]\n",
	channel,0x1 & (rts_state.port_status >> channel),  rts_state.port_status);
	if(!rts_state_valid) return -1;
	return (0x1 & (rts_state.port_status >> channel));
}

static int handle_link_down(hal_port_state_t *p, int link_up)
{
/* If, at any moment, the link goes down, reset the FSM and the port state structure unless
   - it is a backup port, in this case don't touch the sPLL
   - it has a backup port, in this case switch the timing to backup  */
	if(!link_up && p->state != HAL_PORT_STATE_LINK_DOWN && p->state != HAL_PORT_STATE_DISABLED &&
	link_status(p->hw_index) == 0) //received updated from softPLL with the info that link is down
	{
		if(p->locked)
		{
			
			TRACE(TRACE_INFO, "LINK DOWN [%d]: active: %d | backup %d | old %d ",
			p->hw_index, active_port(),backup_port(),old_backup_port());
			if(hal_get_timing_mode() != HAL_TIMING_MODE_GRAND_MASTER)
				
				if(old_backup_port() == p->hw_index) // we just switche over from this port
				{
					rts_backup_channel(p->hw_index, RTS_BACKUP_CH_ACTIVATE);
					rts_state.old_ref = REF_NONE; //nasty: to make hal in sync with RT
					TRACE(TRACE_INFO, "switching to backup reference");
				}
				else if(backup_port() == p->hw_index) // it is backup port
				{
					rts_backup_channel(p->hw_index, RTS_BACKUP_CH_DOWN);
					rts_state.backup_ref = REF_NONE; //nasty: to make hal in sync with RT
					TRACE(TRACE_INFO, "switching off backup reference");
				}
				// this is active slave, not a backup & no backup for it
				else if(active_port() == p->hw_index && backup_port() < 0) 
				{
					rts_set_mode(RTS_MODE_GM_FREERUNNING);
					rts_state.current_ref = REF_NONE; //nasty: to make hal in sync with RT
					rts_state.backup_ref = REF_NONE; 
					rts_state.old_ref = REF_NONE; 
					TRACE(TRACE_INFO, "switching RTS to use local reference");
				}
				else 
				{
					TRACE(TRACE_INFO, "master port went down");
				}
			else
				  TRACE(TRACE_INFO, "I'm grandmaster, now switching of reference");
		}

		shw_sfp_set_led_link(p->hw_index, 0);
		p->state = HAL_PORT_STATE_LINK_DOWN;
		reset_port_state(p);

		rts_enable_ptracker(p->hw_index, 0); // switch over ?
		TRACE(TRACE_INFO, "%s: link down", p->name);

		return 1;
	}
	return 0;
}

/* Main port state machine */
static void port_fsm(hal_port_state_t *p)
{
	int link_up = check_link_up(p->name);

	if(handle_link_down(p, link_up))
		return;
/* handle the locking part */
	port_locking_fsm(p);

	switch(p->state)
	{

	case HAL_PORT_STATE_DISABLED:
		p->calib.tx_calibrated = 0;
		p->calib.rx_calibrated = 0;
		break;

/* Default state - wait until the link goes up */
	case HAL_PORT_STATE_LINK_DOWN:
	{
		if(link_up)
		{
		p->calib.tx_calibrated = 1;
		p->calib.rx_calibrated = 1;
		/* FIXME: use proper register names */
		TRACE(TRACE_INFO, "Bitslide: %d", ((pcs_readl(p, 16) >> 4) & 0x1f));
		p->calib.delta_rx_phy = p->calib.phy_rx_min + ((pcs_readl(p, 16) >> 4) & 0x1f) * 800;
		p->calib.delta_tx_phy = p->calib.phy_tx_min;

//		TRACE(TRACE_INFO,"Bypassing calibration for downlink port %s [dTx %d, dRx %d]", p->name, p->calib.delta_tx_phy, 	p->calib.delta_rx_phy);

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
		if(rts_state_valid)
		{
		   	p->phase_val = rts_state.channels[p->hw_index].phase_loopback;
        p->phase_val_valid = rts_state.channels[p->hw_index].flags & CHAN_PMEAS_READY ? 1 : 0;
				//hal_port_check_lock(p->name);
				//p->locked =
/*		TRACE(TRACE_ERROR,"[main-fsm] Port %s| state up, phase % d, valid %d", p->name,
		p->phase_val,p->phase_val_valid);
		TRACE(TRACE_INFO,"[main-fsm] Port %s| state up, phase % d, valid %d", p->name,
		p->phase_val,p->phase_val_valid);	*/	
		}

		break;

/* Locking state (entered upon calling hal_port_start_lock()). */
	case HAL_PORT_STATE_LOCKING:

/* Once the locking FSM is done, go back to the "UP" state. */

		p->locked = hal_port_check_lock(p->name);

		if(p->locked)
		{
			TRACE(TRACE_INFO,"[main-fsm] Port %s locked.", p->name);
			p->state = HAL_PORT_STATE_UP;
		}

		break;

/* Calibration state (entered by starting the calibration with halexp_calibration_cmd()) */
	case HAL_PORT_STATE_CALIBRATION:

/* Calibration still pending - if not anymore, go back to the "UP" state */
		if(p->rx_cal_pending || p->tx_cal_pending)
			{}//calibration_fsm(p);
		else
			p->state = HAL_PORT_STATE_UP;


		break;
	}
}

static void on_insert_sfp(hal_port_state_t *p)
{
		struct shw_sfp_header shdr;
		if(shw_sfp_read_verify_header(p->hw_index, &shdr) < 0)
			TRACE(TRACE_ERROR, "Failed to read SFP configuration header");
		else {
			struct shw_sfp_caldata *cdata;
			TRACE(TRACE_INFO, "SFP Info: Manufacturer: %.16s P/N: %.16s, S/N: %.16s", shdr.vendor_name, shdr.vendor_pn, shdr.vendor_serial);
			cdata = shw_sfp_get_cal_data(p->hw_index);
			if(cdata)
			{
				TRACE(TRACE_INFO, "SFP Info: (%s) deltaTx %d delta Rx %d alpha %.3f (* 1e6)",
					cdata->flags & SFP_FLAG_CLASS_DATA ? "class-specific" : "device-specific",
					cdata->delta_tx, cdata->delta_rx, cdata->alpha * 1e6);

				memcpy(&p->calib.sfp, cdata, sizeof(struct shw_sfp_caldata));
			} else {
				TRACE(TRACE_ERROR, "WARNING! SFP on port %s is NOT registered in the DB (using default delta & alpha values). This may cause severe timing performance degradation!", p->name);
				p->calib.sfp.delta_tx = 0;
				p->calib.sfp.delta_rx = 0;
				p->calib.sfp.alpha = DEFAULT_FIBER_ALPHA_COEF;
		}

		p->state = HAL_PORT_STATE_LINK_DOWN;
		shw_sfp_set_tx_disable(p->hw_index, 0);
	}
}

static void on_remove_sfp(hal_port_state_t *p)
{
	handle_link_down(p, 0);
	p->state = HAL_PORT_STATE_DISABLED;
}

/* detects insertion/removal of SFP transceivers */
static void poll_sfps()
{
	if (tmo_expired(&tmo_sfp))
	{
		uint32_t mask = shw_sfp_module_scan();
		static int old_mask = 0;

		if(mask != old_mask)
		{
			int i, hw_index;
			for (i=0; i<HAL_MAX_PORTS; i++)
			{
				hw_index = ports[i].hw_index;

				if(ports[i].in_use && (mask ^ old_mask) & (1<<hw_index))
				{
					int insert = mask & (1<<hw_index);
					TRACE(TRACE_INFO, "Detected SFP %s on port %s.", insert ? "insertion" : "removal", ports[i].name);
					if(insert)
						on_insert_sfp(&ports[i]);
					else
						on_remove_sfp(&ports[i]);
				}
			}
		}
		old_mask = mask;
	}
}

/* Executes the port FSM for all ports. Called regularly by the main loop. */
void hal_update_ports()
{
	int i;

	poll_rts_state();
	poll_sfps();

	for(i=0; i<HAL_MAX_PORTS;i++)
		if(ports[i].in_use)
			port_fsm(&ports[i]);
		
	//ML BUG: 
	//    hmmm, the loop might result in change of pstate in rt_ipc,
	//    i.e. current_ref & backup_ref are set to REF_NONE when link
	//    down detected. If we don't update, the info will be out of
	//    sync
}

/* Queries the port state structre for a given network interface. */
static hal_port_state_t *lookup_port(const char *name)
{
	int i;
	for(i = 0; i< HAL_MAX_PORTS;i++)
		if(ports[i].in_use && !strcmp(name, ports[i].name))
			return &ports[i];

	return NULL;
}

int hal_enable_tracking(const char  *port_name)
{
	hal_port_state_t *p = lookup_port(port_name);

	if(!p) return PORT_ERROR;

  return rts_enable_ptracker(p->hw_index, 1) < 0 ? PORT_ERROR : PORT_OK;
}

/* Triggers the locking state machine, called by the PTPd during the WR link setup phase. */
int hal_port_start_lock(const char  *port_name, int priority)
{
	hal_port_state_t *p = lookup_port(port_name);

	if(!p) return PORT_ERROR;

/* can't lock to a disconnected port */
	if(p->state != HAL_PORT_STATE_UP)
		return PORT_BUSY;

/* fixme: check the main FSM state before */
	p->state = HAL_PORT_STATE_LOCKING;

	TRACE(TRACE_INFO, "Locking to port: %s", port_name);

	// single slave mode or no slave yet, don't do for backups
	// (the same condition in  rts_lock_channel())
	if(priority      < 0  || // single slave only (prio disabled
	   active_port() < 0)    // no slave yet
		rts_set_mode(RTS_MODE_BC);

  return rts_lock_channel(p->hw_index, priority) < 0 ? PORT_ERROR : PORT_OK;
}


/* Returns 1 if the port is locked */
int hal_port_check_lock(const char  *port_name)
{
	hal_port_state_t *p = lookup_port(port_name);

	if(!p) return PORT_ERROR;

    if(!rts_state_valid)
        return 0;

		if(rts_state.delock_count > 0)
				return 0;

    return ((rts_state.current_ref == p->hw_index || rts_state.backup_ref == p->hw_index )&&
            (rts_state.flags & RTS_DMTD_LOCKED) &&
            (rts_state.flags & RTS_REF_LOCKED));
}

/* Public function for querying the state of a particular port (DMTD phase, calibration deltas, etc.) */
int halexp_get_port_state(hexp_port_state_t *state, const char *port_name)
{
	hal_port_state_t *p = lookup_port(port_name);

//	TRACE(TRACE_INFO, "GetPortState %s [lup %x]\n", port_name, p);

  if(!p)
			return -1;



/* WARNING! when alpha = 1.0 (no asymmetry), fiber_fix_alpha = 0! */

	state->fiber_fix_alpha = (double)pow(2.0, 40.0) * ((p->calib.sfp.alpha + 1.0) / (p->calib.sfp.alpha + 2.0) - 0.5);

	state->valid = 1;
	state->mode = p->mode;
	state->up = (p->state != HAL_PORT_STATE_LINK_DOWN && p->state != HAL_PORT_STATE_DISABLED);

	state->is_locked = p->locked; //lock_state == LOCK_STATE_LOCKED;
	state->phase_val = p->phase_val;
	state->phase_val_valid = p->phase_val_valid;

	state->tx_calibrated = p->calib.tx_calibrated;
	state->rx_calibrated = p->calib.rx_calibrated;

	state->delta_tx = p->calib.delta_tx_phy + p->calib.sfp.delta_tx + p->calib.delta_tx_board;
	state->delta_rx = p->calib.delta_rx_phy + p->calib.sfp.delta_rx + p->calib.delta_rx_board;

	state->t2_phase_transition = DEFAULT_T2_PHASE_TRANS;
	state->t4_phase_transition = DEFAULT_T4_PHASE_TRANS;
	state->clock_period = REF_CLOCK_PERIOD_PS;

	memcpy(state->hw_addr, p->hw_addr, 6);
	state->hw_index = p->hw_index;

	return 0;
}
/* Public API function - returns the array of names of all WR network interfaces */
int halexp_query_ports(hexp_port_list_t *list)
{
	int i;
	int n = 0;

	for(i=0; i<HAL_MAX_PORTS;i++)
		if(ports[i].in_use)
			strcpy(list->port_names[n++], ports[i].name);

	list->num_physical_ports = num_physical_ports;
	list->num_ports = n;
	return 0;
}

/* Maciek's ptpx export for checking the presence of the external 10 MHz ref clock */
int hal_extsrc_check_lock()
{
    return (hal_get_timing_mode() != HAL_TIMING_MODE_BC) ? 1 : 0;
}

