/*
 * TM100.c
 *
 *  Created on: 07-Jul-2009
 *      Author: poliveir
 */

#include<avr/pgmspace.h>
#include <stdio.h>
#include "sensor.h"
#include "ws.h"
#include "uart.h"
#include "i2csw.h"
#include "tm100.h"

// register select bits
#define REGSEL_TEMP	0	// Temperature Register (READ Only)
#define REGSEL_CONFIG	1	// Configuration Register (READ/WRITE)
#define REGSEL_TLOW	2	// TLOW Register (READ/WRITE)
#define REGSEL_THIGH	3	// THIGH Register (READ/WRITE)


#define MAX_TM100_SENSOR_COUNT 2
unsigned char tm100_sensor_count = 0;

typedef struct tm100_sensor_info {
	unsigned char sensor_id;
	unsigned char reading_hi;
	unsigned char reading_lo;
	unsigned char interface;
	unsigned char i2c_addr;
} TM100_SENSOR_INFO;

// Pointer Register Byte
typedef struct pointer_register {
	uint8_t	register_select:2,reserved:6;
}POINTER_REGISTER;

// configuration register
typedef struct configuration_register {
	uint8_t	shutdown_mode:1,
			thermostat_mode:1,
			polarity:1,
			fault_queue:2,
			conv_resolution:2,
			one_shot:1;
}CONFIGURATION_REGISTER;

TM100_SENSOR_INFO tm100_sensor[MAX_TM100_SENSOR_COUNT];


FULL_SENSOR_RECORD tm100sr;
SENSOR_DATA tm100sd;

void tm100_update_sensor_completion_function( IPMI_WS *ws, int status );
void tm100_init_completion_function( IPMI_WS *ws, int status );


uint16_t
tm100_init(uint8_t interface,
	uint8_t i2c_addr )
{
	printf("init temperature sensors\n\r");
	POINTER_REGISTER *preg;
	CONFIGURATION_REGISTER *creg;

	if( tm100_sensor_count >= MAX_TM100_SENSOR_COUNT )
		return( -1 );

	IPMI_WS *req_ws;

	//tm100_init_sensor_record();

	if( !( req_ws = ws_alloc() ) ) {
		printfr(PSTR("tm100_init no available ws\n\r"));

		return( -1 );
	}


	// keep track of initialized sensors
	tm100_sensor[tm100_sensor_count].interface = interface;
	tm100_sensor[tm100_sensor_count].i2c_addr = i2c_addr;
	tm100_sensor[tm100_sensor_count].sensor_id = tm100_sensor_count;

	tm100_sensor_count++;
	// we're going to do a write of two byes, first byte is the pointer reg,
	// the second the config register
	req_ws->pkt_out[0]=i2c_addr;
	preg = ( POINTER_REGISTER * )&( req_ws->pkt_out[1] );
	preg->register_select = REGSEL_CONFIG;

	creg = ( CONFIGURATION_REGISTER * )&( req_ws->pkt_out[2] );
	creg->shutdown_mode = 0;	// disable shutdown
	creg->thermostat_mode = 0;	// use comparator mode
	creg->polarity = 0;		// ALERT active low
	creg->fault_queue = 1;		// 2 consecutive faults
	creg->conv_resolution = 3;	// 12 bits
	creg->one_shot = 0;

	req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->ipmi_completion_function =tm100_init_completion_function;
	req_ws->addr_out = i2c_addr;
	req_ws->interface = interface;
	req_ws->len_out = 2;
	req_ws->I2C_BUS=getBus(I2C_TEMP);

	ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );

	return( tm100_sensor_count - 1 );
}

void
tm100_init_sensor_record( void )
{
// add initialization to the sensor full record

 sensor_add(&tm100sr,&tm100sd);
}

uint8_t * GetTM100SensorInfo(uint8_t pos)
{
	return (uint8_t * )&tm100_sensor[pos];
}

