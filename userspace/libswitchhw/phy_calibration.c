#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <asm/types.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <sys/ioctl.h>


#include <hw/switch_hw.h>
#include <hw/phy_calibration.h>
#include <hw/dmtd_calibrator_regs.h>
#include <hw/clkb_io.h>

#define CAL_DMTD_AVERAGING_STEPS 256

#define PORT_UPLINK 0
#define PORT_DOWNLINK 1

//driver-specific private ioctls

#define PRIV_IOCGCALIBRATE (SIOCDEVPRIVATE+1)
#define PRIV_IOCGGETPHASE (SIOCDEVPRIVATE+2)

#define CAL_CMD_TX_ON 1
#define CAL_CMD_TX_OFF 2
#define CAL_CMD_RX_ON 3
#define CAL_CMD_RX_OFF 4
#define CAL_CMD_RX_CHECK 5

struct wrmch_calibration_req {
	int cmd;
	int cal_present;
};

struct wrmch_phase_req {
	int ready;
	uint32_t phase;
};

static int cal_socket;

static char cal_current_if[16];
static int cal_in_progress = 0;
static int cal_current_lane ;
static int cal_current_port_index;
static int cal_current_port_type;

#define UPLINK_CAL_IN_UP0_RBCLK 0
#define UPLINK_CAL_IN_UP1_RBCLK 1
#define UPLINK_CAL_IN_REFCLK 2


#define DOWNLINK_CAL_IN_REFCLK 0
#define DOWNLINK_CAL_IN_RBCLK(x) (x+1)


static int uplink_calibrator_configure(int n_avg, int lane, int index)
{
	int input;

	if(lane == PHY_CALIBRATE_TX)
		input = UPLINK_CAL_IN_REFCLK;
	else if(lane == PHY_CALIBRATE_RX)
	{
 		if(index == 0)
 			input = UPLINK_CAL_IN_UP0_RBCLK;
 		else
 			input = UPLINK_CAL_IN_UP1_RBCLK;
	}


	shw_clkb_write_reg(CLKB_BASE_CALIBRATOR + DPC_REG_CR, 0);
	shw_clkb_write_reg(CLKB_BASE_CALIBRATOR + DPC_REG_CR, DPC_CR_EN | DPC_CR_N_AVG_W(n_avg) | DPC_CR_IN_SEL_W(input));
}

int route_order[8] = {0,1,2,3,4,5,6,7};

static int downlink_calibrator_configure(int n_avg, int lane, int index)
{
	int input;

	if(lane == PHY_CALIBRATE_TX)
		input = DOWNLINK_CAL_IN_REFCLK;
	else if(lane == PHY_CALIBRATE_RX)
	{
		input = DOWNLINK_CAL_IN_RBCLK(route_order[index]);
	}


	_fpga_writel(FPGA_BASE_CALIBRATOR + DPC_REG_CR, 0);
	_fpga_writel(FPGA_BASE_CALIBRATOR + DPC_REG_CR, DPC_CR_EN | DPC_CR_N_AVG_W(n_avg) | DPC_CR_IN_SEL_W(input));
}



static int uplink_calibrator_measure(int *phase_raw)
{
	uint32_t rval;
	rval = shw_clkb_read_reg(CLKB_BASE_CALIBRATOR + DPC_REG_SR);

	fprintf(stderr,"UplinkCalMeas: rval %x cr %x\n", rval, shw_clkb_read_reg(CLKB_BASE_CALIBRATOR+DPC_REG_CR));

	if(rval & DPC_SR_PS_RDY)
	{
		shw_clkb_write_reg(CLKB_BASE_CALIBRATOR + DPC_REG_SR, 0xffffffff);
		*phase_raw = (rval & (1<<23) ? 0xff000000 | rval : (rval & 0xffffff));
		return 1;
	}
	return 0;
}


static int downlink_calibrator_measure(int *phase_raw)
{
	uint32_t rval;
	rval = _fpga_readl(FPGA_BASE_CALIBRATOR + DPC_REG_SR);
	if(rval & DPC_SR_PS_RDY)
	{
		_fpga_writel(FPGA_BASE_CALIBRATOR + DPC_REG_SR, 0xffffffff);
		*phase_raw = (rval & (1<<23) ? 0xff000000 | rval : (rval & 0xffffff));
		return 1;
	}
	return 0;
}



static int decode_port_name(const char *if_name, int *type, int *index)
{
	if(strlen(if_name) != 4)
		return -1;

	if( if_name[3] < '0' || if_name[3] > '7')
		return -1;

	*index = if_name[3] - '0';

	if(!strncmp(if_name, "wru", 3))
		*type = PORT_UPLINK;
	else if(!strncmp(if_name, "wrd", 3))
		*type = PORT_DOWNLINK;
	else return -1;

	return 0;
}

static int do_net_ioctl(const char *if_name, int request, void *data)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_PACKET;
	strcpy(ifr.ifr_name, if_name);
	ifr.ifr_data = data;

	int rv = ioctl(cal_socket, request, &ifr);

	return rv;
}

int shw_cal_init()
{
	TRACE(TRACE_INFO,"Initializing PHY calibrators...");
	cal_in_progress = 0;

	// create a raw socket for calibration/DMTD readout
	cal_socket = socket(AF_PACKET, SOCK_DGRAM, 0);

	if(cal_socket < 0)
		return -1;

	return xpoint_configure();
}

