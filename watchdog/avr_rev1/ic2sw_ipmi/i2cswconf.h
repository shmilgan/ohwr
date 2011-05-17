#ifndef I2CSWCONF_H
#define I2CSWCONF_H


//tmp100 bus

	// clock line port
	#define SCLPORT_TEMP	PORTC	// i2c clock port
	#define SCLDDR_TEMP	DDRC	// i2c clock port direction
	#define SCLPIN_TEMP	PINC	// i2c data port input

	// data line port
	#define SDAPORT_TEMP	PORTC	// i2c data port
	#define SDADDR_TEMP	DDRC	// i2c data port direction
	#define SDAPIN_TEMP	PINC	// i2c data port input

	// pin assignments
	#define SCL_TEMP     PC2
	#define SDA_TEMP     PC3

//IMPB0_A

	// clock line port

	#define SCLPORT_A	PORTG	// i2c clock port
	#define SCLDDR_A	DDRG	// i2c clock port direction
	#define SCLPIN_A	PING	// i2c data port input

	// data line port
	#define SDAPORT_A	PORTA	// i2c data port
	#define SDADDR_A	DDRA	// i2c data port direction
	#define SDAPIN_A	PINA	// i2c data port input

	// pin assignments
	#define SCL_A     PG2
	#define SDA_A    PA2


//IMPB0_B
	/// clock line port
	#define SCLPORT_B	PORTA	// i2c clock port
	#define SCLDDR_B	DDRA	// i2c clock port direction
	#define SCLPIN_B	PINA	// i2c data port input

	// data line port
	#define SDAPORT_B	PORTA	// i2c data port
	#define SDADDR_B	DDRA	// i2c data port direction
	#define SDAPIN_B	PINA	// i2c data port input

	// pin assignments
	#define SCL_B     PA1
	#define SDA_B     PA0


#endif
