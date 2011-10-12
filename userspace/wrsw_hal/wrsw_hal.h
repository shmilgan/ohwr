#ifndef __WRSW_HAL_H
#define __WRSW_HAL_H

#include <inttypes.h>

typedef void (*hal_cleanup_callback_t)();

#define PORT_BUSY 1
#define PORT_OK 0
#define PORT_ERROR -1


/* Port delay calibration parameters */
typedef struct {

	/* PHY delay measurement parameters for PHYs which require external calibration (i.e. with
	the feedback network. */

	/* minimum possible delay introduced by the PHY. Expressed as time
	(in picoseconds) between the beginning of the symbol on the serial input
	and the rising edge of the RX clock at which the deserialized word is
	available at the parallel output of the PHY. */
	uint32_t phy_rx_min;

	/* RX delay range of the PHY, expressed as a difference (in picoseconds)
	between the maximum and minimum possible RX delays. For example, a 1.25 Gbps
	PHY with minimum delay of 8 UI and maximum delay of 12 UI will have
	phy_rx_range equal to (12 - 8) * 800 ps = 3200 ps. Due to the nature of the
	calibration method, the measurement range is limited to one parallel clock cycle,
	i.e. 10 UIs, which is true for most 802.3z serdeses.  PHYs which have bigger
	delay variance can't be calibrated using this method. */
	uint32_t phy_rx_range;

	/* value of the phase shift (in picoseconds) measured by the calibrator DMTD
	when the PHY has locked on the minimum possible delay. Used to "unwind" the phase
	measurement into the PHY RX delay. This parameter must be determined experimentally. */
	uint32_t phy_rx_bias;


	/* the same set of parameters, but for the TX path of the PHY */
	uint32_t phy_tx_bias;
	uint32_t phy_tx_min;
	uint32_t phy_tx_range;

	/* Current PHY (clock-to-serial-symbol) TX and RX delays, in picoseconds */
	uint32_t delta_tx_phy;
	uint32_t delta_rx_phy;

	/* Current SFP (electrical in to optical out) TX and RX delays, in picoseconds */
	uint32_t delta_tx_sfp;
	uint32_t delta_rx_sfp;

	/* Current board routing delays (between the DDMTD inputs to the PHY clock
	inputs/outputs), in picoseconds */
	uint32_t delta_tx_board;
	uint32_t delta_rx_board;

	uint32_t raw_delta_rx_phy;
	uint32_t raw_delta_tx_phy;

  /* Fiber "alpha" asymmetry coefficient, as defined in the WRPTP Specification */
	double fiber_alpha;

  /* Fixed point fiber asymmetry coefficient. Expressed as (2 ^ 40 * (fiber_alpha - 1)). */      	
  int32_t fiber_fix_alpha;

  /* When non-zero: RX path is calibrated (delta_*_rx contain valid values) */
  int rx_calibrated;
  /* When non-zero: TX path is calibrated */
  int tx_calibrated;

} hal_port_calibration_t;


int hal_check_running();

int hal_parse_config();
void hal_config_set_config_file(const char *str);
int hal_config_extra_cmdline(const char *str);
int hal_config_get_int(const char *name, int *value);
int hal_config_get_double(const char *name, double *value);
int hal_config_get_string(const char *name, char *value, int max_len);
int hal_config_iterate(const char *section, int index, char *subsection, int max_len);

int hal_init_ports();
void hal_update_ports();

int hal_init_wripc();
int hal_update_wripc();

int hal_add_cleanup_callback(hal_cleanup_callback_t cb);

int hal_port_start_lock(const char  *port_name, int priority);
int hal_port_check_lock(const char  *port_name);
int hal_extsrc_check_lock(void); // added by ML


#endif
