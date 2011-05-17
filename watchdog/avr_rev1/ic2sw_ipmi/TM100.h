/*
 * TM100.h
 *
 *  Created on: 07-Jul-2009
 *      Author: poliveir
 */

#ifndef TM100_H_
#define TM100_H_

void tm100_init_sensor_record( void );
//uint16_t  tm100_init(	uint8_t interface, uint8_t i2c_addr );
uint16_t tm100_init(uint8_t interface, 	uint8_t i2c_addr );

void tm100_update_sensor( unsigned char *arg );

uint8_t * GetTM100SensorInfo(uint8_t pos);

#endif /* TM100_H_ */
