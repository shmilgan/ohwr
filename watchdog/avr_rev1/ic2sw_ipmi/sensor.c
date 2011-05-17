/*
 * sensor.c
 *
 *  Created on: 07-Jul-2009
 *      Author: poliveir
 */

#include "sensor.h"


#define MAX_SDR_COUNT		2
#define MAX_SENSOR_COUNT	2

// in this implementation: number of sensors == number of SDRs
uint8_t current_sensor_count = 0;

SDR_ENTRY sdr_entry_table[MAX_SDR_COUNT];

unsigned sdr_reservation_id = 0;
SENSOR_DATA *sensor[MAX_SENSOR_COUNT];

/*======================================================================*/
uint16_t sensor_add( FULL_SENSOR_RECORD *sdr, SENSOR_DATA *sensor_data )
{
    if( current_sensor_count + 1 > MAX_SENSOR_COUNT ) return( -1 );

    sdr_entry_table[current_sensor_count].record_ptr = ( uint8_t * )sdr;
    ((FULL_SENSOR_RECORD *)(sdr_entry_table[current_sensor_count].record_ptr))->sensor_number = current_sensor_count;
    ((FULL_SENSOR_RECORD *)(sdr_entry_table[current_sensor_count].record_ptr))->record_id[0] = current_sensor_count;
    sensor[current_sensor_count] = sensor_data;
    current_sensor_count++;
    return( 0 );
} // sensor_add
