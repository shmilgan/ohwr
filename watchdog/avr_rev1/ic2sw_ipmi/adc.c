/*
 *
 *
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "global.h"
#include "adc.h"

// global variables

// functions

// initialize adc converter
void adcInit(void)
{
	// configure adc port (PORTA) as input

	// so we can receive analog signals
	DDRA &= ~(_BV(PA3)|_BV(PA4)|_BV(PA5)|_BV(PA6)|_BV(PA7));

	// make sure pull-up resistors are turned off
	PORTA &= ~(_BV(PA3)|_BV(PA4)|_BV(PA5)|_BV(PA6)|_BV(PA7));


	sbi(ADCSRA, ADEN);				// enable ADC (turn on ADC power)
	cbi(ADCSRA, ADFR);				// default to single sample convert mode
	adcSetPrescaler(ADC_PRESCALE);	// set default prescaler
	adcSetReference(ADC_REFERENCE_AVCC);	// set default reference
	cbi(ADMUX, ADLAR);				// set to right-adjusted result
//	cbi(ADCSRA, ADIE);				// enable ADC interrupts


	//sei();						// turn on interrupts (if not already on)
}

// turn off adc converter
void adcOff(void)
{
	cbi(ADCSRA, ADIE);				// disable ADC interrupts
	cbi(ADCSRA, ADEN);				// disable ADC (turn off ADC power)
}

// configure adc converter clock division (prescaling)
void adcSetPrescaler(unsigned char prescale)
{
	ADCSRA= (((ADCSRA) & ~ADC_PRESCALE_MASK) | prescale);
}

// configure adc converter voltage reference
void adcSetReference(unsigned char ref)
{
	ADMUX= (((ADMUX) & ~ADC_REFERENCE_MASK) | (ref<<6));
}

// sets the adc input channel
void adcSetChannel(unsigned char ch)
{
	ADMUX=((ADMUX & ~ADC_MUX_MASK) | (ch & ADC_MUX_MASK));	// set channel
}

// start a conversion on the current adc input channel
void adcStartConvert(void)
{
	sbi(ADCSRA, ADIF);	// clear hardware "conversion complete" flag
	sbi(ADCSRA, ADSC);	// start conversion
}

// return TRUE if conversion is complete
uint8_t adcIsComplete(void)
{
	return bit_is_set(ADCSRA, ADSC);
}

// Perform a 10-bit conversion
// starts conversion, waits until conversion is done, and returns result
unsigned short adcConvert10bit(unsigned char ch)
{
	uint8_t adcl,adch;
	adcSetChannel(ch);
	sbi(ADCSRA, ADIF);						// clear hardware "conversion complete" flag
	sbi(ADCSRA, ADSC);						// start conversion

	while( bit_is_set(ADCSRA, ADSC) );		// wait until conversion complete

	// CAUTION: MUST READ ADCL BEFORE ADCH!!!
	adcl=ADCL;
	adch=ADCH;

	return (adcl | (adch<<8));	// read ADC (full 10 bits);
}
// Perform a 8-bit conversion.
// starts conversion, waits until conversion is done, and returns result
unsigned char adcConvert8bit(unsigned char ch)
{
	// do 10-bit conversion and return highest 8 bits
	return adcConvert10bit(ch)>>2;			// return ADC MSB byte
}

