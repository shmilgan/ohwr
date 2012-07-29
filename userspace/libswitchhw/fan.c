/* Fan speed servo driver. Monitors temperature of I2C onboard sensors and drives the fan accordingly */

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <trace.h>

#include <pio.h>
#include <pio_pins.h>

#include <at91_softpwm.h>

#include "i2c.h"
#include "i2c_fpga_reg.h"

#define FAN_UPDATE_TIMEOUT 500000
#define FAN_TEMP_SENSOR_ADDR 0x4c

#define DESIRED_TEMPERATURE 50.0

#define PI_FRACBITS 8

extern const pio_pin_t PIN_mbl_box_fan_en[];

/* PI regulator state */
typedef struct {
  	float ki, kp; 			/* integral and proportional gains (1<<PI_FRACBITS == 1.0f) */
  	float integrator;		/* current integrator value */
  	float bias;					/* DC offset always added to the output */
		int anti_windup;	/* when non-zero, anti-windup is enabled */
		float y_min;				/* min/max output range, used by clapming and antiwindup algorithms */
		float y_max;			
		float x, y;         /* Current input (x) and output value (y) */
} pi_controller_t;


static i2c_fpga_reg_t fpga_sensors_bus_master = {
	.base_address = FPGA_I2C_SENSORS_ADDRESS,
	.prescaler = 500,
};

static struct i2c_bus fpga_sensors_i2c = {
		.name = "fpga_sensors",
		.type = I2C_BUS_TYPE_FPGA_REG,
		.type_specific = &fpga_sensors_bus_master,
};

static int pwm_fd;
static pi_controller_t fan_pi;

/* Processes a single sample (x) with PI control algorithm (pi). Returns the value (y) to 
	 drive the actuator. */
static inline float pi_update(pi_controller_t *pi, float x)
{
	float i_new, y;
	pi->x = x;
	i_new = pi->integrator + x;
	
	y = ((i_new * pi->ki + x * pi->kp)) + pi->bias;
	
	/* clamping (output has to be in <y_min, y_max>) and anti-windup:
	   stop the integrator if the output is already out of range and the output
	   is going further away from y_min/y_max. */
    if(y < pi->y_min)
    {
       	y = pi->y_min;
       	if((pi->anti_windup && (i_new > pi->integrator)) || !pi->anti_windup)
       		pi->integrator = i_new;
		} else if (y > pi->y_max) {
       	y = pi->y_max;
      	if((pi->anti_windup && (i_new < pi->integrator)) || !pi->anti_windup)
       		pi->integrator = i_new;
		} else /* No antiwindup/clamping? */
			pi->integrator = i_new;

    pi->y = y;
    return y;   
}

/* initializes the PI controller state. Currently almost a stub. */
static inline void pi_init(pi_controller_t *pi)
{
 	pi->integrator = 0;
}

static int enable_d0 = 0;


/* Configures a PWM output on gpio pin (pin). Rate accepts range from 0 (0%) to 1000 (100%) */
void pwm_configure_pin(const pio_pin_t *pin, int enable, int rate)
{
	int index = pin->port * 32 + pin->pin;
	
	if(enable && !enable_d0)
		ioctl(pwm_fd, AT91_SOFTPWM_ENABLE, index);
	else if(!enable && enable_d0) 
		ioctl(pwm_fd, AT91_SOFTPWM_DISABLE, index);
	
	enable_d0 = enable;
	ioctl(pwm_fd, AT91_SOFTPWM_SETPOINT, rate);
}


/* Texas Instruments TMP100 temperature sensor driver */
uint32_t tmp100_read_reg(int dev_addr, uint8_t reg_addr, int n_bytes)
{
	uint8_t data[8];
	uint32_t rv=0, i;
	
	data[0] = reg_addr;
	i2c_write(&fpga_sensors_i2c, dev_addr, 1, data); 
	i2c_read(&fpga_sensors_i2c, dev_addr, n_bytes, data); 
	
	for(i=0; i<n_bytes;i++)
	{
		rv <<= 8;
		rv |= data[i];
	}
	
	return rv;
}



void tmp100_write_reg(int dev_addr, uint8_t reg_addr, uint8_t value)
{
	uint8_t data[2];

	data[0] = reg_addr;
	data[1] = value;
	i2c_write(&fpga_sensors_i2c, dev_addr, 2, data); 
}


float tmp100_read_temp(int dev_addr)
{
		int temp = tmp100_read_reg(dev_addr, 0, 2); 
		return (float) (temp >> 4) / 16.0;
}

int shw_init_fans()
{
	uint8_t dev_map[128];
	int detect, i;
	
	TRACE(TRACE_INFO, "Configuring PWMs for fans (desired temperature = %.1f degC)...", DESIRED_TEMPERATURE);
	
	pwm_fd = open("/dev/at91_softpwm", O_RDWR);
	if(pwm_fd < 0)
	{
		TRACE(TRACE_FATAL, "at91_softpwm driver not installed or device not created.\n");
		return -1;
	}

	if (i2c_init_bus(&fpga_sensors_i2c) < 0) {
		TRACE(TRACE_FATAL, "can't initialize temperature sensors I2C bus.\n");
		return -1;
	}

	tmp100_write_reg(FAN_TEMP_SENSOR_ADDR, 1, 0x60); // 12-bit resolution

	fan_pi.ki = 1.0;
	fan_pi.kp = 4.0;
	fan_pi.y_min = 200;	
	fan_pi.bias = 200;	
	fan_pi.y_max = 800;	

	pi_init(&fan_pi);
	
	return 0;
}


/* Reads out the temperature and drives the fan accordingly */
void shw_update_fans()
{
	static int64_t last_tics = -1;
	int64_t cur_tics = shw_get_tics();
	
	if(last_tics < 0 || (cur_tics - last_tics) > FAN_UPDATE_TIMEOUT)
	{
		float t_cur = tmp100_read_temp(FAN_TEMP_SENSOR_ADDR);
		float drive = pi_update(&fan_pi, t_cur - DESIRED_TEMPERATURE);

		pwm_configure_pin((const pio_pin_t *)&PIN_mbl_box_fan_en, 1, (int)drive);
		last_tics = cur_tics;
	}
}