int shw_cal_enable_pattern(const char *if_name, int enable)
{
	int type, index, rval;
	struct wrmch_calibration_req crq;

	// TRACE(TRACE_INFO,"port %s enable %d", if_name, enable);

	if((rval=decode_port_name(if_name, &type, &index)) < 0) return rval;

	crq.cmd = enable ? CAL_CMD_TX_ON : CAL_CMD_TX_OFF;

	if(type == PORT_UPLINK)
	{
		if(do_net_ioctl(if_name, PRIV_IOCGCALIBRATE, &crq) < 0)
		{
			TRACE(TRACE_ERROR, "Can't send TX calibration pattern");
			return -1;
		}
	}
	return 0;
}

int shw_cal_enable_feedback(const char *if_name, int enable, int lane)
{
	int type, index, rval;
	struct wrmch_calibration_req crq;

	//  TRACE(TRACE_INFO,"port %s enable %d lane %d", if_name, enable, lane);

	if((rval=decode_port_name(if_name, &type, &index)) < 0) return rval;

	//  TRACE(TRACE_INFO, "enable_feedback type:%d index:%d\n", type, index);

	cal_current_lane = lane;
	cal_current_port_index = index;
	cal_current_port_type = type;
	strcpy(cal_current_if, if_name);

	if(type == PORT_UPLINK)
	{
		if(enable)
		{


			switch(lane)
			{
			case PHY_CALIBRATE_TX:

				cal_in_progress = 1;
				xpoint_cal_feedback(1, index, 0); // enable PHY TX line feedback
				//	      TRACE(TRACE_INFO, "TX index %d", index);



				uplink_calibrator_configure(CAL_DMTD_AVERAGING_STEPS, lane, index);
				break;

			case PHY_CALIBRATE_RX:

				cal_in_progress = 1;
				xpoint_cal_feedback(1, index, 1); // enable PHY RX line feedback


				crq.cmd = CAL_CMD_RX_ON;

				if(do_net_ioctl(if_name, PRIV_IOCGCALIBRATE, &crq) < 0)
				{
					TRACE(TRACE_ERROR, "Can't enable RX calibration pattern");
					return -1;
				}

				//  			crq.cmd = CAL_CMD_TX_ON;
				//				if(do_net_ioctl(if_name, &crq) < 0) return -1;


				uplink_calibrator_configure(CAL_DMTD_AVERAGING_STEPS, lane, index);


				break;


			}
		} else { // enable == 0
			TRACE(TRACE_INFO, "Disabling calibration on port: %s", if_name);
			cal_in_progress = 0;
			xpoint_cal_feedback(0, 0, 0);

			if(lane == PHY_CALIBRATE_TX)
			{
				crq.cmd = CAL_CMD_TX_OFF;
				if(do_net_ioctl(if_name, PRIV_IOCGCALIBRATE, &crq) < 0) return -1;
			} else if (lane == PHY_CALIBRATE_RX)
			{
				crq.cmd = CAL_CMD_RX_OFF;
				if(do_net_ioctl(if_name, PRIV_IOCGCALIBRATE, &crq) < 0) return -1;
			}
		}
	} else if(type == PORT_DOWNLINK)
	{
		TRACE(TRACE_ERROR, "Sorry, no downlinks support yet...\n");
		return -1;
	}
}

int shw_cal_measure(uint32_t *phase)
{
	int phase_raw;
	struct wrmch_calibration_req crq;

	if(!cal_in_progress)
		return -1;

	if(cal_current_port_type == PORT_UPLINK)
	{
		switch(cal_current_lane)
		{
		case PHY_CALIBRATE_TX:
			fprintf(stderr,"CalTxMeasUplink\n");


			if(uplink_calibrator_measure(&phase_raw))
			{
				*phase = (uint32_t) ((double) phase_raw / (double) CAL_DMTD_AVERAGING_STEPS / 1.0 /* shw_hpll_get_divider() */ * 8000.0);
				return 1;
			} else return 0;

			break;
		case PHY_CALIBRATE_RX:
			crq.cmd = CAL_CMD_RX_CHECK;
			if(do_net_ioctl(cal_current_if, PRIV_IOCGCALIBRATE, &crq) < 0) return -1;

			if(crq.cal_present && uplink_calibrator_measure(&phase_raw))
			{
				*phase = (uint32_t) ((double) phase_raw / (double) CAL_DMTD_AVERAGING_STEPS / 1.0 /* shw_hpll_get_divider */ * 8000.0);
				return 1;
			} else return 0;

			break;

		}

	} else {
		//	TRACE(TRACE_ERROR, "Sorry, no downlinks support yet...\n");
		return -1;
	}


}

int shw_poll_dmtd(const char *if_name, uint32_t *phase_ps)
{
	struct wrmch_phase_req phr;
	int r;

	r = do_net_ioctl(if_name, PRIV_IOCGGETPHASE, &phr);

	if(r < 0)
	{
		TRACE(TRACE_ERROR, "failed ioctl(PRIV_IOCGGETPHASE): retval %d\n", r);
		return -1;
	}


	if(!phr.ready)  // No valid DMTD measurement?
		return 0;

	//  TRACE(TRACE_INFO,"%s: phase %d", if_name, phr.phase);

	*phase_ps = 0; /* So it compiles, but this file must die as well */
	return 1;
}