/*
The Temperature Register is a 12-bit, read-only register that stores the output
of the most recent conversion. Two bytes must be read to obtain data.
Byte 1 is the most significant byte, followed by byte 2, the least significant
byte. Following power-up or reset, the Temperature Register will read 0°C until
the first conversion is complete.
*/
void tm100_update_sensor( unsigned char *arg )
{
	IPMI_WS *req_ws;
	TM100_SENSOR_INFO *t100_sensor_info = ( TM100_SENSOR_INFO * )arg;

	if( !( req_ws = ws_alloc() ) ) {
		return;
	}

	req_ws->incoming_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->incoming_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->ipmi_completion_function = tm100_update_sensor_completion_function;
	req_ws->addr_out = t100_sensor_info->i2c_addr;
	req_ws->interface = t100_sensor_info->interface;
	req_ws->len_rcv = 2;
	req_ws->I2C_BUS=getBus(I2C_TEMP);

	ws_set_state( req_ws, WS_ACTIVE_MASTER_READ );
}

int
tm100_update_sensor_reading(
	IPMI_WS *req_ws,
	unsigned char interface,
	unsigned char i2c_addr,
	unsigned char *sensor_data )
{

	req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->incoming_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->outgoing_channel = IPMI_CH_NUM_PRIMARY_IPMB;
	req_ws->addr_out = i2c_addr;
	req_ws->interface = interface;
	req_ws->ipmi_completion_function = tm100_update_sensor_completion_function;
	req_ws->len_rcv = 2;	/* amount of data we want to read */

	/* dispatch the request */
	ws_set_state( req_ws, WS_ACTIVE_MASTER_READ );

	return( 0 );
}

void tm100_update_sensor_completion_function( IPMI_WS *ws, int status )
{
	unsigned char sensor_id;

	printfr(PSTR("tm100_update_sensor_completion_function\n\r"));

	// find sensor id
	for( sensor_id = 0; sensor_id <tm100_sensor_count; sensor_id++ )
	{
		if( ( tm100_sensor[sensor_id].interface == ws->interface ) &&
		    ( tm100_sensor[sensor_id].i2c_addr == ws->addr_out ) ) {
			break;
	    	}
	}

	switch ( status ) {
		case XPORT_REQ_NOERR:
			tm100_sensor[sensor_id].reading_hi = ws->pkt_in[0];
			tm100_sensor[sensor_id].reading_lo = ws->pkt_in[1];
			break;
		case XPORT_REQ_ERR:
		case XPORT_RESP_NOERR:
		case XPORT_RESP_ERR:
		default:
			tm100_sensor[sensor_id].reading_hi = 0;
			tm100_sensor[sensor_id].reading_lo = 0;
			break;
	}
	ws_free( ws );

	tm100_update_sensor((unsigned char *)&tm100_sensor[sensor_id]);

}

/* This function handles completion for two events:
 * 	- initial config write to tm100
 * 	- write to switch the register selector to the temperature register
 */
void
tm100_init_completion_function( IPMI_WS *ws, int status )
{
	POINTER_REGISTER *preg;
	unsigned char sensor_id;

	preg = (POINTER_REGISTER *)&(ws->pkt_out[1]);

	if( preg->register_select == REGSEL_CONFIG ) {
		// if we completed initial config write to tm100,
		// switch the register selector to the temperature register
		preg->register_select = REGSEL_TEMP;
		ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
		ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
		ws->ipmi_completion_function = tm100_init_completion_function;
		ws->len_out = 1;
		ws_set_state( ws, WS_ACTIVE_MASTER_WRITE );
	} else 	if( preg->register_select == REGSEL_TEMP ) {
		// we completed a write to switch the register
		// selector to the temperature register

		// find sensor id
		for( sensor_id = 0; sensor_id < tm100_sensor_count; sensor_id++ ) {
			if( ( tm100_sensor[sensor_id].interface == ws->interface ) &&
			    ( tm100_sensor[sensor_id].i2c_addr == ws->addr_out ) ) {
				break;
			    }
		}
		ws_free( ws );

		tm100_update_sensor(&tm100_sensor[sensor_id]);
		// Register a callout to read temp sensors periodically
	//	timer_add_callout_queue( ( void * )&lm75_update_sensor_timer_handle,
	//       		10*HZ, lm75_update_sensor, ( unsigned char * )&lm75_sensor[sensor_id] ); /* 10 sec timeout */
	}
}
