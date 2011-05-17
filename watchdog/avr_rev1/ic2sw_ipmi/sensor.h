/*
 * sensor.h
 *
 *  Created on: 07-Jul-2009
 *      Author: poliveir
 */

#ifndef SENSOR_H_
#define SENSOR_H_


#include <inttypes.h>
#include "ipmi.h"


typedef struct sensor_data {
	uint8_t	sensor_id;
	uint8_t	last_sensor_reading;
	uint8_t	scan_period;		/* time between each sensor scan in seconds, 0 = no scan */
	void(*scan_function)( void * );	/* the routine that does the sensor scan */
	uint8_t	reserved:5,
		unavailable:1,              /* 1b = reading/state unavailable */
		sensor_scanning_enabled:1,  /* 0b = sensor scanning disabled */
		event_messages_enabled:1; /* 0b = All Event Messages disabled from this sensor */

} SENSOR_DATA;

typedef struct sdr_entry {
	uint16_t  record_id;
	uint8_t   rec_len;
	uint8_t   *record_ptr;
} SDR_ENTRY;

uint16_t sensor_add( FULL_SENSOR_RECORD *sdr, SENSOR_DATA *sensor_data );


#endif /* SENSOR_H_ */
